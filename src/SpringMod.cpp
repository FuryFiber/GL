#include "plugin.hpp"
#include "stdio.h"
#include "cmath"

struct SpringMod : Module {
    float t = 0.f;
    float A0 = 10.f;
	enum ParamId {
		MASS_PARAM,
		VISC_PARAM,
		SPRING_PARAM,
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
		configParam(MASS_PARAM, 0.1f, 100.f, 0.f, "Mass", " kg");
		configParam(VISC_PARAM, 0.1f, 20.f, 0.f, "Viscosity", " mPas");
		configParam(SPRING_PARAM, 0.1f, 20.f, 0.f, "Spring constant", "kN/m");
		configParam(MASSMOD_PARAM, -1.f, 1.f, 0.f, "Mass modulation", "%", 0.f, 100.f);
		configParam(SPRINGMOD_PARAM, -1.f, 1.f, 0.f, "Spring constant modulation", "%", 0.f, 100.f);
		configParam(VISCMOD_PARAM, -1.f, 1.f, 0.f, "Viscosity modulation" ,"%", 0.f, 100.f);
		configInput(MASSMOD_INPUT, "Mass modulation");
		configInput(SPRINGMOD_INPUT, "Spring constant modulation");
		configInput(VISCMOD_INPUT, "Viscosity modulation");
		configInput(TRIG_INPUT, "Trigger");
		configOutput(OUT_OUTPUT, "Output");
	}

	void process(const ProcessArgs& args) override {
        // get inputs
        float mass = params[MASS_PARAM].getValue() + inputs[MASSMOD_INPUT].getVoltage() * params[MASSMOD_PARAM].getValue();
        float viscosity = params[VISC_PARAM].getValue() + inputs[VISCMOD_INPUT].getVoltage() * params[VISCMOD_PARAM].getValue();
        float spring_constant = (params[SPRING_PARAM].getValue() + inputs[SPRINGMOD_INPUT].getVoltage() * params[SPRINGMOD_PARAM].getValue()) * 1000;
        t += args.sampleTime;

        // reset phase if trigger is on
        if (inputs[TRIG_INPUT].getVoltage() > 0.f) {
            t = 0;
        }

        // calculate output
        float output = A0 * exp((-viscosity * t)/(2 * mass)) * cos(sqrt((spring_constant / mass) - pow((viscosity/2 * mass), 2)) * t);

        // send to output
        outputs[OUT_OUTPUT].setVoltage(output);
    }
};


struct SpringModWidget : ModuleWidget {
	SpringModWidget(SpringMod* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-SpringMod.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.678, 27.06)), module, SpringMod::MASS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.046, 27.06)), module, SpringMod::VISC_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48, 52.461)), module, SpringMod::SPRING_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.296, 78.618)), module, SpringMod::MASSMOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.508, 78.618)), module, SpringMod::SPRINGMOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.664, 78.618)), module, SpringMod::VISCMOD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.296, 95.251)), module, SpringMod::MASSMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.508, 95.251)), module, SpringMod::SPRINGMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.664, 95.251)), module, SpringMod::VISCMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.634, 108.95)), module, SpringMod::TRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.732, 108.95)), module, SpringMod::OUT_OUTPUT));
	}
};


Model* modelSpringMod = createModel<SpringMod, SpringModWidget>("SpringMod");