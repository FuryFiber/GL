#include "plugin.hpp"


struct SpringMod : Module {
	enum ParamId {
		MASS_PARAM,
		SPRING_PARAM,
		VISC_PARAM,
		MASSMOD_PARAM,
		SPRINGMOD_PARAM,
		VISCMOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MASSMOD_INPUT,
		SPRINGMOD_INPUT,
		VISCMOD_INPUT,
		TRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SpringMod() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MASS_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SPRING_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VISC_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MASSMOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SPRINGMOD_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VISCMOD_PARAM, 0.f, 1.f, 0.f, "");
		configInput(MASSMOD_INPUT, "");
		configInput(SPRINGMOD_INPUT, "");
		configInput(VISCMOD_INPUT, "");
		configInput(TRIG_INPUT, "");
		configOutput(OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct SpringModWidget : ModuleWidget {
	SpringModWidget(SpringMod* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-SpringMod.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.736, 51.931)), module, SpringMod::MASS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.89, 51.931)), module, SpringMod::SPRING_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.988, 51.931)), module, SpringMod::VISC_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.354, 78.618)), module, SpringMod::MASSMOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.508, 78.618)), module, SpringMod::SPRINGMOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.606, 78.618)), module, SpringMod::VISCMOD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.354, 95.251)), module, SpringMod::MASSMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.508, 95.251)), module, SpringMod::SPRINGMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48.606, 95.251)), module, SpringMod::VISCMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.634, 108.95)), module, SpringMod::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.732, 108.95)), module, SpringMod::OUT_OUTPUT));
	}
};


Model* modelSpringMod = createModel<SpringMod, SpringModWidget>("SpringMod");