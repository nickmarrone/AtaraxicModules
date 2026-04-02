#include "plugin.hpp"
#include <random>

struct ToneCutoffQuantity : ParamQuantity {
	float getCutoff() {
		float character = clamp(getValue(), 0.f, 1.f);
		if (character < 0.5f) {
			return std::pow(10.f, 1.f + 3.3f * (character / 0.5f));
		} else {
			return std::pow(10.f, 1.f + 3.3f * ((character - 0.5f) / 0.5f));
		}
	}
	std::string getDisplayValueString() override {
		float character = clamp(getValue(), 0.f, 1.f);
		std::string type = (character < 0.5f) ? "LP " : "HP ";
		float cutoff = getCutoff();
		if (cutoff < 1000.f)
			return type + string::f("%.1f", cutoff);
		else
			return type + string::f("%.2f k", cutoff / 1000.f);
	}
};

struct Urusai : Module {
	enum ParamId {
		COLOR_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		COLOR_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		WHITE_OUTPUT,
		PINK_OUTPUT,
		BLUE_OUTPUT,
		VIOLET_OUTPUT,
		VELVET_OUTPUT,
		CMOS_OUTPUT,
		EIGHT_BIT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	float white = 0.f;		// White is used to generate many other noises
	float lastWhite = 0.f;
	bool whiteGenerated = false;

	float pink = 0.f;		// Pink is used to generate blue noise
	float lastPink = 0.f;
	float b0 = 0.f, b1 = 0.f, b2 = 0.f, b3 = 0.f, b4 = 0.f, b5 = 0.f, b6 = 0.f;
	bool pinkGenerated = false;

	uint32_t lfsrStateCmos = 0xACE1u;
	float cmosPhase = 0.f;

	uint32_t lfsrState8Bit = 0xACE1u;
	float eightBitPhase = 0.f;

	float whiteLp = 0.f, whiteHpLp = 0.f;
	float pinkLp = 0.f, pinkHpLp = 0.f;
	float blueLp = 0.f, blueHpLp = 0.f;
	float violetLp = 0.f, violetHpLp = 0.f;

	Urusai() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam<ToneCutoffQuantity>(COLOR_PARAM, 0.f, 1.f, 0.5f, "Tone", "Hz");
		configInput(COLOR_INPUT, "Tone CV");

		configOutput(WHITE_OUTPUT, "White Noise");
		configOutput(PINK_OUTPUT, "Pink Noise");
		configOutput(BLUE_OUTPUT, "Blue Noise");
		configOutput(VIOLET_OUTPUT, "Violet Noise");
		configOutput(VELVET_OUTPUT, "Velvet Noise");
		configOutput(CMOS_OUTPUT, "CMOS Noise");
		configOutput(EIGHT_BIT_OUTPUT, "8-Bit Noise");
	}

	// whiteNoise generates white noise
	float whiteNoise() {
		if (!whiteGenerated) {
			white = random::uniform() * 2.f - 1.f;
			whiteGenerated = true;
		}
		return white;
	}

	// pinkNoise generates pink noise
	float pinkNoise() {
		if (!pinkGenerated) {
			float white = whiteNoise();
			b0 = 0.99886f * b0 + white * 0.0555179f;
			b1 = 0.99332f * b1 + white * 0.0750759f;
			b2 = 0.96900f * b2 + white * 0.1538520f;
			b3 = 0.86650f * b3 + white * 0.3104856f;
			b4 = 0.55000f * b4 + white * 0.5329522f;
			b5 = -0.7616f * b5 - white * 0.0168980f;
			pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
			b6 = white * 0.115926f;
			pink *= 0.11f; 
			pinkGenerated = true;
		}
		return pink;
	}

	// blueNoise generates blue noise
	float blueNoise() {
		float pink = pinkNoise();
		float blue = pink - lastPink;
		lastPink = pink;
		blue *= 10.f; // gain compensation
		return blue;
	}

	// violetNoise generates violet noise
	float violetNoise() {
		float white = whiteNoise();
		float violet = white - lastWhite;
		lastWhite = white;
		violet *= 0.707f; // gain compensation
		return violet;
	}

	// velvetNoise generates velvet noise
	float velvetNoise(const ProcessArgs& args, float character) {
		float velvetRate = std::pow(10.f, 1.f + 3.f * character); // 10Hz to 10000Hz based on character
		float p = velvetRate * args.sampleTime;
		float velvet = 0.f;
		if (random::uniform() < p) {
			velvet = (whiteNoise() > 0.f) ? 1.f : -1.f;
		}
		return velvet;
	}

	// cmosNoise generates cmos noise
	float cmosNoise(const ProcessArgs& args, float character) {
		float rate = std::pow(10.f, 3.f + 1.8f * character); // sweep CMOS clock rate from 1000Hz to ~63kHz (Nyquist)
		cmosPhase += rate * args.sampleTime;
		if (cmosPhase >= 1.f) {
			cmosPhase -= 1.f;
			// 17-bit LFSR (like MM5837 chip commonly used in analog synths)
			// Taps for 17-bit: 17 and 14 (indices 16 and 13)
			unsigned bitCmos = ((lfsrStateCmos >> 13) ^ (lfsrStateCmos >> 16)) & 1;
			lfsrStateCmos = (lfsrStateCmos << 1) | bitCmos;
		}
		return ((lfsrStateCmos >> 16) & 1) ? 1.f : -1.f;
	}

