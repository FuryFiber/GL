#include "plugin.hpp"


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

	LFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.f, "Frequency", "HZ", 0.f, 1024.f);
		configParam(PULSE_PARAM, 0.f, 1.f, 0.5f, "Pulsewidth", "%", 0.f, 100.f);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		configParam(OFST_PARAM, 0.f, 1.f, 0.f, "Offset");
		configParam(PULSEMOD_PARAM, -1.f, 1.f, 0.f, "Pulsewidth modulation", "%", 0.f, 100.f);
		configInput(FM_INPUT, "Frequency modulation");
		configInput(RESET_INPUT, "Reset");
		configInput(PULSEMOD_INPUT, "Pulsewidth modulation");
		configOutput(SINE_OUTPUT, "Sine output");
		configOutput(TRIANGLE_OUTPUT, "Triangle output");
		configOutput(SAW_OUTPUT, "Saw output");
		configOutput(SQUARE_OUTPUT, "Square output");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct LFOWidget : ModuleWidget {
	LFOWidget(LFO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-LFO.svg")));

        // add screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.326, 23.092)), module, LFO::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.578, 23.092)), module, LFO::PULSE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.326, 71.511)), module, LFO::FM_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(30.48, 71.511)), module, LFO::OFST_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.578, 71.511)), module, LFO::PULSEMOD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.326, 89.767)), module, LFO::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 89.767)), module, LFO::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48.578, 89.767)), module, LFO::PULSEMOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.326, 108.95)), module, LFO::SINE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.405, 108.95)), module, LFO::TRIANGLE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.484, 108.95)), module, LFO::SAW_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.578, 108.95)), module, LFO::SQUARE_OUTPUT));
	}
};


Model* modelLFO = createModel<LFO, LFOWidget>("LFO");