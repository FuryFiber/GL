#include "plugin.hpp"


struct KICKS : Module {
	enum ParamId {
		FREQ_PARAM,
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

	KICKS() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.f, "");
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


struct KICKSWidget : ModuleWidget {
	KICKSWidget(KICKS* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-KICKS.svg")));

        // add screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // add parameter controls
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.86, 18.472)), module, KICKS::FREQ_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(22.843, 71.511)), module, KICKS::FM_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(38.852, 71.511)), module, KICKS::DECAY_MOD_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(6.834, 40.742)), module, KICKS::CUTOFF_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(38.852, 40.742)), module, KICKS::DECAY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.834, 71.511)), module, KICKS::CUTOFF_MOD_PARAM));

        // add inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 89.767)), module, KICKS::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.869, 89.767)), module, KICKS::DECAY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 89.767)), module, KICKS::CUTOFF_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 108.95)), module, KICKS::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 108.95)), module, KICKS::RETR_INPUT));

        // add outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.869, 108.95)), module, KICKS::OUT_OUTPUT));
	}
};


Model* modelKICKS = createModel<KICKS, KICKSWidget>("KICKS");