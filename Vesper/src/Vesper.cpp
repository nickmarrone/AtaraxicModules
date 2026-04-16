#include "plugin.hpp"
#include "../../lib/ataraxic_dsp/ataraxic_dsp.hpp"

struct Vesper : Module {
	enum ParamId {
		FREQ_PARAM,
		MORPH_PARAM,
		TIMBRE_PARAM,
		FM_TYPE_PARAM,
		FM_ATTEN_PARAM,
		TIMBRE_ATTEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FM_INPUT,
		TIMBRE_INPUT,
		VOCT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FM_LINEAR_LIGHT,
		FM_TZ_LIGHT,
		FM_LOG_LIGHT,
		LIGHTS_LEN
	};

	ataraxic_dsp::MorphingOscillator osc;
	ataraxic_dsp::SchmittTrigger fmTypeTrigger;
	int fmType = 0;  // 0 = linear, 1 = through-zero, 2 = exponential

	Vesper() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(FREQ_PARAM, -2.f, 2.f, 0.f, "Frequency", " oct");
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(TIMBRE_PARAM, 0.f, 1.f, 0.5f, "Timbre");
		configParam(FM_TYPE_PARAM, 0.f, 1.f, 0.f, "FM Type");
		configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM CV Attenuator");
		configParam(TIMBRE_ATTEN_PARAM, 0.f, 1.f, 0.f, "Timbre CV Attenuator");

		configInput(FM_INPUT, "FM CV");
		configInput(TIMBRE_INPUT, "Timbre CV");
		configInput(VOCT_INPUT, "V/Oct Pitch");

		configOutput(OUT_OUTPUT, "Audio Out");
	}

	void process(const ProcessArgs& args) override {
		// FM type button: cycle on rising edge
		if (fmTypeTrigger.process(params[FM_TYPE_PARAM].getValue()))
			fmType = (fmType + 1) % 3;

		// Pitch: coarse offset (±2 oct) + V/Oct CV
		float freqParam = params[FREQ_PARAM].getValue();
		float voct      = inputs[VOCT_INPUT].getVoltage();
		float baseHz    = 261.626f * std::pow(2.f, freqParam + voct);

		// FM CV: normalize Eurorack ±5V to [-1, 1], scale by attenuator
		float fmCV = inputs[FM_INPUT].getVoltage() / 5.f
		             * params[FM_ATTEN_PARAM].getValue();

		// Morph [0, 1] → [0, 4] (sine→tri→pulse→saw→super saw)
		float morph = params[MORPH_PARAM].getValue() * 4.f;

		// Timbre: knob sets base, CV shifts around it (±0.5 at full attenuation)
		float timbreCV = inputs[TIMBRE_INPUT].getVoltage() / 10.f
		                 * params[TIMBRE_ATTEN_PARAM].getValue();
		float timbre = clamp(params[TIMBRE_PARAM].getValue() + timbreCV, 0.f, 1.f);

		// Compute output based on FM type
		float out;
		if (fmType == 0) {
			float instHz = ataraxic_dsp::fmLinear(baseHz, fmCV, 2000.f);
			out = osc.process(instHz, args.sampleTime, morph, timbre);
		} else if (fmType == 1) {
			float instHz = ataraxic_dsp::fmThroughZero(baseHz, fmCV, 2000.f);
			out = osc.processTZ(instHz, args.sampleTime, morph, timbre);
		} else {
			float instHz = ataraxic_dsp::fmExp(baseHz, fmCV, 1.f);
			out = osc.process(instHz, args.sampleTime, morph, timbre);
		}

		outputs[OUT_OUTPUT].setVoltage(out * 5.f);

		lights[FM_LINEAR_LIGHT].setBrightness(fmType == 0 ? 1.f : 0.f);
		lights[FM_TZ_LIGHT    ].setBrightness(fmType == 1 ? 1.f : 0.f);
		lights[FM_LOG_LIGHT   ].setBrightness(fmType == 2 ? 1.f : 0.f);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "fmType", json_integer(fmType));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* fmTypeJ = json_object_get(rootJ, "fmType");
		if (fmTypeJ)
			fmType = (int)json_integer_value(fmTypeJ);
	}
};

