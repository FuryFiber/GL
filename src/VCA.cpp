#include "plugin.hpp"


struct VCA : Module {
    int polyphony1 = 1;
    int polyphony2 = 1;
    float amp1 = 0.f;
    float amp2 = 0.f;
    bool mod1Connected = false;
    bool mod2Connected = false;
    int steps = 0;
	enum ParamId {
		AMP1_PARAM,
		AMP2_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MOD1_INPUT,
		MOD2_INPUT,
		IN1_INPUT,
		IN2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
        L1_1_LIGHT,
        L1_2_LIGHT,
        L1_3_LIGHT,
        L1_4_LIGHT,
        L1_5_LIGHT,
        L1_6_LIGHT,
        L1_7_LIGHT,
        L1_8_LIGHT,
        L1_9_LIGHT,
		L1_10_LIGHT,
        L2_1_LIGHT,
        L2_2_LIGHT,
        L2_3_LIGHT,
        L2_4_LIGHT,
        L2_5_LIGHT,
        L2_6_LIGHT,
        L2_7_LIGHT,
        L2_8_LIGHT,
        L2_9_LIGHT,
		L2_10_LIGHT,
		LIGHTS_LEN
	};

	VCA() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(AMP1_PARAM, 0.f, 1.f, 1.f, "Amplitude", "%", 0.f, 100.f);
		configParam(AMP2_PARAM, 0.f, 1.f, 1.f, "Amplitude", "%", 0.f, 100.f);
		configInput(MOD1_INPUT, "cv input");
		configInput(MOD2_INPUT, "cv input");
		configInput(IN1_INPUT, "");
		configInput(IN2_INPUT, "");
		configOutput(OUT1_OUTPUT, "");
		configOutput(OUT2_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
        // only call every 4 ticks to save performance
        steps = (steps + 1)%4;
        if (steps == 0) {
            process4steps(args);
        }

        // if mod1 is not connected use slider to control amplitude
        if (!mod1Connected){
            for (int i = 0; i < polyphony1; i++){
                float out = inputs[IN1_INPUT].getVoltage(i) * amp1;
                outputs[OUT1_OUTPUT].setVoltage(out, i);
            }
            if (steps == 0) {
                setLights(amp1*10, 1);
            }
        }

        // if connected use cv to control amplitude
        else{
            float mod = 0.f;
            for (int i = 0; i < polyphony1; i++){
                mod = (inputs[MOD1_INPUT].getVoltage(i)/10.f);
                float out = inputs[IN1_INPUT].getVoltage(i) * mod;
                outputs[OUT1_OUTPUT].setVoltage(out, i);
            }
            if (steps == 0) {
                setLights(mod*10, 1);
            }
        }

        // same for second channel
        if (!mod2Connected){
            for (int i = 0; i < polyphony2; i++){
                float out = inputs[IN2_INPUT].getVoltage(i) * amp2;
                outputs[OUT2_OUTPUT].setVoltage(out, i);
            }
            if (steps == 0) {
                setLights(amp2*10, 2);
            }
        }
        else{
            float mod = 0.f;
            for (int i = 0; i < polyphony2; i++){
                mod = (inputs[MOD2_INPUT].getVoltage(i)/10.f);
                float out = inputs[IN2_INPUT].getVoltage(i)*mod;
                outputs[OUT2_OUTPUT].setVoltage(out, i);
            }
            if (steps == 0) {
                setLights(mod*10, 2);
            }
        }
	}

    void setLights(float amplitude, int channel) {
        if (channel == 1) {
            for (int i=0; i<LIGHTS_LEN/2; i++){
                if (ceil(amplitude) > i){
                    lights[i].setBrightness(1.f);
                }
                else {
                    lights[i].setBrightness(0.f);
                }
            }
        }
        if (channel == 2) {
            for (int i=10; i<LIGHTS_LEN; i++){
                if (ceil(amplitude) > i-10){
                    lights[i].setBrightness(1.f);
                }
                else {
                    lights[i].setBrightness(0.f);
                }
            }
        }
    }

    void process4steps(const ProcessArgs& args) {
        // get and set polyphony channels
        polyphony1 = inputs[IN1_INPUT].getChannels();
        polyphony2 = inputs[IN2_INPUT].getChannels();
        outputs[OUT1_OUTPUT].setChannels(polyphony1);
        outputs[OUT2_OUTPUT].setChannels(polyphony2);

        // read amplitude param
        amp1 = params[AMP1_PARAM].getValue();
        amp2 = params[AMP2_PARAM].getValue();

        // read if modulation inputs are connected to overwrite the sliders
        mod1Connected = inputs[MOD1_INPUT].isConnected();
        mod2Connected = inputs[MOD2_INPUT].isConnected();
    }
};


struct VCAWidget : ModuleWidget {
	VCAWidget(VCA* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-VCA.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<VCVSlider>(mm2px(Vec(7.567, 41.052)), module, VCA::AMP1_PARAM));
		addParam(createParamCentered<VCVSlider>(mm2px(Vec(22.913, 41.052)), module, VCA::AMP2_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.567, 73.627)), module, VCA::MOD1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.913, 73.627)), module, VCA::MOD2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.567, 91.749)), module, VCA::IN1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.913, 91.749)), module, VCA::IN2_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.567, 106.834)), module, VCA::OUT1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.913, 106.834)), module, VCA::OUT2_OUTPUT));

        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(13.133, 62.706)), module, VCA::L1_1_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(13.133, 57.894)), module, VCA::L1_2_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(13.133, 53.082)), module, VCA::L1_3_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(13.133, 48.27)), module, VCA::L1_4_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(13.133, 43.458)), module, VCA::L1_5_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(13.133, 38.645)), module, VCA::L1_6_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(13.133, 33.833)), module, VCA::L1_7_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(13.133, 29.021)), module, VCA::L1_8_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(13.133, 24.209)), module, VCA::L1_9_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(13.133, 19.397)), module, VCA::L1_10_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(17.347, 62.706)), module, VCA::L2_1_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(17.347, 57.894)), module, VCA::L2_2_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(17.347, 53.082)), module, VCA::L2_3_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(17.347, 48.27)), module, VCA::L2_4_LIGHT));
        addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(17.347, 43.458)), module, VCA::L2_5_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(17.347, 38.645)), module, VCA::L2_6_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(17.347, 33.833)), module, VCA::L2_7_LIGHT));
        addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(17.347, 29.021)), module, VCA::L2_8_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.347, 24.209)), module, VCA::L2_9_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.347, 19.397)), module, VCA::L2_10_LIGHT));
	}
};


Model* modelVCA = createModel<VCA, VCAWidget>("VCA");