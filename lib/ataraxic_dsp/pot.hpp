#pragma once

namespace ataraxic_dsp {

// Maps a normalized potentiometer value in [0, 1] to a target range using a
// cubic (x³) curve. The cubic curve gives a "log-pot" feel: finer resolution
// at the low end, compressed at the high end — without the discontinuities of
// a true logarithm near zero.
//
//   param:   knob/CV value in [0, 1]
//   minVal:  output value when param = 0  (e.g. 0.001f for 1 ms)
//   maxVal:  output value when param = 1  (e.g. 10.0f for 10 s)
inline float cubicPotScale(float param, float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * (param * param * param);
}

} // namespace ataraxic_dsp
