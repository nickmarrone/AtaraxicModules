#pragma once
#include <cmath>
#include <stdint.h>

namespace ataraxic_dsp {

// AD/AR envelope generator with linear ramp attack and decay/release.
struct EnvelopeADAR {
    enum State : uint8_t { OFF, ATTACK, SUSTAIN, DECAY_RELEASE };

    State state;
    float output;   // Current envelope output, 0.0 to 1.0
    bool  arMode;   // false = AD (one-shot), true = AR (gate-held sustain)

    EnvelopeADAR() : state(OFF), output(0.0f), arMode(false) {}

    void reset() {
        state  = OFF;
        output = 0.0f;
        arMode = false;
    }

    // Trigger a one-shot AD envelope (ignores gate level).
    void triggerAD() {
        state  = ATTACK;
        arMode = false;
    }

    // Start an AR envelope (sustains while gate is held high).
    // Call on rising edge of gate.
    void triggerAR() {
        state  = ATTACK;
        arMode = true;
    }

    // Process one sample.
    //   attackRate = 1.0f / (attackTimeSecs * sampleRate)
    //   decayRate  = 1.0f / (decayTimeSecs  * sampleRate)
    //   gate       = current gate level (used in AR mode for sustain/release decisions)
    // Returns output in [0, 1].
    float process(float attackRate, float decayRate, bool gate) {
        switch (state) {
            case OFF:
                output = 0.0f;
                break;
            case ATTACK:
                output += attackRate;
                if (output >= 1.0f) {
                    output = 1.0f;
                    state  = arMode ? SUSTAIN : DECAY_RELEASE;
                } else if (arMode && !gate) {
                    state = DECAY_RELEASE;
                }
                break;
            case SUSTAIN:
                output = 1.0f;
                if (!gate) state = DECAY_RELEASE;
                break;
            case DECAY_RELEASE:
                output -= decayRate;
                if (output <= 0.0f) {
                    output = 0.0f;
                    state  = OFF;
                }
                break;
        }
        return output;
    }
};

// Cubic "log-pot" time scaling used by ADVCA.
// Maps param in [0, 1] to a time in seconds using a x^3 curve, giving a more
// natural feel than a pure exponential: slow times spread out more evenly.
//   baseTime: minimum time (e.g. 0.001s = 1ms)
//   maxTime:  maximum time (e.g. 2.0s for attack, 10.0s for decay)
inline float advcaScaleTime(float param, float baseTime, float maxTime) {
    return baseTime + (maxTime - baseTime) * (param * param * param);
}

} // namespace ataraxic_dsp
