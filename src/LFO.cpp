#include "plugin.hpp"
#include <math.h>
#include "stdio.h"

using float_4 = simd::float_4;

/*
 * Simd sine approximation
 */
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

/*
 * Low frequency oscillator module implementation.
 * Provides, sine, square, saw and triangle waves from 0-1024Hz
 */
struct LFO : Module {
	enum ParamId {
		FREQ_PARAM,
		PULSE_PARAM,
		FM_PARAM,
		OFST_PARAM,
		PULSEMOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FM_INPUT,
		RESET_INPUT,
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

    float_4 phaseAccumulator = 0;
    float_4 phaseAdvance = 0;

    float pulsewidth = 1.f;
    float offset = 0.f;
    int currentBanks = 1;
    int loopCounter = 0;
    bool outputSaw = false;
    bool outputSin = false;
    bool outputSqr = false;
    bool outputTri = false;


	LFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, -8.f, 6.f, 1.f, "Frequency", "HZ", 2, 16);
		configParam(PULSE_PARAM, 0.f, 1.f, 0.5f, "Pulsewidth", "%", 0.f, 100.f);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		configSwitch(OFST_PARAM, 0.f, 1.f, 0.f, "Offset");
		configParam(PULSEMOD_PARAM, -1.f, 1.f, 0.f, "Pulsewidth modulation", "%", 0.f, 100.f);
		configInput(FM_INPUT, "Frequency modulation");
		configInput(RESET_INPUT, "Reset");
		configInput(PULSEMOD_INPUT, "Pulsewidth modulation");
		configOutput(SINE_OUTPUT, "Sine output");
		configOutput(TRIANGLE_OUTPUT, "Triangle output");
		configOutput(SAW_OUTPUT, "Saw output");
		configOutput(SQUARE_OUTPUT, "Square output");
	}

    /*
     * Process a single timestep
     */
	void process(const ProcessArgs& args) override {
        if (loopCounter-- == 0) {
            loopCounter = 3;
            processEvery4Samples(args);
        }

        generateOutput();
	}

    /*
     * Process called only every 4 timesteps for performance optimization reasons.
     * Mostly consists of updating parameters and calculating output frequency
     */
    void processEvery4Samples(const ProcessArgs& args) {
        outputSaw = outputs[SAW_OUTPUT].isConnected();
        outputSin = outputs[SINE_OUTPUT].isConnected();
        outputSqr = outputs[SQUARE_OUTPUT].isConnected();
        outputTri = outputs[TRIANGLE_OUTPUT].isConnected();

        float pulseWidthParam = params[PULSE_PARAM].getValue();
        float pulseModParam = params[PULSEMOD_PARAM].getValue();

        // Set offset
        if (params[OFST_PARAM].getValue() > 0.f) {
            offset = 5.f;
        }
        else {
            offset = 0.f;
        }

        // Set pulsewidth
        pulsewidth = pulseWidthParam + inputs[PULSEMOD_INPUT].getVoltage() / 10.f * pulseModParam;
        pulsewidth = clamp(pulsewidth, 0.01f, 1.f - 0.01f);

        // Note that assigning a float to a float_4 silently copies the float into all
        // four floats in the float_4.
        float_4 pitchParam = params[FREQ_PARAM].value;
        float fmParam = params[FM_PARAM].getValue();
        const int currentChannel = 1;

        float_4 combinedPitch = pitchParam - float_4(4.f);

        const float_4 q = float(std::log2(261.626));       // move up to C
        combinedPitch += q;
        combinedPitch += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(currentChannel) * fmParam;


        const float_4 freq = rack::dsp::approxExp2_taylor5<float_4>(combinedPitch);

        const float_4 normalizedFreq = float_4(args.sampleTime) * freq;
        phaseAdvance = normalizedFreq;

    }

    /*
     * Actually generate output signal
     */
    void generateOutput() {
        // advance phase and wrap
        phaseAccumulator += phaseAdvance;
        phaseAccumulator -= simd::floor(phaseAccumulator);
        if (inputs[RESET_INPUT].getVoltage() > 0.f) {
            phaseAccumulator = 0;
        }

        if (outputSaw) {
            // generate raw saw signal
            float_4 rawSaw = phaseAccumulator + float_4(.5f);
            rawSaw -= simd::trunc(rawSaw);
            rawSaw = 2 * rawSaw - 1;

            // transform from -1v / 1v to -5v / 5v and send to output
            float_4 sawWave = float_4(5) * rawSaw + float_4(offset);
            outputs[SAW_OUTPUT].setVoltageSimd(sawWave, 0);
        }

        if (outputSqr) {
            // generate raw square signal
            float_4 rawSqr = simd::ifelse(phaseAccumulator < pulsewidth, 1.f, -1.f);

            // transform from -1v / 1v to -5v / 5v and send to output
            float_4 sqrWave = float_4(5) * rawSqr + float_4(offset);
            outputs[SQUARE_OUTPUT].setVoltageSimd(sqrWave, 0);
        }

        if (outputSin) {
            const static float twoPi = 2 * 3.141592653589793238;
            float_4 sinWave = float_4(5.f) * sinTwoPi(phaseAccumulator * twoPi) + float_4(offset);
            outputs[SINE_OUTPUT].setVoltageSimd(sinWave, 0);
        }

        if (outputTri) {
            // generate triangle wave based on saw wave
            float_4 saw = (phaseAccumulator - 0.5) * 2;
            float_4 triangle = (abs(saw) * 2) - 1;

            // send to output
            outputs[TRIANGLE_OUTPUT].setVoltageSimd(triangle * float_4(5) + float_4(offset), 0);
        }
    }
};


struct LFOWidget : ModuleWidget {
	LFOWidget(LFO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-LFO.svg")));

        // screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // params
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.326, 23.092)), module, LFO::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.578, 23.092)), module, LFO::PULSE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.326, 71.511)), module, LFO::FM_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(30.48, 71.511)), module, LFO::OFST_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.578, 71.511)), module, LFO::PULSEMOD_PARAM));

        // inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.326, 89.767)), module, LFO::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 89.767)), module, LFO::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48.578, 89.767)), module, LFO::PULSEMOD_INPUT));

        // outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.326, 108.95)), module, LFO::SINE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.405, 108.95)), module, LFO::TRIANGLE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.484, 108.95)), module, LFO::SAW_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.578, 108.95)), module, LFO::SQUARE_OUTPUT));
	}
};


Model* modelLFO = createModel<LFO, LFOWidget>("LFO");