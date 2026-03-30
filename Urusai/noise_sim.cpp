#include <iostream>
#include <random>
#include <cmath>

int main() {
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    float lastPink = 0;
    float lastWhite = 0;
    
    double sumSqWhite = 0, sumSqPink = 0, sumSqBlue = 0;
    double sumSqViolet = 0, sumSqVelvet = 0, sumSqCmos = 0, sumSq8Bit = 0;

    std::mt19937 gen(12345);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    int N = 44100 * 10; // 10 seconds at 44.1kHz
    float sampleTime = 1.0f / 44100.0f;
    float velvetRate = 3000.f;
    float p = velvetRate * sampleTime;

    uint32_t lfsrStateCmos = 0xACE1u;
    uint32_t lfsrState8Bit = 0xACE1u;
    float eightBitPhase = 0.f;

    for (int i = 0; i < N; ++i) {
        float white = dist(gen);
        sumSqWhite += white * white;

        // Pink
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        pink *= 0.11f;
        sumSqPink += pink * pink;

        // Blue
        float blue = pink - lastPink;
        lastPink = pink;
        blue *= 10.f;
        sumSqBlue += blue * blue;

        // Violet
        float violet = white - lastWhite;
        lastWhite = white;
        violet *= 0.707f;
        sumSqViolet += violet * violet;

        // Velvet
        float velvet = 0.f;
        if (dist01(gen) < p) {
            velvet = (dist01(gen) > 0.5f) ? 1.f : -1.f;
        }
        sumSqVelvet += velvet * velvet;

        // CMOS
        unsigned bitCmos = ((lfsrStateCmos >> 0) ^ (lfsrStateCmos >> 2) ^ (lfsrStateCmos >> 3) ^ (lfsrStateCmos >> 5)) & 1;
        lfsrStateCmos = (lfsrStateCmos >> 1) | (bitCmos << 15);
        float cmos = (lfsrStateCmos & 1) ? 1.f : -1.f;
        sumSqCmos += cmos * cmos;

        // 8-Bit
        eightBitPhase += 10000.f * sampleTime;
        if (eightBitPhase >= 1.f) {
            eightBitPhase -= 1.f;
            unsigned bit8 = ((lfsrState8Bit >> 0) ^ (lfsrState8Bit >> 2) ^ (lfsrState8Bit >> 3) ^ (lfsrState8Bit >> 5)) & 1;
            lfsrState8Bit = (lfsrState8Bit >> 1) | (bit8 << 15);
        }
        float eightBit = (lfsrState8Bit & 1) ? 1.f : -1.f;
        sumSq8Bit += eightBit * eightBit;
    }

    float rmsWhite = std::sqrt(sumSqWhite / N);
    float rmsPink = std::sqrt(sumSqPink / N);
    float rmsBlue = std::sqrt(sumSqBlue / N);
    float rmsViolet = std::sqrt(sumSqViolet / N);
    float rmsVelvet = std::sqrt(sumSqVelvet / N);
    float rmsCmos = std::sqrt(sumSqCmos / N);
    float rms8Bit = std::sqrt(sumSq8Bit / N);

    std::cout << "RMS Values (before 5V mult):" << std::endl;
    std::cout << "White:  " << rmsWhite << std::endl;
    std::cout << "Pink:   " << rmsPink << " (multiplier relative to white: " << rmsWhite / rmsPink << ")" << std::endl;
    std::cout << "Blue:   " << rmsBlue << " (multiplier relative to white: " << rmsWhite / rmsBlue << ")" << std::endl;
    std::cout << "Violet: " << rmsViolet << " (multiplier relative to white: " << rmsWhite / rmsViolet << ")" << std::endl;
    std::cout << "Velvet: " << rmsVelvet << " (multiplier relative to white: " << rmsWhite / rmsVelvet << ")" << std::endl;
    std::cout << "CMOS:   " << rmsCmos << " (multiplier relative to white: " << rmsWhite / rmsCmos << ")" << std::endl;
    std::cout << "8-Bit:  " << rms8Bit << " (multiplier relative to white: " << rmsWhite / rms8Bit << ")" << std::endl;

    return 0;
}
