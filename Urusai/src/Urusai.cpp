#include "plugin.hpp"
#include "../../lib/ataraxic_dsp/ataraxic_dsp.hpp"

static float rackRngWrapper(void*) {
    return rack::random::uniform();
}

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

	ataraxic_dsp::UrusaiDsp dsp;

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

		dsp.init(rackRngWrapper);
	}

	void process(const ProcessArgs& args) override {
		dsp.beginSample();

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
			float in = dsp.getWhite() * ataraxic_dsp::URUSAI_GAIN_WHITE;
			dsp.whiteLp  += gLp * (in - dsp.whiteLp);
			dsp.whiteHpLp += gHp * (in - dsp.whiteHpLp);
			outputs[WHITE_OUTPUT].setVoltage(isHp ? (in - dsp.whiteHpLp) : dsp.whiteLp);
		}

		if (outputs[PINK_OUTPUT].isConnected()) {
			float in = dsp.getPink() * ataraxic_dsp::URUSAI_GAIN_PINK;
			dsp.pinkLp  += gLp * (in - dsp.pinkLp);
			dsp.pinkHpLp += gHp * (in - dsp.pinkHpLp);
			outputs[PINK_OUTPUT].setVoltage(isHp ? (in - dsp.pinkHpLp) : dsp.pinkLp);
		}

		if (outputs[BLUE_OUTPUT].isConnected()) {
			float in = dsp.getBlue() * ataraxic_dsp::URUSAI_GAIN_BLUE;
			dsp.blueLp  += gLp * (in - dsp.blueLp);
			dsp.blueHpLp += gHp * (in - dsp.blueHpLp);
			outputs[BLUE_OUTPUT].setVoltage(isHp ? (in - dsp.blueHpLp) : dsp.blueLp);
		}

		if (outputs[VIOLET_OUTPUT].isConnected()) {
			float in = dsp.getViolet() * ataraxic_dsp::URUSAI_GAIN_VIOLET;
			dsp.violetLp  += gLp * (in - dsp.violetLp);
			dsp.violetHpLp += gHp * (in - dsp.violetHpLp);
			outputs[VIOLET_OUTPUT].setVoltage(isHp ? (in - dsp.violetHpLp) : dsp.violetLp);
		}

		if (outputs[VELVET_OUTPUT].isConnected()) {
			outputs[VELVET_OUTPUT].setVoltage(dsp.getVelvet(character, args.sampleTime) * ataraxic_dsp::URUSAI_GAIN_VELVET);
		}

		if (outputs[CMOS_OUTPUT].isConnected()) {
			outputs[CMOS_OUTPUT].setVoltage(dsp.getCmos(character, args.sampleTime) * ataraxic_dsp::URUSAI_GAIN_CMOS);
		}

		if (outputs[EIGHT_BIT_OUTPUT].isConnected()) {
			outputs[EIGHT_BIT_OUTPUT].setVoltage(dsp.getEightBit(character, args.sampleTime) * ataraxic_dsp::URUSAI_GAIN_8BIT);
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
