#include "plugin.hpp"


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

	ADSR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ATCK_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEC_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SUS_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ATCK_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DEC_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SUS_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REL_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configInput(ATCK_INPUT, "");
		configInput(DEC_INPUT, "");
		configInput(SUS_INPUT, "");
		configInput(REL_INPUT, "");
		configInput(GATE_INPUT, "");
		configInput(RETR_INPUT, "");
		configOutput(OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct ADSRWidget : ModuleWidget {
	ADSRWidget(ADSR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-ADSR.svg")));

        // add screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // param inputs
        addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(6.834, 40.742)), module, ADSR::ATCK_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(17.506, 40.742)), module, ADSR::DEC_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(28.179, 40.742)), module, ADSR::SUS_PARAM));
		addParam(createParamCentered<GL_SlidePot>(mm2px(Vec(38.852, 40.742)), module, ADSR::REL_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.834, 71.511)), module, ADSR::ATCK_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.506, 71.511)), module, ADSR::DEC_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(28.179, 71.511)), module, ADSR::SUS_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(38.852, 71.511)), module, ADSR::REL_MOD_PARAM));

        // Inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 89.767)), module, ADSR::ATCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.524, 89.767)), module, ADSR::DEC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(28.196, 89.767)), module, ADSR::SUS_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.869, 89.767)), module, ADSR::REL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.851, 108.95)), module, ADSR::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.86, 108.95)), module, ADSR::RETR_INPUT));

        // Outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.869, 108.95)), module, ADSR::OUT_OUTPUT));

        // Lights
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(6.834, 13.085)), module, ADSR::ATCK_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(17.506, 13.085)), module, ADSR::DEC_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(28.179, 13.085)), module, ADSR::SUS_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(38.852, 13.085)), module, ADSR::REL_LIGHT));
	}
};


Model* modelADSR = createModel<ADSR, ADSRWidget>("ADSR");