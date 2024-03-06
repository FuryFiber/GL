#include "plugin.hpp"
#include <math.h>
#include "stdio.h"

const int MAX_POLYPHONY = 16;

struct VCO : Module {
    float phases[MAX_POLYPHONY] = {};
    float normalizedFreqs[MAX_POLYPHONY] = {};
    float pulsewidth = 0.5f;
    int steps = 0;
    int polyphony = 1;
    dsp::MinBlepGenerator<16,16,float> sawMinBleps[MAX_POLYPHONY];
    dsp::MinBlepGenerator<16,16,float> sqrMinBleps[MAX_POLYPHONY];
	enum ParamId {
		PITCH_PARAM,
		PULSE_PARAM,
		FMPARAM_PARAM,
		PULSEMODPARAM_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		SYNC_INPUT,
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

    VCO() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM, 0, 10, 4, "Frequency");
        configParam(PULSE_PARAM, 0.01f, 0.99f, 0.5f, "Pulse width", "%", 0.f, 100.f);
        configParam(FMPARAM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
        configParam(PULSEMODPARAM_PARAM, -1.f, 1.f, 0.f, "Pulse Width modulation", "%", 0.f, 100.f);
        configInput(VOCT_INPUT, "1V/octave pitch");
        configInput(FM_INPUT, "Frequency modulation");
        configInput(SYNC_INPUT, "Sync");
        configInput(PULSEMOD_INPUT, "Pulse width modulation");
        configOutput(SINE_OUTPUT, "Sine");
        configOutput(TRIANGLE_OUTPUT, "Triangle");
        configOutput(SAW_OUTPUT, "Saw");
        configOutput(SQUARE_OUTPUT, "Square");
    }

    /*
     * Update module state every sample rate tick
     * ie generate output signals
     */
    void process(const ProcessArgs& args) override {
        bool sineConnected = outputs[SINE_OUTPUT].isConnected();
        bool squareConnected = outputs[SQUARE_OUTPUT].isConnected();
        bool sawConnected = outputs[SAW_OUTPUT].isConnected();
        bool triangleConnected = outputs[TRIANGLE_OUTPUT].isConnected();
        // execute every 4 steps
        steps = (steps + 1)%4;
        if (steps == 0) {
            process4steps(args);
        }
        // For every polyphony channel compute its outputs
        for (int i = 0; i < polyphony; ++i) {
            // Accumulate the phase
            phases[i] += normalizedFreqs[i];
            if (phases[i] >= 1.f){
                phases[i] -= 1.f;
            }

            // Compute outputs
            if (sineConnected){
                // generate sine wave
                float sine = sin(2.f * M_PI * phases[i]);

                // send to output
                outputs[SINE_OUTPUT].setVoltage(sine * 5.f, i);
            }
            if (squareConnected){
                /* Minblep antialiasing */
                // jump square when crossing 0
                float wrapCrossing = (-(phases[i] - normalizedFreqs[i])) / normalizedFreqs[i];
                bool wrapJump = ((0 < wrapCrossing) & (wrapCrossing <= 1.f));
                if (wrapJump) {
                    float jumpPhase = wrapCrossing - 1.f;
                    float jumpAmount = -2.f;
                    sqrMinBleps[i].insertDiscontinuity(jumpPhase, jumpAmount);
                }

                // jump square when crossing pulse width
                float pulseCrossing = (pulsewidth - (phases[i] - normalizedFreqs[i])) / normalizedFreqs[i];
                bool pulseJump = ((0 < pulseCrossing) & (pulseCrossing <= 1.f));
                if (pulseJump) {
                    float jumpPhase = pulseCrossing - 1.f;
                    float jumpAmount = -2.f;
                    sqrMinBleps[i].insertDiscontinuity(jumpPhase, jumpAmount);
                }

                // process minblep
                float minBlep = sqrMinBleps[i].process();

                /* signal generation */
                float square;
                if (phases[i]<pulsewidth){
                    square = 1.f;
                }
                else {
                    square = -1.f;
                }

                // add minblep
                square += minBlep;

                // send to output
                outputs[SQUARE_OUTPUT].setVoltage(square * 5.f, i);
            }
            if (sawConnected){
                /* MinBlep antialiasing */
                // get crossing point
                float crossing = (0.5f - (phases[i] - normalizedFreqs[i])) / normalizedFreqs[i];

                // decide wether to jump or not
                bool jump = ((0 < crossing) & (crossing <= 1.f));
                if (jump) {
                    // update minblep generator
                    float jumpPhase = crossing - 1.f;
                    float jumpAmount = -2;
                    sawMinBleps[i].insertDiscontinuity(jumpPhase, jumpAmount);
                }

                // process updated minblep generator
                float sawMinBlep = sawMinBleps[i].process();

                /* signal generation */
                // generate saw based on phase
                float saw = (phases[i] - 0.5)*2;

                // add processed minblep to saw signal
                saw += sawMinBlep;

                // send to output
                outputs[SAW_OUTPUT].setVoltage(saw * 5.f, i);
            }
            if (triangleConnected) {
                // generate triangle wave based on saw wave
                float saw = (phases[i] - 0.5)*2;
                float triangle = (abs(saw) * 2) - 1;

                // send to output
                outputs[TRIANGLE_OUTPUT].setVoltage(triangle * 5.f, i);
            }
        }
    }

