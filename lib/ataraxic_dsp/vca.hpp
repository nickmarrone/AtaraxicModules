#pragma once
#include <cmath>

namespace ataraxic_dsp {

// Voltage-controlled amplifier with blendable gain response curves.
//
// Set responseMix once (or whenever the knob changes) via setResponse().
// Call computeGain(gainNorm) or process(in, gainNorm) each sample.
//
// responseMix: 0.0 = full exponential (x^4, punchy)
//              0.5 = linear
//              1.0 = full logarithmic (x^0.25)
struct VCA {
    // Pre-computed from responseMix; updated by setResponse().
    bool  _upperHalf;  // true when responseMix >= 0.5
    float _mix;        // blend weight within the active half-range, [0, 1]
    float _invMix;     // 1 - _mix

    VCA() { setResponse(0.5f); }

    // Call whenever responseMix changes (e.g. on knob update).
    void setResponse(float responseMix) {
        _upperHalf = responseMix >= 0.5f;
        _mix    = _upperHalf ? (responseMix - 0.5f) * 2.0f : responseMix * 2.0f;
        _invMix = 1.0f - _mix;
    }

    // Compute gain from a normalized CV value.
    //   gainNorm: CV normalized to [0, 1] (e.g. clamp(voltage / 10.f, 0.f, 1.f))
    // Returns gain in [0, 1].
    float computeGain(float gainNorm) const {
        if (_upperHalf) {
            // Blend linear → logarithmic (x^0.25 via two sqrts, faster than pow)
            float logGain = std::sqrt(std::sqrt(gainNorm));
            return gainNorm * _invMix + logGain * _mix;
        } else {
            // Blend exponential (x^4) → linear
            float g2      = gainNorm * gainNorm;
            float expGain = g2 * g2;
            return expGain * _invMix + gainNorm * _mix;
        }
    }

    // Apply gain to an audio sample.
    float process(float in, float gainNorm) const {
        return in * computeGain(gainNorm);
    }
};

} // namespace ataraxic_dsp
