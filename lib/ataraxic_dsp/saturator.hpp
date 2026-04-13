#pragma once
#include <cmath>

namespace ataraxic_dsp {

// Soft-clip saturator using tanh with drive-controlled gain compensation.
// All methods are static — no state.
struct Saturator {
    // Process one sample.
    //   in:    audio input, any range
    //   drive: saturation amount in [0, 1].
    //          0 = 1x gain (gentle brickwall near ±1),
    //          1 = 10x gain (heavy saturation, onset around ±0.1).
    // Returns tanh(in * gain) / gain, where gain = 1 + drive * 9.
    // Small-signal behavior is unity gain; large signals are soft-clipped.
    static float process(float in, float drive) {
        float gain = 1.0f + drive * 9.0f;
        return std::tanh(in * gain) / gain;
    }
};

} // namespace ataraxic_dsp
