#include "plugin.hpp"
#include <algorithm>
#include "stdio.h"

static const int maxPolyphony = 16;


struct WAVECRAFTER : Module {
	enum ParamId {
        FREQ_PARAM,
		FMPARAM_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

    std::vector<float> buffer;

    bool enableEditing = true;
    int nchannels = 1;
    float phaseAccumulators[maxPolyphony] = {};
    float phaseAdvance[maxPolyphony] = {};
    int loopcounter = 0;

    void initBuffer() {
        buffer.clear();
        int default_steps = 1000;
        for(int i = 0; i < default_steps; i++) {
            buffer.push_back(i / (default_steps - 1.f));
        }
    }

	WAVECRAFTER() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(FREQ_PARAM, 0.f, 10.f, 4.f, "");
		configParam(FMPARAM_PARAM, 0.f, 1.f, 0.f, "");
		configInput(VOCT_INPUT, "");
		configInput(FM_INPUT, "");
		configOutput(OUTPUT, "");
        initBuffer();
	}

    size_t numFadeSamples() {
        // Calculate the clicking prevention fade size (in samples)
        // based on the current buffer size.
        size_t n = buffer.size();
        if(n < 5) {
            return 0;
        } else {
            return std::min<size_t>(n / 100 + 2, 200);
        }
    }

    void onReset() override {
        enableEditing = true;
        initBuffer();
    }

	void process(const ProcessArgs& args) override {
        if (loopcounter-- == 0) {
            loopcounter = 3;
            process4Samples(args);
        }

        generateOutput();
	}

    void process4Samples(const ProcessArgs& args){

        nchannels = std::max(1, inputs[VOCT_INPUT].getChannels());
        outputs[OUTPUT].setChannels(nchannels);
        float pitchParam = params[FREQ_PARAM].value;
        for (int i = 0; i < nchannels; ++i) {
            float pitchCV = inputs[VOCT_INPUT].getVoltage(i);
            float combinedPitch = pitchParam + pitchCV - 4.f;

            const float q = float(std::log2(261.626));       // move up to C
            combinedPitch += q;

            // Combined pitch is in volts. Now use an exponential function
            // to convert that to a pitch.
            // This time we use the fast exp approximation from the VCV SDK.
            // NEW 4 VCO2
            const float freq = rack::dsp::approxExp2_taylor5(combinedPitch);

            // figure out how much to add to our ramp every cycle
            // to make a saw at the desired frequency.
            // restrict the range to something reasonable to avoid bugs.
            float normalizedFreq = args.sampleTime * freq;
            math::clamp(normalizedFreq, 1e-6f, 0.35f);
            phaseAdvance[i] = normalizedFreq;
        }
    }

    void generateOutput() {

        int size = buffer.size();
        float inOutMin = -5;
        float inOutMax = 5;
        for(int chan = 0; chan < nchannels; chan++) {
            phaseAccumulators[chan] += phaseAdvance[chan];
            if (phaseAccumulators[chan] > 1.f) {
                // We limit our phase to the range 0..1
                // Note that this code assumes phaseAccumulators[i] < 1.
                // We have clamped the values already, so we know this is true.
                phaseAccumulators[chan] -= 1.f;
            }
            float phase = phaseAccumulators[chan];

            // interpolated output, based on tabread4_tilde_perform() in
            // https://github.com/pure-data/pure-data/blob/master/src/d_array.c
            //TODO: adjust symmetry of surrounding indices (based on range polarity)?
            int i = clamp((int) std::floor(phase * size), 0, size - 1);
            int ia, ib, ic, id;
            ia = clamp(i - 1, 0, size - 1);
            ib = clamp(i + 0, 0, size - 1);
            ic = clamp(i + 1, 0, size - 1);
            id = clamp(i + 2, 0, size - 1);
            float a = buffer[ia];
            float b = buffer[ib];
            float c = buffer[ic];
            float d = buffer[id];

            // Pd algorithm magic
            float frac = phase * size - i; // fractional part of phase
            float y = b + frac * (
                    c - b - 0.1666667f * (1.f - frac) * (
                            (d - a - 3.f * (c - b)) * frac + (d + 2.f * a - 3.f * b)
                    )
            );
            outputs[OUTPUT].setVoltage(rescale(y, 0.f, 1.f, inOutMin, inOutMax), chan);
        }
    }
};

struct Display : OpaqueWidget {
    WAVECRAFTER* module;
    Vec dragPosition;
    bool dragging = false;

    Display(WAVECRAFTER* module): OpaqueWidget() {
        this->module = module;
        box.size = Vec(350, 180);
    }

