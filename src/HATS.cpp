#include "plugin.hpp"


struct HATS : Module {
	enum ParamId {
		CUTOFF_PARAM,
		DECAY_PARAM,
		CUTOFF_MOD_PARAM,
		DECAY_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CUTOFF_INPUT,
		DECAY_INPUT,
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

	HATS() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CUTOFF_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DECAY_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configInput(CUTOFF_INPUT, "");
		configInput(DECAY_INPUT, "");
		configInput(GATE_INPUT, "");
		configInput(RETR_INPUT, "");
		configOutput(OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct HATSWidget : ModuleWidget {
	HATSWidget(HATS* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-HATS.svg")));

        // add screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // add parameter controls
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(6.834, 40.742)), module, HATS::CUTOFF_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(38.852, 40.742)), module, HATS::DECAY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.834, 71.511)), module, HATS::CUTOFF_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(38.852, 71.511)), module, HATS::DECAY_MOD_PARAM));

        // add inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 89.767)), module, HATS::CUTOFF_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.869, 89.767)), module, HATS::DECAY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 108.95)), module, HATS::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 108.95)), module, HATS::RETR_INPUT));

        // add outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.869, 108.95)), module, HATS::OUT_OUTPUT));
	}
};


Model* modelHATS = createModel<HATS, HATSWidget>("HATS");