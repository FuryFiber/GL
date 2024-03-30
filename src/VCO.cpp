#include "plugin.hpp"
#include <math.h>
#include "stdio.h"

using float_4 = simd::float_4;
const int maxPolyphony = 16;
static const int maxBanks = maxPolyphony / 4;

inline float_4 SquinkyLabs_sinTwoPi(float_4 _x) {
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

    float pulsewidth = 0.5f;
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
        /*bool sineConnected = outputs[SINE_OUTPUT].isConnected();
        bool squareConnected = outputs[SQUARE_OUTPUT].isConnected();
        bool sawConnected = outputs[SAW_OUTPUT].isConnected();
        bool triangleConnected = outputs[TRIANGLE_OUTPUT].isConnected();
        // execute every 4 steps
        steps = (steps + 1)%4;
        if (steps == 0) {
            process4steps(args);
        }
        // For every polyphony channel compute its outputs
        for (int i = 0; i < polyphony; ++i) {
            // Accumulate the phase
            phases[i] += normalizedFreqs[i];
            if (phases[i] >= 1.f){
                phases[i] -= 1.f;
            }

            // Compute outputs
            if (sineConnected){
                // generate sine wave
                float sine = sin(2.f * M_PI * phases[i]);

                // send to output
                outputs[SINE_OUTPUT].setVoltage(sine * 5.f, i);
            }
            if (squareConnected){
                *//* Minblep antialiasing *//*
                // jump square when crossing 0
                float wrapCrossing = (-(phases[i] - normalizedFreqs[i])) / normalizedFreqs[i];
                bool wrapJump = ((0 < wrapCrossing) & (wrapCrossing <= 1.f));
                if (wrapJump) {
                    float jumpPhase = wrapCrossing - 1.f;
                    float jumpAmount = -2.f;
                    sqrMinBleps[i].insertDiscontinuity(jumpPhase, jumpAmount);
                }

                // jump square when crossing pulse width
                float pulseCrossing = (pulsewidth - (phases[i] - normalizedFreqs[i])) / normalizedFreqs[i];
                bool pulseJump = ((0 < pulseCrossing) & (pulseCrossing <= 1.f));
                if (pulseJump) {
                    float jumpPhase = pulseCrossing - 1.f;
                    float jumpAmount = -2.f;
                    sqrMinBleps[i].insertDiscontinuity(jumpPhase, jumpAmount);
                }

                // process minblep
                float minBlep = sqrMinBleps[i].process();

                *//* signal generation *//*
                float square;
                if (phases[i]<pulsewidth){
                    square = 1.f;
                }
                else {
                    square = -1.f;
                }

                // add minblep
                //square += minBlep;

                // send to output
                outputs[SQUARE_OUTPUT].setVoltage(square * 5.f, i);
            }
            if (sawConnected){
                *//* MinBlep antialiasing *//*
                // get crossing point
                float crossing = (0.5f - (phases[i] - normalizedFreqs[i])) / normalizedFreqs[i];

                // decide wether to jump or not
                bool jump = ((0 < crossing) & (crossing <= 1.f));
                if (jump) {
                    // update minblep generator
                    float jumpPhase = crossing - 1.f;
                    float jumpAmount = -2;
                    sawMinBleps[i].insertDiscontinuity(jumpPhase, jumpAmount);
                }

                // process updated minblep generator
                float sawMinBlep = sawMinBleps[i].process();

                *//* signal generation *//*
                // generate saw based on phase
                float saw = (phases[i] - 0.5)*2;

                // add processed minblep to saw signal
                //saw += sawMinBlep;

                // send to output
                outputs[SAW_OUTPUT].setVoltage(saw * 5.f, i);
            }
            if (triangleConnected) {
                // generate triangle wave based on saw wave
                float saw = (phases[i] - 0.5)*2;
                float triangle = (abs(saw) * 2) - 1;

                // send to output
                outputs[TRIANGLE_OUTPUT].setVoltage(triangle * 5.f, i);
            }
        }*/
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
        outputs[SINE_OUTPUT].setChannels(currentPolyphony);
        outputs[SAW_OUTPUT].setChannels(currentPolyphony);
        outputs[SQUARE_OUTPUT].setChannels(currentPolyphony);

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

        // Note that assigning a float to a float_4 silently copies the float into all
        // four floats in the float_4.
        float_4 pitchParam = params[PITCH_PARAM].value;
        for (int bank = 0; bank < currentBanks; ++bank) {
            const int currentChannel = bank * 4;

            // This API lets us fetch the CV for four channels at once.
            float_4 pitchCV = inputs[VOCT_INPUT].getPolyVoltageSimd<float_4>(currentChannel);

            // Normal arithmetic operators work transparently with float_4.
            // Sometimes when expressions mix float_4 and float you need
            // to explicitly convert, like we do here with the number f.
            // Other time we do it "just because".
            float_4 combinedPitch = pitchParam + pitchCV - float_4(4.f);

            const float_4 q = float(std::log2(261.626));       // move up to C
            combinedPitch += q;

            // Note that because rack's approxExp2_taylor5 is templatized, it works just
            // the same for a float_4 as it did for float.
            const float_4 freq = rack::dsp::approxExp2_taylor5<float_4>(combinedPitch);

            const float_4 normalizedFreq = float_4(args.sampleTime) * freq;
            phaseAdvance[bank] = normalizedFreq;

            // Now scale the offset linear with frequency.
            dcOffsetCompensation[bank] = normalizedFreq * float_4(sawCorrect);
        }
    }

    /*void proc8ess4steps(const ProcessArgs& args){
        // Get params
        float pulseWidthParam = params[PULSE_PARAM].getValue();
        float pulseModParam = params[PULSEMODPARAM_PARAM].getValue();
        float pitch = params[PITCH_PARAM].getValue();
        float fmParam = params[FMPARAM_PARAM].getValue();

        // Set pulsewidth
        pulsewidth = pulseWidthParam + inputs[PULSEMOD_INPUT].getVoltage() / 10.f * pulseModParam;
        pulsewidth = clamp(pulsewidth, 0.01f, 1.f - 0.01f);
        // Tell each output how many polyphony channels are used
        polyphony = std::max(1, inputs[VOCT_INPUT].getChannels());
        outputs[SINE_OUTPUT].setChannels(polyphony);
        outputs[TRIANGLE_OUTPUT].setChannels(polyphony);
        outputs[SAW_OUTPUT].setChannels(polyphony);
        outputs[SQUARE_OUTPUT].setChannels(polyphony);

        // for each polyphony channel compute its frequency
        for (int i = 0; i < polyphony; ++i) {
            float pitchCV = inputs[VOCT_INPUT].getVoltage(i);
            float combinedpitch = pitch + pitchCV - 4.f;
            combinedpitch += inputs[FM_INPUT].getVoltage(i) * fmParam;

            // The default frequency is C4 = 261.6256f so tune pitch to C4
            combinedpitch += float(std::log2(261.626));

            // convert from volts to pitch
            float freq = dsp::exp2_taylor5(combinedpitch);

            // normalize
            normalizedFreqs[i] = args.sampleTime * freq;
        }
    }*/
    void generateOutput() {
        for (int bank = 0; bank < currentBanks; ++bank) {
            const int baseChannel = bank * 4;
            const int relativeChannel = currentPolyphony - baseChannel;

            // Just as before, be advance the phase and wrap around 0.
            // This time we do four oscillators at once.
            phaseAccumulators[bank] += phaseAdvance[bank];
            phaseAccumulators[bank] -= simd::floor(phaseAccumulators[bank]);

            float_4 SawMinBlepValue;
            if (outputSaw) {

                // Evaluate the phase, and determine if we are at a discontinuity.
                // Determine if the saw "should have" already crossed .5V in the last sample period
                // This is the SIMD version of the minBlep code from VCO2. Well, more properly it's the
                // code from VCV Fundamental VCO-1. I find this algorithm pretty difficult to figure out
                // in SIMD. Thank goodness the Fundamental code is such a rich well to draw from.
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
                float_4 rawSaw = phaseAccumulators[bank] + float_4(.5f);
                rawSaw -= simd::trunc(rawSaw);
                rawSaw = 2 * rawSaw - 1;

                rawSaw += SawMinBlepValue;
                rawSaw += dcOffsetCompensation[bank];       // Add in the offset voltage compensation we observed.
                float_4 sawWave = float_4(5) * rawSaw;
                outputs[SAW_OUTPUT].setVoltageSimd(sawWave, baseChannel);
            }

            if (outputSqr) {
                float_4 rawSqr = simd::ifelse(phaseAccumulators[bank] < pulsewidth, 1.f, -1.f);
                /*rawSqr -= simd::trunc(rawSqr);
                rawSqr = 2 * rawSqr - 1;*/

                rawSqr += sqrMinBlepValue;
                rawSqr += dcOffsetCompensation[bank];
                float_4 sqrWave = float_4(5) * rawSqr;
                outputs[SQUARE_OUTPUT].setVoltageSimd(sqrWave, baseChannel);
            }

            if (outputSin) {
                const static float twoPi = 2 * 3.141592653589793238;
                float_4 sinWave = float_4(5.f) * SquinkyLabs_sinTwoPi( phaseAccumulators[bank] * twoPi);
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

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.326, 28.913)), module, VCO::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.578, 28.913)), module, VCO::PULSE_PARAM));
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