    void draw(const DrawArgs &args) override {
        OpaqueWidget::draw(args);
        const auto vg = args.vg;

        if(module) {

            // draw the array contents
            int s = module->buffer.size();
            float w = box.size.x * 1.f / s;
            nvgBeginPath(vg);
            if(s < box.size.x) {
                for(int i = 0; i < s; i++) {
                    float x1 = i * w;
                    float x2 = (i + 1) * w;
                    float y = (1.f - module->buffer[i]) * box.size.y;

                    if(i == 0) nvgMoveTo(vg, x1, y);
                    else nvgLineTo(vg, x1, y);

                    nvgLineTo(vg, x2, y);
                }
            } else {
                for(int i = 0; i < box.size.x; i++) {
                    //int i1 = clamp(int(rescale(i, 0, box.size.x - 1, 0, s - 1)), 0, s - 1);
                    // just use the left edge (should really use average over i1..i2 instead...
                    int ii = clamp(int(rescale(i, 0, box.size.x - 1, 0, s - 1)), 0, s - 1);
                    float y = (1.f - module->buffer[ii]) * box.size.y;
                    if(i == 0) nvgMoveTo(vg, 0, y);
                    else nvgLineTo(vg, i, y);
                }
            }
            nvgStrokeWidth(vg, 1.f);
            nvgStrokeColor(vg, nvgRGB(255, 255, 255));
            nvgStroke(vg);

        }

        // draw 0 line
        nvgBeginPath(vg);
        nvgStrokeColor(vg, nvgRGB(150, 150, 150));
        nvgStrokeWidth(vg, 1.f);
        nvgRect(vg, 0, 0, box.size.x, box.size.y/2);
        nvgStroke(vg);

        // draw outer rectangle
        nvgBeginPath(vg);
        nvgStrokeColor(vg, nvgRGB(255, 255, 255));
        nvgStrokeWidth(vg, 1.f);
        nvgRect(vg, 0, 0, box.size.x, box.size.y);
        nvgStroke(vg);

    }


    void onButton(const event::Button &e) override {
        bool ctrl = (APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL;
        if(e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS
           && module->enableEditing && !ctrl) {
            e.consume(this);
            dragPosition = e.pos;
        }
    }

    void onDragStart(const event::DragStart &e) override {
        OpaqueWidget::onDragStart(e);
        dragging = true;
    }

    void onDragEnd(const event::DragEnd &e) override {
        OpaqueWidget::onDragEnd(e);
        dragging = false;
    }

    void onDragMove(const event::DragMove &e) override {
        OpaqueWidget::onDragMove(e);
        if(!module->enableEditing) return;
        Vec dragPosition_old = dragPosition;
        float zoom = getAbsoluteZoom();
        dragPosition = dragPosition.plus(e.mouseDelta.div(zoom)); // take zoom into account

        // int() rounds down, so the upper limit of rescale is buffer.size() without -1.
        int s = module->buffer.size();
        math::Vec bs = box.size;
        int i1 = clamp(int(rescale(dragPosition_old.x, 0, bs.x, 0, s)), 0, s - 1);
        int i2 = clamp(int(rescale(dragPosition.x,     0, bs.x, 0, s)), 0, s - 1);

        if(abs(i1 - i2) < 2) {
            float y = clamp(rescale(dragPosition.y, 0, bs.y, 1.f, 0.f), 0.f, 1.f);
            module->buffer[i2] = y;
        } else {
            // mouse moved more than one index, interpolate
            float y1 = clamp(rescale(dragPosition_old.y, 0, bs.y, 1.f, 0.f), 0.f, 1.f);
            float y2 = clamp(rescale(dragPosition.y,     0, bs.y, 1.f, 0.f), 0.f, 1.f);
            if(i2 < i1) {
                std::swap(i1, i2);
                std::swap(y1, y2);
            }
            for(int i = i1; i <= i2; i++) {
                float y = y1 + rescale(i, i1, i2, 0.f, 1.0f) * (y2 - y1);
                module->buffer[i] = y;
            }
        }
    }

    void step() override {
        OpaqueWidget::step();
    }
};


struct WAVECRAFTERWidget : ModuleWidget {
    Display* display;

	WAVECRAFTERWidget(WAVECRAFTER* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GL-WAVECRAFTER.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(86.688, 86.158)), module, WAVECRAFTER::FMPARAM_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.96, 86.158)), module, WAVECRAFTER::FREQ_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.536, 86.158)), module, WAVECRAFTER::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(106.708, 86.158)), module, WAVECRAFTER::FM_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60.96, 108.95)), module, WAVECRAFTER::OUTPUT));

        display = new Display(module);
        display->box.pos = Vec(5, 35);
        addChild(display);
	}
};


Model* modelWAVECRAFTER = createModel<WAVECRAFTER, WAVECRAFTERWidget>("WAVECRAFTER");