    /*
     * functionality that only needs to be called every so often to save some performance
     * mostly reading parameters and modulation inputs
     * since modulation inputs are only read every 4 samples, sending audio rate frequencies into these inputs might give wrong result
     */
    void process4steps(const ProcessArgs& args){
        // Get params
        float pulseWidthParam = params[PULSE_PARAM].getValue();
        float pulseModParam = params[PULSEMODPARAM_PARAM].getValue();
        float pitch = params[PITCH_PARAM].getValue();
        float fmParam = params[FMPARAM_PARAM].getValue();

        // Set pulsewidth
        pulsewidth = pulseWidthParam + inputs[PULSEMOD_INPUT].getVoltage() / 10.f * pulseModParam;
        pulsewidth = clamp(pulsewidth, 0.01f, 1.f - 0.01f);
        // Tell each output how many polyphony channels are used
        polyphony = std::max(1, inputs[VOCT_INPUT].getChannels());
        outputs[SINE_OUTPUT].setChannels(polyphony);
        outputs[TRIANGLE_OUTPUT].setChannels(polyphony);
        outputs[SAW_OUTPUT].setChannels(polyphony);
        outputs[SQUARE_OUTPUT].setChannels(polyphony);

        // for each polyphony channel compute its frequency
        for (int i = 0; i < polyphony; ++i) {
            float pitchCV = inputs[VOCT_INPUT].getVoltage(i);
            float combinedpitch = pitch + pitchCV - 4.f;
            combinedpitch += inputs[FM_INPUT].getVoltage(i) * fmParam;

            // The default frequency is C4 = 261.6256f so tune pitch to C4
            combinedpitch += float(std::log2(261.626));

            // convert from volts to pitch
            float freq = dsp::exp2_taylor5(combinedpitch);

            // normalize
            normalizedFreqs[i] = args.sampleTime * freq;
        }
    }
};


struct VCOWidget : ModuleWidget {
	VCOWidget(VCO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-VCO.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.326, 28.913)), module, VCO::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.578, 28.913)), module, VCO::PULSE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(12.326, 71.511)), module, VCO::FMPARAM_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(48.578, 71.511)), module, VCO::PULSEMODPARAM_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 71.511)), module, VCO::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.326, 89.767)), module, VCO::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48, 89.767)), module, VCO::SYNC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48.578, 89.767)), module, VCO::PULSEMOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.326, 108.95)), module, VCO::SINE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.405, 108.95)), module, VCO::TRIANGLE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.484, 108.95)), module, VCO::SAW_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.578, 108.95)), module, VCO::SQUARE_OUTPUT));
	}
};


Model* modelVCO = createModel<VCO, VCOWidget>("VCO");