	// eightBitNoise generates 8-bit noise
	float eightBitNoise(const ProcessArgs& args, float character) {
		float rate = std::pow(10.f, 1.f + 3.f * character); // Sweep rate from 10Hz to 10000Hz
		eightBitPhase += rate * args.sampleTime;
		if (eightBitPhase >= 1.f) {
			eightBitPhase -= 1.f;
			// 15-bit LFSR (classic NES style noise)
			// Taps: 14 and 13
			unsigned bit8 = ((lfsrState8Bit >> 13) ^ (lfsrState8Bit >> 14)) & 1;
			lfsrState8Bit = (lfsrState8Bit << 1) | bit8;
		}
		return ((lfsrState8Bit >> 14) & 1) ? 1.f : -1.f;
	}

	void process(const ProcessArgs& args) override {
		whiteGenerated = false;
		pinkGenerated = false;

		float character = params[COLOR_PARAM].getValue();
		if (inputs[COLOR_INPUT].isConnected()) {
			character += inputs[COLOR_INPUT].getVoltage() / 10.f;
		}
		character = clamp(character, 0.f, 1.f);
		
		float lpCutoff = std::pow(10.f, 1.f + 3.3f * clamp(character / 0.5f, 0.f, 1.f)); 
		float hpCutoff = std::pow(10.f, 1.f + 3.3f * clamp((character - 0.5f) / 0.5f, 0.f, 1.f));
		
		float gLp = clamp(lpCutoff * args.sampleTime * 3.14159f, 0.f, 1.f);
		float gHp = clamp(hpCutoff * args.sampleTime * 3.14159f, 0.f, 1.f);

		bool isHp = character >= 0.5f;

		if (outputs[WHITE_OUTPUT].isConnected()) {
			float in = whiteNoise() * 5.f;
			whiteLp += gLp * (in - whiteLp);
			whiteHpLp += gHp * (in - whiteHpLp);
			outputs[WHITE_OUTPUT].setVoltage(isHp ? (in - whiteHpLp) : whiteLp);
		}

		if (outputs[PINK_OUTPUT].isConnected()) {
			float in = pinkNoise() * 15.3f;
			pinkLp += gLp * (in - pinkLp);
			pinkHpLp += gHp * (in - pinkHpLp);
			outputs[PINK_OUTPUT].setVoltage(isHp ? (in - pinkHpLp) : pinkLp);
		}

		if (outputs[BLUE_OUTPUT].isConnected()) {
			float in = blueNoise() * 2.5f;
			blueLp += gLp * (in - blueLp);
			blueHpLp += gHp * (in - blueHpLp);
			outputs[BLUE_OUTPUT].setVoltage(isHp ? (in - blueHpLp) : blueLp);
		}

		if (outputs[VIOLET_OUTPUT].isConnected()) {
			float in = violetNoise() * 5.f;
			violetLp += gLp * (in - violetLp);
			violetHpLp += gHp * (in - violetHpLp);
			outputs[VIOLET_OUTPUT].setVoltage(isHp ? (in - violetHpLp) : violetLp);
		}

		if (outputs[VELVET_OUTPUT].isConnected()) {
			outputs[VELVET_OUTPUT].setVoltage(velvetNoise(args, character) * 11.f);
		}

		if (outputs[CMOS_OUTPUT].isConnected()) {
			outputs[CMOS_OUTPUT].setVoltage(cmosNoise(args, character) * 2.88f);
		}

		if (outputs[EIGHT_BIT_OUTPUT].isConnected()) {
			outputs[EIGHT_BIT_OUTPUT].setVoltage(eightBitNoise(args, character) * 2.88f);
		}
	}
};

struct UrusaiLabels : Widget {
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

		NVGcolor dark = nvgRGB(26, 26, 26);
		NVGcolor white = nvgRGB(255, 255, 255);

		// Macro label
		drawText(15.f, 24.f, "TONE", 10.f, dark, boldFont);

		// Labels for WHT port
		drawText(15.f, 108.f, "WHT", 9.f, dark, boldFont);

		// Labels for PNK port
		drawText(15.f, 146.f, "PNK", 9.f, dark, boldFont);

		// Labels for BLU port
		drawText(15.f, 184.f, "BLU", 9.f, dark, boldFont);

		// Labels for VIO port
		drawText(15.f, 222.f, "VIO", 9.f, dark, boldFont);

		// Labels for VLV port
		drawText(15.f, 260.f, "VLV", 9.f, dark, boldFont);

		// Labels for CMOS port
		drawText(15.f, 298.f, "CMOS", 9.f, white, boldFont);

		// Labels for 8-BIT port
		drawText(15.f, 336.f, "8-BIT", 9.f, white, boldFont);
	}
};

struct UrusaiWidget : ModuleWidget {
	UrusaiWidget(Urusai* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Urusai.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addChild(createWidget<UrusaiLabels>(Vec(0, 0)));

		addParam(createParamCentered<RoundBlackKnob>(Vec(15., 46.), module, Urusai::COLOR_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(15., 77.), module, Urusai::COLOR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 122.), module, Urusai::WHITE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 160.), module, Urusai::PINK_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 198.), module, Urusai::BLUE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 236.), module, Urusai::VIOLET_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 274.), module, Urusai::VELVET_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 312.), module, Urusai::CMOS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 350.), module, Urusai::EIGHT_BIT_OUTPUT));
	}
};

Model* modelUrusai = createModel<Urusai, UrusaiWidget>("Urusai");
