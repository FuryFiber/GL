#include "plugin.hpp"
#include "filters.hpp"


/*
 * Voltage controlled filter module implementation.
 * Provides both IIR and FIR filtering.
 * IIR: 3 layer cascading butterworth filter using 3 biquad IIR filters.
 * FIR: Basic FIR with window-sinc method using Hamming window function.
 *
 * All output signals have their own filter so that they can be used at the same time.
 */
struct VCF : Module {
	enum ParamId {
		CUT_PARAM,
		GAIN_PARAM,
		BANDWIDTH_PARAM,
        GAINMOD_PARAM,
		CUTMOD_PARAM,
        BANDWIDTHMOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
        GAINMOD_INPUT,
		CUTMOD_INPUT,
        BANDWIDTHMOD_INPUT,
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

    // Filtering mode 0:IIR, 1:FIR
    int mode;

    // Filters
    Cascade6PButterFilter IIR_lowpass_filter;
    Cascade6PButterFilter IIR_bandpass_filter;
    Cascade6PButterFilter IIR_highpass_filter;
    VariableCutoffFIRFilter<64> FIR_lowpass_filter;
    VariableCutoffFIRFilter<64> FIR_bandpass_filter;
    VariableCutoffFIRFilter<64> FIR_highpass_filter;


	VCF() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(CUT_PARAM, 0.001f, 20000.f, 1000.f, "Cutoff frequency", "Hz");
		configParam(GAIN_PARAM, 0.f, 10.f, 0.f, "Resonance gain", "dB");
		configParam(BANDWIDTH_PARAM, 0.f, 1.f, 0.f, "Resonance bandwidth ratio");
		configParam(GAINMOD_PARAM, 0.f, 1.f, 0.f, "Resonance gain modulation", "%", 0.f, 100.f);
		configParam(CUTMOD_PARAM, -1.f, 1.f, 0.f, "Cutoff frequency modulation", "%", 0.f, 100.f);
		configParam(BANDWIDTHMOD_PARAM, 0.f, 1.f, 0.f, "Resonance bandwidth modulation", "%", 0.f, 100.f);
		configInput(GAINMOD_INPUT, "Resonance modulation");
		configInput(CUTMOD_INPUT, "Cutoff frequency modulation");
		configInput(BANDWIDTHMOD_INPUT, "Drive modulation");
		configInput(IN_INPUT, "Input");
		configOutput(LP_OUTPUT, "Lowpass");
		configOutput(BP_OUTPUT, "Bandpass");
		configOutput(HP_OUTPUT, "Highpass");
	}

    /*
     * process a single sample
     */
	void process(const ProcessArgs& args) override {
        // if nothing is connected, do nothing (performance)
        if (!outputs[LP_OUTPUT].isConnected() && !outputs[BP_OUTPUT].isConnected() && !outputs[HP_OUTPUT].isConnected()){
            return;
        }
        if (!inputs[IN_INPUT].isConnected()){
            return;
        }

        // get input
        float input = inputs[IN_INPUT].getVoltage();

        // calculate non-normal cutoff frequency and clamp between 0-20kHz
        float cutoff_param = params[CUT_PARAM].getValue();
        float cutoff_mod_param = params[CUTMOD_PARAM].getValue();
        float cutoff_mod_cv = inputs[CUTMOD_INPUT].getVoltage();
        float cutoff = cutoff_param + cutoff_mod_param*cutoff_mod_cv;
        if (cutoff > args.sampleRate){
            cutoff = args.sampleRate;
        }
        if (cutoff < 0) {
            cutoff = 0;
        }

        // get resonance parameters
        // gain
        float gain_param = params[GAIN_PARAM].getValue();
        float gain_mod_param = params[GAINMOD_PARAM].getValue();
        float gain_mod_in = inputs[GAINMOD_INPUT].getVoltage();
        float G = gain_param + gain_mod_in * gain_mod_param;
        clamp(G, 0.001f, 10.f);
        // bandwidth
        float bw_param = params[BANDWIDTH_PARAM].getValue();
        float bw_mod_param = params[BANDWIDTHMOD_PARAM].getValue();
        float bw_mod_in = inputs[BANDWIDTHMOD_INPUT].getVoltage();
        float Q = bw_param + bw_mod_in*bw_mod_param;
        clamp(Q, 0.001f, 1.f);
        // In case of IIR filtering mode
        if (mode == 0){

            // Normalize cutoff frequency because filter expects value between 0.f 0.5f
            float normalized_cutoff = cutoff/args.sampleRate;

            // Set peak boost at cutoff frequency
            IIR_lowpass_filter.setResonance(normalized_cutoff, G, Q);

            // If low pass output is connected, perform lowpass filtering and send to lowpass output
            if (outputs[LP_OUTPUT].isConnected()){
                IIR_lowpass_filter.setCutoffLow(normalized_cutoff);
                float out = IIR_lowpass_filter.process(input);
                outputs[LP_OUTPUT].setVoltage(out);
            }

            // If bandpass output is connected, perform bandpass filtering and send to bandpass output
            if (outputs[BP_OUTPUT].isConnected()){
                IIR_bandpass_filter.setCutoffBand(normalized_cutoff);
                float out = IIR_bandpass_filter.process(input);
                outputs[BP_OUTPUT].setVoltage(out);
            }

            // If highpass output is connected, perform highpass filtering and send to highpass output
            if (outputs[HP_OUTPUT].isConnected()){
                IIR_highpass_filter.setCutoffHigh(normalized_cutoff);
                float out = IIR_highpass_filter.process(input);
                outputs[HP_OUTPUT].setVoltage(out);
            }
        }

        // In case of FIR filtering mode
        if (mode == 1) {

            // If low pass output is connected, perform lowpass filtering and send to lowpass output
            if (outputs[LP_OUTPUT].isConnected()){
                FIR_lowpass_filter.setLowPass(cutoff, args.sampleRate);
                float out = FIR_lowpass_filter.process(input);
                outputs[LP_OUTPUT].setVoltage(out);
            }

            // If highpass output is connected, perform highpass filtering and send to highpass output
            if (outputs[HP_OUTPUT].isConnected()){
                FIR_highpass_filter.setHighpass(cutoff, args.sampleRate);
                float out = FIR_highpass_filter.process(input);
                outputs[HP_OUTPUT].setVoltage(out);
            }
        }
	}
};


struct VCFWidget : ModuleWidget {
    /*
     * Create module widget
     */
	VCFWidget(VCF* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-VCF.svg")));

        // screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH- 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // parameters
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(30.48, 20.71)), module, VCF::CUT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.678, 42.935)), module, VCF::GAIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.664, 42.935)), module, VCF::BANDWIDTH_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.296, 74.914)), module, VCF::GAINMOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.508, 74.914)), module, VCF::CUTMOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(49.664, 74.914)), module, VCF::BANDWIDTHMOD_PARAM));

        // inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.296, 91.547)), module, VCF::GAINMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.508, 91.547)), module, VCF::CUTMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.664, 91.547)), module, VCF::BANDWIDTHMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.296, 108.874)), module, VCF::IN_INPUT));

        // outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.409, 108.95)), module, VCF::LP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.488, 108.95)), module, VCF::BP_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.582, 108.954)), module, VCF::HP_OUTPUT));
	}

    /*
     * Add filtering mode selection to module context window
     */
    void appendContextMenu(Menu* menu) override {
        VCF* module = getModule<VCF>();

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Filter settings"));
        menu->addChild(createIndexPtrSubmenuItem("Mode", {"IIR", "FIR"}, &module->mode));

    }
};


Model* modelVCF = createModel<VCF, VCFWidget>("VCF");