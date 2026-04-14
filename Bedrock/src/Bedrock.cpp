#include "plugin.hpp"
#include "../../lib/ataraxic_dsp/ataraxic_dsp.hpp"

struct Bedrock : Module {
	enum ParamId {
		TUNE_PARAM,
		PITCH_PARAM,
		PDEC_PARAM,
		DECAY_PARAM,
		TONE_PARAM,
		DRIVE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
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

	ataraxic_dsp::SchmittTrigger trigger;
	ataraxic_dsp::EnvelopeAD     pitchEnv;
	ataraxic_dsp::EnvelopeAD     vcaEnv;
	ataraxic_dsp::Oscillator     osc;
	ataraxic_dsp::OnePole        toneFilter;
	ataraxic_dsp::VCA            vca;

	Bedrock() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(TUNE_PARAM,  0.f, 1.f, 0.25f, "Tune",         " Hz");
		configParam(PITCH_PARAM, 0.f, 1.f, 0.5f,  "Pitch Amount", " oct");
		configParam(PDEC_PARAM,  0.f, 1.f, 0.3f,  "Pitch Decay");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.4f,  "VCA Decay");
		configParam(TONE_PARAM,  0.f, 1.f, 0.8f,  "Tone",         " Hz");
		configParam(DRIVE_PARAM, 0.f, 1.f, 0.0f,  "Drive");

		configInput(TRIG_INPUT,  "Trigger");
		configOutput(OUT_OUTPUT, "Audio Out");

		trigger.lowThreshold  = 0.1f;
		trigger.highThreshold = 2.0f;
		vca.setResponse(0.0f);
	}

	void process(const ProcessArgs& args) override {
		float sampleTime = args.sampleTime;

		// --- TRIGGER ---
		bool triggered = trigger.process(inputs[TRIG_INPUT].getVoltage());
		if (triggered) {
			pitchEnv.trigger();
			vcaEnv.trigger();
		}

		// --- PITCH ENVELOPE ---
		// Fixed ~0.1ms attack; P.DEC: 1ms–500ms
		float pitchDecayTime      = ataraxic_dsp::cubicPotScale(params[PDEC_PARAM].getValue(), 0.001f, 0.5f);
		pitchEnv.attackRate       = 1.0f / (0.0001f * args.sampleRate);
		pitchEnv.decayRate        = 1.0f / (pitchDecayTime * args.sampleRate);
		float pitchEnvOut         = pitchEnv.process();

		// --- VCA ENVELOPE ---
		// Fixed ~0.5ms attack; DECAY: 10ms–2s
		float vcaDecayTime        = ataraxic_dsp::cubicPotScale(params[DECAY_PARAM].getValue(), 0.01f, 2.0f);
		vcaEnv.attackRate         = 1.0f / (0.0005f * args.sampleRate);
		vcaEnv.decayRate          = 1.0f / (vcaDecayTime * args.sampleRate);
		float vcaEnvOut           = vcaEnv.process();

		// --- OSCILLATOR ---
		// Pitch sweeps from baseFreq * 2^pitchOctaves down to baseFreq
		float baseFreq  = 30.0f * std::pow(200.0f / 30.0f, params[TUNE_PARAM].getValue());
		float pitchOcts = params[PITCH_PARAM].getValue() * 3.0f;
		float pitchFreq = baseFreq * std::pow(2.0f, pitchOcts * pitchEnvOut);
		float oscOut    = osc.process(pitchFreq, sampleTime, ataraxic_dsp::Oscillator::SINE);

		// --- TONE FILTER ---
		// TONE knob: 200 Hz – 18 kHz, log scale
		float toneCutoff = 200.0f * std::pow(90.0f, params[TONE_PARAM].getValue());
		float g          = ataraxic_dsp::OnePole::computeG(toneCutoff, sampleTime);
		float filtered   = toneFilter.processLP(oscOut, g);

		// --- VCA ---
		// Exponential response (responseMix=0) for punchy envelope shape
		float vcaGain = vca.computeGain(vcaEnvOut);
		float audio   = filtered * vcaGain;

		// --- SATURATOR + OUTPUT ---
		float sat = ataraxic_dsp::Saturator::process(audio, params[DRIVE_PARAM].getValue());
		outputs[OUT_OUTPUT].setVoltage(sat * 5.0f);
	}
};

// ---------------------------------------------------------------------------
// Labels widget
// ---------------------------------------------------------------------------

struct BedrockLabels : Widget {
	std::shared_ptr<Font> boldFont;

	void draw(const DrawArgs& args) override {
		std::shared_ptr<Font> font = APP->window->uiFont;
		if (!font) return;

		if (!boldFont) {
			boldFont = APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
		}

		auto drawText = [&](float x, float y, const char* text, float size,
		                    NVGcolor color, std::shared_ptr<Font> customFont = nullptr) {
			nvgFontSize(args.vg, size);
			nvgFontFaceId(args.vg, customFont ? customFont->handle : font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
			nvgFillColor(args.vg, color);
			nvgText(args.vg, x, y, text, NULL);
		};

		NVGcolor dark  = nvgRGB(34,  34,  34);
		NVGcolor white = nvgRGB(255, 255, 255);

		// Title (on dark header)
		drawText(45.72f, 23.f, "BEDROCK", 13.f, white, boldFont);

		// TRIG label (on dark input background)
		drawText(45.72f, 50.f, "TRIG", 8.f, white);

		// Knob labels
		drawText(22.86f, 103.f, "TUNE",  8.f, dark);
		drawText(68.58f, 103.f, "PITCH", 8.f, dark);
		drawText(22.86f, 158.f, "P.DEC", 8.f, dark);
		drawText(68.58f, 158.f, "DECAY", 8.f, dark);
		drawText(22.86f, 213.f, "TONE",  8.f, dark);
		drawText(68.58f, 213.f, "DRIVE", 8.f, dark);

		// OUT label (on dark output background)
		drawText(45.72f, 270.f, "OUT", 8.f, white);
	}
};

// ---------------------------------------------------------------------------
// Module widget
// ---------------------------------------------------------------------------

struct BedrockWidget : ModuleWidget {
	BedrockWidget(Bedrock* module) {
		setModule(module);

		// 6HP width
		box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		setPanel(createPanel(asset::plugin(pluginInstance, "res/Bedrock.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addChild(createWidget<BedrockLabels>(Vec(0, 0)));

		// I/O ports
		addInput(createInputCentered<PJ301MPort>(Vec(45.72f, 60.f),  module, Bedrock::TRIG_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(45.72f, 280.f), module, Bedrock::OUT_OUTPUT));

		// Knobs — two-column layout
		addParam(createParamCentered<RoundBlackKnob>(Vec(22.86f, 118.f), module, Bedrock::TUNE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(68.58f, 118.f), module, Bedrock::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(22.86f, 173.f), module, Bedrock::PDEC_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(68.58f, 173.f), module, Bedrock::DECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(22.86f, 228.f), module, Bedrock::TONE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(68.58f, 228.f), module, Bedrock::DRIVE_PARAM));
	}
};

Model* modelBedrock = createModel<Bedrock, BedrockWidget>("Bedrock");
