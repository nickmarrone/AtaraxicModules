#include "plugin.hpp"
#include "../../lib/ataraxic_dsp/ataraxic_dsp.hpp"

struct ADVCA : Module {
	enum ParamId {
		ATTACK_PARAM,
		DECAY_PARAM,
		RESPONSE_PARAM, // 0 = Linear, 1 = Exponential
		CV_ATTEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_INPUT,
		GATE_INPUT,
		IN_INPUT,
		CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENV_OUTPUT,
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LEVEL_LIGHT,
		LIGHTS_LEN
	};

	ataraxic_dsp::EnvelopeADAR envelope;
	ataraxic_dsp::SchmittTrigger trigTrigger;
	ataraxic_dsp::SchmittTrigger gateSchmittTrigger;

	ADVCA() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(ATTACK_PARAM, 0.f, 1.f, 0.2f, "Attack Time");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.2f, "Decay/Release Time");
		configParam(RESPONSE_PARAM, 0.f, 1.f, 0.5f, "Response (0: Exp, 0.5: Lin, 1: Log)");
		configParam(CV_ATTEN_PARAM, 0.f, 1.f, 1.f, "CV Attenuator");

		configInput(TRIG_INPUT, "Trigger (AD)");
		configInput(GATE_INPUT, "Gate (AR)");
		configInput(IN_INPUT, "Audio In");
		configInput(CV_INPUT, "VCA CV");

		configOutput(ENV_OUTPUT, "Envelope Out");
		configOutput(OUT_OUTPUT, "Audio Out");
	}

	void process(const ProcessArgs& args) override {
		// --- ENVELOPE ---
		bool gate = inputs[GATE_INPUT].getVoltage() >= 1.0f;
		float attackParam = params[ATTACK_PARAM].getValue();
		float decayParam  = params[DECAY_PARAM].getValue();

		// Time scaling: Map 0-1 param to 1ms -> 10s using a cubic curve.
		// A cubic curve (x^3) gives a more natural "log pot" feel than the pure exponential formula,
		// spreading out the mid-range times more evenly.
		float attackTime = ataraxic_dsp::advcaScaleTime(attackParam, 0.001f, 2.0f);
		float decayTime  = ataraxic_dsp::advcaScaleTime(decayParam,  0.001f, 10.0f);

		float attackRate = 1.f / (attackTime * args.sampleRate);
		float decayRate  = 1.f / (decayTime  * args.sampleRate);

		if (trigTrigger.process(inputs[TRIG_INPUT].getVoltage())) {
			envelope.triggerAD();
		} else if (gateSchmittTrigger.process(inputs[GATE_INPUT].getVoltage())) {
			envelope.triggerAR();
		}

		float envOut = envelope.process(attackRate, decayRate, gate);

		// Calculate 0 to 10V Envelope Output
		outputs[ENV_OUTPUT].setVoltage(envOut * 10.f);

		// --- VCA ---
		// CV is normalled to ENV_OUTPUT
		float cvVolts = inputs[CV_INPUT].getNormalVoltage(envOut * 10.f);

		// Apply attenuator (0 to 1) to CV
		float cvInput = cvVolts * params[CV_ATTEN_PARAM].getValue();

		// Map CV (0-10V) to [0, 1] gain scale
		float gainNorm = clamp(cvInput / 10.f, 0.f, 1.f);

		float responseMix = params[RESPONSE_PARAM].getValue(); // 0 = Exp, 0.5 = Lin, 1 = Log
		float finalGain   = ataraxic_dsp::VCA::computeGain(gainNorm, responseMix);

		// Apply VCA
		float audioIn  = inputs[IN_INPUT].getVoltage();
		float audioOut = audioIn * finalGain;

		outputs[OUT_OUTPUT].setVoltage(audioOut);

		// Set LED brightness to the final gain value
		lights[LEVEL_LIGHT].setBrightness(finalGain);
	}
};

struct ADVCALabels : Widget {
	std::shared_ptr<Font> boldFont;

	void draw(const DrawArgs& args) override {
		std::shared_ptr<Font> font = APP->window->uiFont;
		if (!font) return;

		if (!boldFont) {
			boldFont = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
		}

		auto drawText = [&](float x, float y, const char* text, float size, NVGcolor color, std::shared_ptr<Font> customFont = nullptr) {
			nvgFontSize(args.vg, size);
			nvgFontFaceId(args.vg, customFont ? customFont->handle : font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
			nvgFillColor(args.vg, color);
			nvgText(args.vg, x, y, text, NULL);
		};

		NVGcolor dark = nvgRGB(34, 34, 34);
		NVGcolor white = nvgRGB(255, 255, 255);

		// Title
		drawText(30.48f, 28.f, "ADVCA", 14.f, dark, boldFont);


		// Envelope Inputs
		drawText(16.f, 47.f, "TRIG", 8.f, dark);
		drawText(44.f, 47.f, "GATE", 8.f, dark);

		// Knobs
		drawText(30.48f, 94.f, "ATTACK", 8.f, dark);
		drawText(30.48f, 134.f, "DECAY", 8.f, dark);

		// Envelope Output
		drawText(30.48f, 199.f, "ENV", 8.f, white);

		// CV Section
		drawText(16.f, 281.f, "CV", 8.f, dark);
		drawText(44.f, 281.f, "AMT", 8.f, dark);

		// VCA I/O
		drawText(16.f, 324.f, "IN", 8.f, dark);
		drawText(44.f, 324.f, "OUT", 8.f, white);
	}
};

struct ADVCAWidget : ModuleWidget {
	ADVCAWidget(ADVCA* module) {
		setModule(module);

		// 4HP width
		box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		setPanel(createPanel(asset::plugin(pluginInstance, "res/ADVCA.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addChild(createWidget<ADVCALabels>(Vec(0, 0)));

		// Measurements
		float centerX = box.size.x / 2.f;
		float leftColumn = 16.f;
		float rightColumn = 44.f;

		// Top Half: Envelope
		addInput(createInputCentered<PJ301MPort>(Vec(leftColumn, 60.f), module, ADVCA::TRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(rightColumn, 60.f), module, ADVCA::GATE_INPUT));

		addParam(createParamCentered<RoundBlackKnob>(Vec(centerX, 110.f), module, ADVCA::ATTACK_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(centerX, 150.f), module, ADVCA::DECAY_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(Vec(centerX, 215.f), module, ADVCA::ENV_OUTPUT));

		// Bottom Half: VCA
		addParam(createParamCentered<Trimpot>(Vec(centerX, 255.f), module, ADVCA::RESPONSE_PARAM));

		addChild(createLightCentered<MediumLight<RedLight>>(Vec(centerX, 275.f), module, ADVCA::LEVEL_LIGHT));

		addInput(createInputCentered<PJ301MPort>(Vec(leftColumn, 295.f), module, ADVCA::CV_INPUT));
		addParam(createParamCentered<Trimpot>(Vec(rightColumn, 295.f), module, ADVCA::CV_ATTEN_PARAM));

		addInput(createInputCentered<PJ301MPort>(Vec(leftColumn, 340.f), module, ADVCA::IN_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(rightColumn, 340.f), module, ADVCA::OUT_OUTPUT));
	}
};

Model* modelADVCA = createModel<ADVCA, ADVCAWidget>("ADVCA");
