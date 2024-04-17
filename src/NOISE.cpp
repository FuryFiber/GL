#include "plugin.hpp"

const float g_fScale = 2.0f / 0xffffffff;



struct NOISE : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		RED_OUTPUT,
		WHITE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
    int g_x1 = 0x67452301;
    int g_x2 = 0xefcdab89;
    float white_buffer[128];
    float red_buffer[128];
    int counter = 0;

	NOISE() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(RED_OUTPUT, "");
		configOutput(WHITE_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
        counter = (counter+1) % 128;
        if (counter == 0) {
            fill_buffer();
        }

        outputs[WHITE_OUTPUT].setVoltage(white_buffer[counter] * 10);

	}

    void whitenoise(
            float* _fpDstBuffer, // Pointer to buffer
            unsigned int _uiBufferSize = 128, // Size of buffer
            float _fLevel = 1.f ) // Noiselevel (0.0 ... 1.0)
    {
        _fLevel *= g_fScale;

        while( _uiBufferSize-- )
        {
            g_x1 ^= g_x2;
            *_fpDstBuffer++ = g_x2 * _fLevel;
            g_x2 += g_x1;
        }
    }

    void fill_buffer() {
        whitenoise(white_buffer);
    }
};


struct NOISEWidget : ModuleWidget {
	NOISEWidget(NOISE* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-NOISE.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH - 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 10, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 94.476)), module, NOISE::RED_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 108.95)), module, NOISE::WHITE_OUTPUT));
	}
};


Model* modelNOISE = createModel<NOISE, NOISEWidget>("NOISE");