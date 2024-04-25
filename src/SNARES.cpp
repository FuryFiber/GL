#include "plugin.hpp"


struct SNARES : Module {
	enum ParamId {
		FREQ_PARAM,
		NOISE_PARAM,
		FM_MOD_PARAM,
		DECAY_MOD_PARAM,
		CUTOFF_PARAM,
		DECAY_PARAM,
		CUTOFF_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FM_INPUT,
		DECAY_INPUT,
		CUTOFF_INPUT,
		GATE_INPUT,
		RETR_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SNARES() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.f, "");
		configParam(NOISE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(FM_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DECAY_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CUTOFF_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configInput(FM_INPUT, "");
		configInput(DECAY_INPUT, "");
		configInput(CUTOFF_INPUT, "");
		configInput(GATE_INPUT, "");
		configInput(RETR_INPUT, "");
		configOutput(OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct SNARESWidget : ModuleWidget {
	SNARESWidget(SNARES* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-SNARES.svg")));

        // add screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 18.472)), module, SNARES::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 46.224)), module, SNARES::NOISE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(22.843, 71.511)), module, SNARES::FM_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(38.852, 71.511)), module, SNARES::DECAY_MOD_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(6.834, 40.742)), module, SNARES::CUTOFF_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(38.852, 40.742)), module, SNARES::DECAY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.834, 71.511)), module, SNARES::CUTOFF_MOD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 89.767)), module, SNARES::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.869, 89.767)), module, SNARES::DECAY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 89.767)), module, SNARES::CUTOFF_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 108.95)), module, SNARES::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 108.95)), module, SNARES::RETR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.869, 108.95)), module, SNARES::OUT_OUTPUT));
	}
};


Model* modelSNARES = createModel<SNARES, SNARESWidget>("SNARES");