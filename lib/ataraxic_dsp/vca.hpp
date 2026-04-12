#pragma once
#include <cmath>

namespace ataraxic_dsp {

// Voltage-controlled amplifier with blendable gain response curves.
// All methods are stateless (static).
struct VCA {
    // Compute gain given a normalized CV and response mix.
    //   gainNorm:    CV normalized to [0, 1] (e.g. voltage / 10V, clamped)
    //   responseMix: 0.0 = full exponential (x^4, punchy),
    //                0.5 = linear,
    //                1.0 = full logarithmic (x^0.25)
    // Returns gain in [0, 1].
    static float computeGain(float gainNorm, float responseMix) {
        float expGain = gainNorm * gainNorm * gainNorm * gainNorm;
        float linGain = gainNorm;
        float logGain = std::pow(gainNorm, 0.25f);

        if (responseMix < 0.5f) {
            float mix = responseMix * 2.0f;
            return expGain + mix * (linGain - expGain);
        } else {
            float mix = (responseMix - 0.5f) * 2.0f;
            return linGain + mix * (logGain - linGain);
        }
    }

    // Apply gain to an audio sample.
    static float process(float in, float gainNorm, float responseMix) {
        return in * computeGain(gainNorm, responseMix);
    }
};

} // namespace ataraxic_dsp
