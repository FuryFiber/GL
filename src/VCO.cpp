#include "plugin.hpp"
#include <math.h>
#include "stdio.h"

using float_4 = simd::float_4;
const int maxPolyphony = 16;
static const int maxBanks = maxPolyphony / 4;

inline float_4 sinTwoPi(float_4 _x) {
    const static float twoPi = 2 * 3.141592653589793238;
    const static float pi =  3.141592653589793238;
    _x -= ifelse((_x > float_4(pi)), float_4(twoPi), float_4::zero());

    float_4 xneg = _x < float_4::zero();
    float_4 xOffset = ifelse(xneg, float_4(pi / 2.f), float_4(-pi  / 2.f));
    xOffset += _x;
    float_4 xSquared = xOffset * xOffset;
    float_4 ret = xSquared * float_4(1.f / 24.f);
    float_4 correction = ret * xSquared *  float_4(.02 / .254);
    ret += float_4(-.5);
    ret *= xSquared;
    ret += float_4(1.f);

    ret -= correction;
    return ifelse(xneg, -ret, ret);
}

struct VCO : Module {
	enum ParamId {
		PITCH_PARAM,
		PULSE_PARAM,
		FMPARAM_PARAM,
		PULSEMODPARAM_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		PULSEMOD_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SINE_OUTPUT,
		TRIANGLE_OUTPUT,
		SAW_OUTPUT,
		SQUARE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

    float_4 phaseAccumulators[maxBanks] = {};
    float_4 phaseAdvance[maxBanks] = {};
    dsp::MinBlepGenerator<16, 16, float_4> sawMinBlep[maxBanks];
    dsp::MinBlepGenerator<16, 16, float_4> sqrMinBlep[maxBanks];
    float_4 dcOffsetCompensation[maxBanks] = {};
    bool sync_connected = false;
    float sync_prev = 0;

    float pulsewidth = 1.f;
    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    bool outputSaw = false;
    bool outputSin = false;
    bool outputSqr = false;
    bool outputTri = false;

    VCO() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM, 0, 10, 4, "Frequency");
        configParam(PULSE_PARAM, 0.01f, 0.99f, 0.5f, "Pulse width", "%", 0.f, 100.f);
        configParam(FMPARAM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
        configParam(PULSEMODPARAM_PARAM, -1.f, 1.f, 0.f, "Pulse Width modulation", "%", 0.f, 100.f);
        configInput(VOCT_INPUT, "1V/octave pitch");
        configInput(FM_INPUT, "Frequency modulation");
        configInput(SYNC_INPUT, "Sync");
        configInput(PULSEMOD_INPUT, "Pulse width modulation");
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(TRIANGLE_OUTPUT, "Triangle");
        configOutput(SAW_OUTPUT, "Saw");
        configOutput(SQUARE_OUTPUT, "Square");
    }

    /*
     * Update module state every sample rate tick
     * ie generate output signals
     */
    void process(const ProcessArgs& args) override {
        if (loopCounter-- == 0) {
            loopCounter = 3;
            processEvery4Samples(args);
        }

        generateOutput();
    }

