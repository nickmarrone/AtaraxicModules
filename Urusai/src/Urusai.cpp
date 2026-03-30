#include "plugin.hpp"
#include <random>

struct Urusai : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
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

	float b0 = 0.f, b1 = 0.f, b2 = 0.f, b3 = 0.f, b4 = 0.f, b5 = 0.f, b6 = 0.f;
	float lastWhite = 0.f;
	float lastPink = 0.f;
	uint32_t lfsrStateCmos = 0xACE1u;
	uint32_t lfsrState8Bit = 0xACE1u;
	float eightBitPhase = 0.f;

	Urusai() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(WHITE_OUTPUT, "White Noise");
		configOutput(PINK_OUTPUT, "Pink Noise");
		configOutput(BLUE_OUTPUT, "Blue Noise");
		configOutput(VIOLET_OUTPUT, "Violet Noise");
		configOutput(VELVET_OUTPUT, "Velvet Noise");
		configOutput(CMOS_OUTPUT, "CMOS Noise");
		configOutput(EIGHT_BIT_OUTPUT, "8-Bit Noise");
	}

	void process(const ProcessArgs& args) override {
		// White Noise
		float white = random::uniform() * 2.f - 1.f; 

		// Pink Noise
		b0 = 0.99886f * b0 + white * 0.0555179f;
		b1 = 0.99332f * b1 + white * 0.0750759f;
		b2 = 0.96900f * b2 + white * 0.1538520f;
		b3 = 0.86650f * b3 + white * 0.3104856f;
		b4 = 0.55000f * b4 + white * 0.5329522f;
		b5 = -0.7616f * b5 - white * 0.0168980f;
		float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
		b6 = white * 0.115926f;
		pink *= 0.11f; 

		// Blue Noise
		float blue = pink - lastPink;
		lastPink = pink;
		blue *= 10.f; // gain compensation

		// Violet Noise
		float violet = white - lastWhite;
		lastWhite = white;
		violet *= 0.707f; 

		// Velvet Noise (sparse random impulses)
		float velvetRate = 3000.f; // Impulses per second
		float p = velvetRate * args.sampleTime;
		float velvet = 0.f;
		if (random::uniform() < p) {
			velvet = (random::uniform() > 0.5f) ? 1.f : -1.f;
		}

		// CMOS Noise
        unsigned bitCmos = ((lfsrStateCmos >> 0) ^ (lfsrStateCmos >> 2) ^ (lfsrStateCmos >> 3) ^ (lfsrStateCmos >> 5)) & 1;
        lfsrStateCmos = (lfsrStateCmos >> 1) | (bitCmos << 15);
		float cmos = (lfsrStateCmos & 1) ? 1.f : -1.f;

        // 8-Bit Noise
        eightBitPhase += 10000.f * args.sampleTime;
        if (eightBitPhase >= 1.f) {
            eightBitPhase -= 1.f;
            unsigned bit8 = ((lfsrState8Bit >> 0) ^ (lfsrState8Bit >> 2) ^ (lfsrState8Bit >> 3) ^ (lfsrState8Bit >> 5)) & 1;
		    lfsrState8Bit = (lfsrState8Bit >> 1) | (bit8 << 15);
        }
		float eightBit = (lfsrState8Bit & 1) ? 1.f : -1.f;

		outputs[WHITE_OUTPUT].setVoltage(white * 5.f);
		outputs[PINK_OUTPUT].setVoltage(pink * 15.3f);
		outputs[BLUE_OUTPUT].setVoltage(blue * 2.5f);
		outputs[VIOLET_OUTPUT].setVoltage(violet * 5.f);
		outputs[VELVET_OUTPUT].setVoltage(velvet * 11.f);
		outputs[CMOS_OUTPUT].setVoltage(cmos * 2.88f);
		outputs[EIGHT_BIT_OUTPUT].setVoltage(eightBit * 2.88f);
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

		// Labels for WHT port
		drawText(15.f, 27.f, "う", 13.f, dark);
		drawText(15.f, 33.f, "WHT", 5.f, dark, boldFont);

		// Labels for PNK port
		drawText(15.f, 76.f, "る", 13.f, dark);
		drawText(15.f, 82.f, "PNK", 5.f, dark, boldFont);

		// Labels for BLU port
		drawText(15.f, 125.f, "さ", 13.f, dark);
		drawText(15.f, 131.f, "BLU", 5.f, dark, boldFont);

		// Labels for VIO port
		drawText(15.f, 174.f, "い", 13.f, dark);
		drawText(15.f, 180.f, "VIO", 5.f, dark, boldFont);

		// Labels for VLV port
		drawText(15.f, 229.f, "VLV", 8.f, dark, boldFont);

		// Labels for CMS port
		drawText(15.f, 278.f, "CMOS", 9.f, white, boldFont);

		// Labels for 8-BIT port
		drawText(15.f, 327.f, "8-BIT", 9.f, white, boldFont);
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

		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 47.), module, Urusai::WHITE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 96.), module, Urusai::PINK_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 145.), module, Urusai::BLUE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 194.), module, Urusai::VIOLET_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 243.), module, Urusai::VELVET_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 292.), module, Urusai::CMOS_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(15., 341.), module, Urusai::EIGHT_BIT_OUTPUT));
	}
};

Model* modelUrusai = createModel<Urusai, UrusaiWidget>("Urusai");