// ---------------------------------------------------------------------------
// Panel layout  —  4 × RACK_GRID_WIDTH = 60.96 mm wide
// cx = 30.48   leftCol = 16   rightCol = 44
//
// TOP HALF  (y = 0 … 190, line at y = 190)
//   Equal 32-unit gaps between content blocks:
//   y = 22   title
//   y = 54   "FREQ" label
//   y = 81   FREQ knob (RoundHugeBlackKnob, r ≈ 19 → top 62, bottom 100)
//   y = 132  "MORPH" / "TIMBRE" labels
//   y = 148  MORPH knob (x=16) / TIMBRE knob (x=44)  (r ≈ 9.5 → bottom 157.5)
//   y = 190  dividing line
//
// BOTTOM HALF  (y = 190 … 381)
//   50-unit sections, 25-unit half-sections at edges:
//   y = 215  FM button (x=16) + LEDs stacked (x=44, at 205/215/225)
//   y = 265  FM ATT trimpot (x=16)  /  TIMBRE ATT trimpot (x=44)
//   y = 315  FM CV input (x=16)     /  TIMBRE CV input (x=44)
//   y = 340  V/Oct input (x=16)     /  OUT output (x=44)
//
// SVG vertical connector lines link each ATT trimpot to its CV jack.
// ---------------------------------------------------------------------------

struct VesperLabels : Widget {
	std::shared_ptr<Font> boldFont;

	void draw(const DrawArgs& args) override {
		std::shared_ptr<Font> font = APP->window->uiFont;
		if (!font) return;

		if (!boldFont) {
			boldFont = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
		}

		auto drawText = [&](float x, float y, const char* text, float size, NVGcolor color,
		                    std::shared_ptr<Font> customFont = nullptr) {
			nvgFontSize(args.vg, size);
			nvgFontFaceId(args.vg, customFont ? customFont->handle : font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
			nvgFillColor(args.vg, color);
			nvgText(args.vg, x, y, text, NULL);
		};

		NVGcolor dark  = nvgRGB(34, 34, 34);
		NVGcolor white = nvgRGB(255, 255, 255);

		float cx       = 30.48f;
		float leftCol  = 16.f;
		float rightCol = 44.f;

		// --- Top half ---
		drawText(cx,       22.f,  "VESPER", 14.f, dark, boldFont);
		drawText(cx,       54.f,  "FREQ",   10.f, dark);
		drawText(leftCol,  132.f, "MORPH",   6.f, dark);
		drawText(rightCol, 132.f, "TIMBRE",  6.f, dark);

		// --- Bottom half ---
		// FM button label (above button)
		drawText(leftCol, 202.f, "FM", 8.f, dark);

		// V/Oct and Out labels (above their jacks)
		drawText(leftCol,  328.f, "V/OCT", 8.f, dark);
		drawText(rightCol, 328.f, "OUT",   8.f, white);
	}
};

struct VesperWidget : ModuleWidget {
	VesperWidget(Vesper* module) {
		setModule(module);

		box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		setPanel(createPanel(asset::plugin(pluginInstance, "res/Vesper.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addChild(createWidget<VesperLabels>(Vec(0, 0)));

		float cx       = box.size.x / 2.f;  // 30.48
		float leftCol  = 16.f;
		float rightCol = 44.f;

		// --- Top half ---

		// FREQ (huge knob, centered)
		addParam(createParamCentered<RoundHugeBlackKnob>(Vec(cx, 81.f), module, Vesper::FREQ_PARAM));

		// MORPH and TIMBRE (medium knobs, side by side)
		addParam(createParamCentered<RoundBlackKnob>(Vec(leftCol,  148.f), module, Vesper::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(rightCol, 148.f), module, Vesper::TIMBRE_PARAM));

		// --- Bottom half ---

		// FM button (left) and mode LEDs stacked vertically (right)
		addParam(createParamCentered<VCVButton>(Vec(leftCol, 215.f), module, Vesper::FM_TYPE_PARAM));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(rightCol, 205.f), module, Vesper::FM_LINEAR_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(rightCol, 215.f), module, Vesper::FM_TZ_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(rightCol, 225.f), module, Vesper::FM_LOG_LIGHT));

		// Attenuator pots (no labels)
		addParam(createParamCentered<Trimpot>(Vec(leftCol,  265.f), module, Vesper::FM_ATTEN_PARAM));
		addParam(createParamCentered<Trimpot>(Vec(rightCol, 265.f), module, Vesper::TIMBRE_ATTEN_PARAM));

		// CV inputs
		addInput(createInputCentered<PJ301MPort>(Vec(leftCol,  315.f), module, Vesper::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(rightCol, 315.f), module, Vesper::TIMBRE_INPUT));

		// Bottom row: V/Oct (left) and Out (right)
		addInput(createInputCentered<PJ301MPort>(Vec(leftCol,  340.f), module, Vesper::VOCT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(rightCol, 340.f), module, Vesper::OUT_OUTPUT));
	}
};

Model* modelVesper = createModel<Vesper, VesperWidget>("Vesper");