    /*
     * functionality that only needs to be called every so often to save some performance
     * mostly reading parameters and modulation inputs
     * since modulation inputs are only read every 4 samples, sending audio rate frequencies into these inputs might give wrong result
     */
    void processEvery4Samples(const ProcessArgs& args) {
        currentPolyphony = std::max(1, inputs[VOCT_INPUT].getChannels());
        currentBanks = currentPolyphony / 4;
        if (currentPolyphony % 4) {
            ++currentBanks;
        }

        // set sync connected
        sync_connected = inputs[SYNC_INPUT].isConnected();

        // set output channels polyphony
        outputs[SINE_OUTPUT].setChannels(currentPolyphony);
        outputs[SAW_OUTPUT].setChannels(currentPolyphony);
        outputs[SQUARE_OUTPUT].setChannels(currentPolyphony);
        outputs[TRIANGLE_OUTPUT].setChannels(currentPolyphony);

        // check for connected outputs
        outputSaw = outputs[SAW_OUTPUT].isConnected();
        outputSin = outputs[SINE_OUTPUT].isConnected();
        outputSqr = outputs[SQUARE_OUTPUT].isConnected();
        outputTri = outputs[TRIANGLE_OUTPUT].isConnected();

        float pulseWidthParam = params[PULSE_PARAM].getValue();
        float pulseModParam = params[PULSEMODPARAM_PARAM].getValue();

        // This number was determined by measuring sawtooth voltage offset
        // It's what the offset would be at sample rate.
        const float sawCorrect = -5.698;

        // Set pulsewidth
        pulsewidth = pulseWidthParam + inputs[PULSEMOD_INPUT].getVoltage() / 10.f * pulseModParam;
        pulsewidth = clamp(pulsewidth, 0.01f, 1.f - 0.01f);

        // calculate pitch
        float_4 pitchParam = params[PITCH_PARAM].value;
        float fmParam = params[FMPARAM_PARAM].getValue();
        for (int bank = 0; bank < currentBanks; ++bank) {
            const int currentChannel = bank * 4;

            float_4 pitchCV = inputs[VOCT_INPUT].getPolyVoltageSimd<float_4>(currentChannel);
            float_4 combinedPitch = pitchParam + pitchCV - float_4(4.f);

            const float_4 q = float(std::log2(261.626));       // move up to C
            combinedPitch += q;
            combinedPitch += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(currentChannel) * fmParam;


            const float_4 freq = rack::dsp::approxExp2_taylor5<float_4>(combinedPitch);

            const float_4 normalizedFreq = float_4(args.sampleTime) * freq;
            phaseAdvance[bank] = normalizedFreq;

            dcOffsetCompensation[bank] = normalizedFreq * float_4(sawCorrect);
        }
    }

