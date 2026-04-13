#include <iostream>
#include <random>
#include <cmath>
#include "src/UrusaiDsp.hpp"

// Standalone RMS calibration tool for Urusai noise types.
// Generates 10 seconds of audio at 44.1 kHz and reports RMS values and
// gain multipliers needed to equalize all outputs to match white noise RMS.
//
// Compile: g++ -std=c++11 -I../lib -O2 noise_sim.cpp -o noise_sim -lm
// Run:     ./noise_sim

static std::mt19937 gen(12345);
static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

static float rngFn(void*) {
    return dist01(gen);
}

int main() {
    const int N = 44100 * 10; // 10 seconds at 44.1 kHz
    const float sampleTime = 1.0f / 44100.0f;
    const float character  = 0.5f; // mid-point tone for calibration

    UrusaiDsp dsp;
    dsp.init(rngFn);

    double sumSqWhite  = 0, sumSqPink   = 0, sumSqBlue  = 0;
    double sumSqViolet = 0, sumSqVelvet = 0, sumSqCmos  = 0, sumSq8Bit = 0;

    for (int i = 0; i < N; ++i) {
        dsp.beginSample();

        float white  = dsp.getWhite();
        float pink   = dsp.getPink();
        float blue   = dsp.getBlue();
        float violet = dsp.getViolet();
        float velvet = dsp.getVelvet(character, sampleTime);
        float cmos   = dsp.getCmos(character, sampleTime);
        float ebit   = dsp.getEightBit(character, sampleTime);

        sumSqWhite  += white  * white;
        sumSqPink   += pink   * pink;
        sumSqBlue   += blue   * blue;
        sumSqViolet += violet * violet;
        sumSqVelvet += velvet * velvet;
        sumSqCmos   += cmos   * cmos;
        sumSq8Bit   += ebit   * ebit;
    }

    float rmsWhite  = std::sqrt((float)(sumSqWhite  / N));
    float rmsPink   = std::sqrt((float)(sumSqPink   / N));
    float rmsBlue   = std::sqrt((float)(sumSqBlue   / N));
    float rmsViolet = std::sqrt((float)(sumSqViolet / N));
    float rmsVelvet = std::sqrt((float)(sumSqVelvet / N));
    float rmsCmos   = std::sqrt((float)(sumSqCmos   / N));
    float rms8Bit   = std::sqrt((float)(sumSq8Bit   / N));

    std::cout << "RMS Values (raw, before gain calibration):" << std::endl;
    std::cout << "White:  " << rmsWhite  << std::endl;
    std::cout << "Pink:   " << rmsPink   << "  -> multiplier: " << rmsWhite / rmsPink   << std::endl;
    std::cout << "Blue:   " << rmsBlue   << "  -> multiplier: " << rmsWhite / rmsBlue   << std::endl;
    std::cout << "Violet: " << rmsViolet << "  -> multiplier: " << rmsWhite / rmsViolet << std::endl;
    std::cout << "Velvet: " << rmsVelvet << "  -> multiplier: " << rmsWhite / rmsVelvet << std::endl;
    std::cout << "CMOS:   " << rmsCmos   << "  -> multiplier: " << rmsWhite / rmsCmos   << std::endl;
    std::cout << "8-Bit:  " << rms8Bit   << "  -> multiplier: " << rmsWhite / rms8Bit   << std::endl;

    std::cout << std::endl;
    std::cout << "Current gain constants in noise.hpp:" << std::endl;
    std::cout << "URUSAI_GAIN_WHITE  = " << ataraxic_dsp::NOISE_GAIN_WHITE  << std::endl;
    std::cout << "URUSAI_GAIN_PINK   = " << ataraxic_dsp::NOISE_GAIN_PINK   << std::endl;
    std::cout << "URUSAI_GAIN_BLUE   = " << ataraxic_dsp::NOISE_GAIN_BLUE   << std::endl;
    std::cout << "URUSAI_GAIN_VIOLET = " << ataraxic_dsp::NOISE_GAIN_VIOLET << std::endl;
    std::cout << "URUSAI_GAIN_VELVET = " << ataraxic_dsp::NOISE_GAIN_VELVET << std::endl;
    std::cout << "URUSAI_GAIN_CMOS   = " << ataraxic_dsp::NOISE_GAIN_CMOS   << std::endl;
    std::cout << "URUSAI_GAIN_8BIT   = " << ataraxic_dsp::NOISE_GAIN_8BIT   << std::endl;

    return 0;
}
