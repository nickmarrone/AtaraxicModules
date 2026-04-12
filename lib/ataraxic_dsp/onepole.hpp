#pragma once
#include <cmath>

namespace ataraxic_dsp {

// One-pole lowpass filter with highpass tap (HP = input - LP).
// State is the LP integrator value.
struct OnePole {
    float lp;

    OnePole() : lp(0.0f) {}

    void reset() { lp = 0.0f; }

    // Process one sample through the lowpass. Returns LP output.
    // HP output = in - result.
    //   g: filter coefficient in [0, 1]. Use computeG() to derive from frequency.
    float processLP(float in, float g) {
        lp += g * (in - lp);
        return lp;
    }

    // Compute the filter coefficient g from a cutoff frequency.
    //   cutoffHz:   cutoff frequency in Hz
    //   sampleTime: 1 / sampleRate
    static float computeG(float cutoffHz, float sampleTime) {
        float g = cutoffHz * 3.14159265f * sampleTime;
        return (g < 0.0f) ? 0.0f : (g > 1.0f) ? 1.0f : g;
    }
};

} // namespace ataraxic_dsp