    void generateOutput() {
        for (int bank = 0; bank < currentBanks; ++bank) {
            const int baseChannel = bank * 4;
            const int relativeChannel = currentPolyphony - baseChannel;

            // advance phase and wrap
            phaseAccumulators[bank] += phaseAdvance[bank];
            phaseAccumulators[bank] -= simd::floor(phaseAccumulators[bank]);

            // sync
            if (sync_connected) {
                // get sync
                float sync_in = inputs[SYNC_INPUT].getVoltage();

                if (sync_in >=0 && sync_prev < 0) {
                    // if sync has crossed over, set all phases to 0
                    for (int i=0; i<maxBanks; i++){
                        phaseAccumulators[i] = 0.f;
                    }
                }

                // update sync
                sync_prev = sync_in;
            }


            float_4 SawMinBlepValue;
            if (outputSaw) {
                // Evaluate the phase, and determine if we are at a discontinuity.
                // Determine if the saw "should have" already crossed .5V in the last sample period
                float_4 halfCrossing = (0.5f - (phaseAccumulators[bank] -  phaseAdvance[bank])) /  phaseAdvance[bank];
                int halfMask = simd::movemask((0 < halfCrossing) & (halfCrossing <= 1.f));
                if (halfMask) {
                    for (int subChannel=0; subChannel < relativeChannel; ++subChannel) {
                        if (halfMask & (1 << subChannel)) {
                            float_4 mask = simd::movemaskInverse<float_4>(1 << subChannel);
                            float jumpPhase = halfCrossing[subChannel] - 1.f;
                            float_4 jumpAmount = mask & -2.f;
                            sawMinBlep[bank].insertDiscontinuity(jumpPhase, jumpAmount);
                        }
                    }
                }
                SawMinBlepValue = sawMinBlep[bank].process();
            }

            float_4 sqrMinBlepValue;
            if (outputSqr) {
                // jump square when crossing 0
                float_4 wrapCrossing = (- (phaseAccumulators[bank] -  phaseAdvance[bank])) /  phaseAdvance[bank];
                int halfmask = simd::movemask((0<wrapCrossing) & (wrapCrossing <= 1.f));
                if (halfmask) {
                    for (int subChannel = 0; subChannel<relativeChannel; ++subChannel){
                        float_4 mask = simd::movemaskInverse<float_4>(1 << subChannel);
                        float jumpPhase = wrapCrossing[subChannel] - 1.f;
                        float_4 jumpAmount = mask & -2.f;
                        sqrMinBlep[bank].insertDiscontinuity(jumpPhase, jumpAmount);
                    }
                }

                // jump square when crossing pulse width
                float_4 pulseCrossing = (pulsewidth - (phaseAccumulators[bank] - phaseAdvance[bank])) / phaseAdvance[bank];
                int pulseJump = simd::movemask((0 < pulseCrossing) & (pulseCrossing <= 1.f));
                if (pulseJump) {
                    for (int subChannel=0; subChannel < relativeChannel; ++subChannel){
                        float_4 mask = simd::movemaskInverse<float_4>(1 << subChannel);
                        float jumpPhase = pulseCrossing[subChannel] - 1.f;
                        float_4 jumpAmount = mask & -2.f;
                        sqrMinBlep[bank].insertDiscontinuity(jumpPhase, jumpAmount);
                    }
                }

                sqrMinBlepValue = sqrMinBlep[bank].process();
            }

            if (outputSaw) {
                // generate raw saw signal
                float_4 rawSaw = phaseAccumulators[bank] + float_4(.5f);
                rawSaw -= simd::trunc(rawSaw);
                rawSaw = 2 * rawSaw - 1;

                // add antialiassing minblep
                rawSaw += SawMinBlepValue;
                rawSaw += dcOffsetCompensation[bank];

                // transform from -1v / 1v to -5v / 5v and send to output
                float_4 sawWave = float_4(5) * rawSaw;
                outputs[SAW_OUTPUT].setVoltageSimd(sawWave, baseChannel);
            }

            if (outputSqr) {
                // generate raw square signal
                float_4 rawSqr = simd::ifelse(phaseAccumulators[bank] < pulsewidth, 1.f, -1.f);

                // add minblep antialiassing
                rawSqr += sqrMinBlepValue;

                // transform from -1v / 1v to -5v / 5v and send to output
                float_4 sqrWave = float_4(5) * rawSqr;
                outputs[SQUARE_OUTPUT].setVoltageSimd(sqrWave, baseChannel);
            }

            if (outputSin) {
                const static float twoPi = 2 * 3.141592653589793238;
                float_4 sinWave = float_4(5.f) * sinTwoPi( phaseAccumulators[bank] * twoPi);
                outputs[SINE_OUTPUT].setVoltageSimd(sinWave, baseChannel);
            }

            if (outputTri) {
                // generate triangle wave based on saw wave
                float_4 saw = (phaseAccumulators[bank] - 0.5)*2;
                float_4 triangle = (abs(saw) * 2) - 1;

                // send to output
                outputs[TRIANGLE_OUTPUT].setVoltageSimd(triangle * float_4(5), bank);
            }
        }
    }

};


struct VCOWidget : ModuleWidget {
	VCOWidget(VCO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-VCO.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.326, 23.092)), module, VCO::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.578, 23.092)), module, VCO::PULSE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.326, 71.511)), module, VCO::FMPARAM_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.578, 71.511)), module, VCO::PULSEMODPARAM_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 71.511)), module, VCO::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.326, 89.767)), module, VCO::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 89.767)), module, VCO::SYNC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48.578, 89.767)), module, VCO::PULSEMOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.326, 108.95)), module, VCO::SINE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.405, 108.95)), module, VCO::TRIANGLE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.484, 108.95)), module, VCO::SAW_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.578, 108.95)), module, VCO::SQUARE_OUTPUT));
	}
};


Model* modelVCO = createModel<VCO, VCOWidget>("VCO");