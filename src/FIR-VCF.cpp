#include "plugin.hpp"
#include "filters.hpp"


struct GL_FIR_VCF : Module {
	enum ParamId {
		CUT_PARAM,
		RES_PARAM,
		DRIVE_PARAM,
		RESMOD_PARAM,
		CUTMOD_PARAM,
		DRIVEMOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RESMOD_INPUT,
		CUTMOD_INPUT,
		DRIVEMOD_INPUT,
		IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LP_OUTPUT,
		BP_OUTPUT,
		HP_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

    VariableCutoffFIRFilter<64> filter;

    GL_FIR_VCF() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(CUT_PARAM, 0.001f, 20000.f, 1000.f, "Cutoff frequency", "Hz");
        configParam(RES_PARAM, 0.f, 1.f, 0.f, "Resonance");
        configParam(DRIVE_PARAM, 0.f, 1.f, 0.f, "Drive");
        configParam(RESMOD_PARAM, -1.f, 1.f, 0.f, "Resonance modulation", "%", 0.f, 100.f);
        configParam(CUTMOD_PARAM, -1.f, 1.f, 0.f, "Cutoff frequency modulation", "%", 0.f, 100.f);
        configParam(DRIVEMOD_PARAM, -1.f, 1.f, 0.f, "Drive modulation", "%", 0.f, 100.f);
        configInput(RESMOD_INPUT, "Resonance modulation");
        configInput(CUTMOD_INPUT, "Cutoff frequency modulation");
        configInput(DRIVEMOD_INPUT, "Drive modulation");
        configInput(IN_INPUT, "Input");
        configOutput(LP_OUTPUT, "Lowpass");
        configOutput(BP_OUTPUT, "Bandpass");
        configOutput(HP_OUTPUT, "Highpass");
    }


	void process(const ProcessArgs& args) override {
        // if nothing is connected, do nothing (performance)
        if (!outputs[LP_OUTPUT].isConnected() && !outputs[BP_OUTPUT].isConnected() && !outputs[HP_OUTPUT].isConnected()){
            return;
        }

        float input = inputs[IN_INPUT].getVoltage();
        float cutoff_param = params[CUT_PARAM].getValue();
        float cutoff_mod_param = params[CUTMOD_PARAM].getValue();
        float cutoff_mod_cv = inputs[CUTMOD_INPUT].getVoltage();
        float cutoff = cutoff_param + cutoff_mod_param*cutoff_mod_cv;
        if (outputs[LP_OUTPUT].isConnected()){
            filter.setLowPass(cutoff, args.sampleRate);
            float out = filter.process(input);
            outputs[LP_OUTPUT].setVoltage(out);
        }
	}
};


struct GL_FIR_VCFWidget : ModuleWidget {
	GL_FIR_VCFWidget(GL_FIR_VCF* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-VCF.svg")));

        // add screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(30.48, 20.71)), module, GL_FIR_VCF::CUT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.678, 42.935)), module, GL_FIR_VCF::RES_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.664, 42.935)), module, GL_FIR_VCF::DRIVE_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(11.296, 74.914)), module, GL_FIR_VCF::CUTMOD_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(49.664, 74.914)), module, GL_FIR_VCF::DRIVEMOD_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.296, 91.547)), module, GL_FIR_VCF::RESMOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.508, 91.547)), module, GL_FIR_VCF::CUTMOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.664, 91.547)), module, GL_FIR_VCF::DRIVEMOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.296, 108.874)), module, GL_FIR_VCF::IN_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.409, 108.95)), module, GL_FIR_VCF::LP_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.488, 108.95)), module, GL_FIR_VCF::BP_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.582, 108.954)), module, GL_FIR_VCF::HP_OUTPUT));
	}
};


Model* modelGL_FIR_VCF = createModel<GL_FIR_VCF, GL_FIR_VCFWidget>("GL-FIR-VCF");