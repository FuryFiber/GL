#include "plugin.hpp"

const int maxPolyphony = 16;


/*
 * ADSR envelope module implementation
 * Provides polyphonic envelope generation.
 */
struct ADSR : Module {
	enum ParamId {
		ATCK_PARAM,
		DEC_PARAM,
		SUS_PARAM,
		REL_PARAM,
		ATCK_MOD_PARAM,
		DEC_MOD_PARAM,
		SUS_MOD_PARAM,
		REL_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ATCK_INPUT,
		DEC_INPUT,
		SUS_INPUT,
		REL_INPUT,
		GATE_INPUT,
		RETR_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ATCK_LIGHT,
		DEC_LIGHT,
		SUS_LIGHT,
		REL_LIGHT,
		LIGHTS_LEN
	};

    bool decaying[maxPolyphony];
    float env[maxPolyphony];
    dsp::SchmittTrigger trigger[maxPolyphony];

	ADSR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ATCK_PARAM, 0.f, 1.f, 0.5f, "Attack");
		configParam(DEC_PARAM, 0.f, 1.f, 0.5f, "Decay");
		configParam(SUS_PARAM, 0.f, 1.f, 0.5f, "Sustain");
		configParam(REL_PARAM, 0.f, 1.f, 0.5f, "Release");
		configParam(ATCK_MOD_PARAM, -1.f, 1.f, 0.f, "Attack modulation", "%", 0.f, 100.f);
		configParam(DEC_MOD_PARAM, -1.f, 1.f, 0.f, "Decay modulation", "%", 0.f, 100.f);
		configParam(SUS_MOD_PARAM, -1.f, 1.f, 0.f, "Sustain modulation", "%", 0.f, 100.f);
		configParam(REL_MOD_PARAM, -1.f, 1.f, 0.f, "Release modulation", "%", 0.f, 100.f);
		configInput(ATCK_INPUT, "Attack modulation");
		configInput(DEC_INPUT, "Decay modulation");
		configInput(SUS_INPUT, "Sustain modulation");
		configInput(REL_INPUT, "Release modulation");
		configInput(GATE_INPUT, "Gate");
		configInput(RETR_INPUT, "Retrigger");
		configOutput(OUT_OUTPUT, "Envelope");
	}

    /*
     * Process a single timestep
     */
	void process(const ProcessArgs& args) override {

        // Calculate parameter values
        float attack = clamp(params[ATCK_PARAM].getValue() + inputs[ATCK_INPUT].getVoltage()*params[ATCK_MOD_PARAM].getValue() / 10.0f, 0.0f, 1.0f);
        float decay = clamp(params[DEC_PARAM].getValue() + inputs[DEC_INPUT].getVoltage()*params[DEC_MOD_PARAM].getValue() / 10.0f, 0.0f, 1.0f);
        float sustain = clamp(params[SUS_PARAM].getValue() + inputs[SUS_INPUT].getVoltage()*params[SUS_MOD_PARAM].getValue() / 10.0f, 0.0f, 1.0f);
        float release = clamp(params[REL_PARAM].getValue() + inputs[REL_INPUT].getVoltage()*params[REL_MOD_PARAM].getValue() / 10.0f, 0.0f, 1.0f);

        // Get gate and trigger
        int channels = inputs[GATE_INPUT].getChannels();
        outputs[OUT_OUTPUT].setChannels(channels);

        // For each used polyphony channel
        for (int i = 0; i<channels; i++) {

            // Check if gate is open
            bool gated = inputs[GATE_INPUT].getVoltage(i) >= 1.0f;

            // Check if triggered
            if (trigger[i].process(inputs[RETR_INPUT].getVoltage(i)))
                decaying[i] = false;

            const float base = 20000.0f;
            const float maxTime = 10.0f;

            // if gate is open, generate envelope signal
            if (gated) {
                if (decaying[i]) {
                    // Decay
                    if (decay < 1e-4) {
                        env[i] = sustain;
                    } else {
                        env[i] += powf(base, 1 - decay) / maxTime * (sustain - env[i]) / args.sampleRate;
                    }
                } else {
                    // Attack
                    // Skip ahead if attack is all the way down (infinitely fast)
                    if (attack < 1e-4) {
                        env[i] = 1.0f;
                    } else {
                        env[i] += powf(base, 1 - attack) / maxTime * (1.01 - env[i]) / args.sampleRate;
                    }
                    if (env[i] >= 1.0f) {
                        env[i] = 1.0f;
                        decaying[i] = true;
                    }
                }
            } else {
                // Release
                if (release < 1e-4) {
                    env[i] = 0.0f;
                } else {
                    env[i] += powf(base, 1 - release) / maxTime * (0.0 - env[i]) / args.sampleRate;
                }
                decaying[i] = false;
            }

            bool sustaining = isNear(env[i], sustain, 1e-3);
            bool resting = isNear(env[i], 0.0, 1e-3);

            outputs[OUT_OUTPUT].setVoltage(10.0f * env[i], i);

            // Lights
            lights[ATCK_LIGHT].value = (gated && !decaying) ? 1.0f : 0.0f;
            lights[DEC_LIGHT].value = (gated && decaying && !sustaining) ? 1.0f : 0.0f;
            lights[SUS_LIGHT].value = (gated && decaying && sustaining) ? 1.0f : 0.0f;
            lights[REL_LIGHT].value = (!gated && !resting) ? 1.0f : 0.0f;
        }
	}
};


struct ADSRWidget : ModuleWidget {
    /*
     * Create module widget
     */
	ADSRWidget(ADSR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-ADSR.svg")));

        // screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // params
        addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(6.834, 40.742)), module, ADSR::ATCK_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(17.506, 40.742)), module, ADSR::DEC_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(28.179, 40.742)), module, ADSR::SUS_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(38.852, 40.742)), module, ADSR::REL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.834, 71.511)), module, ADSR::ATCK_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.506, 71.511)), module, ADSR::DEC_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(28.179, 71.511)), module, ADSR::SUS_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(38.852, 71.511)), module, ADSR::REL_MOD_PARAM));

        // inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 89.767)), module, ADSR::ATCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.524, 89.767)), module, ADSR::DEC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(28.196, 89.767)), module, ADSR::SUS_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.869, 89.767)), module, ADSR::REL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 108.95)), module, ADSR::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 108.95)), module, ADSR::RETR_INPUT));

        // outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.869, 108.95)), module, ADSR::OUT_OUTPUT));

        // lights
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(6.834, 13.085)), module, ADSR::ATCK_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(17.506, 13.085)), module, ADSR::DEC_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(28.179, 13.085)), module, ADSR::SUS_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(38.852, 13.085)), module, ADSR::REL_LIGHT));
	}
};


Model* modelADSR = createModel<ADSR, ADSRWidget>("ADSR");