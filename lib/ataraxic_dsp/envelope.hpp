#pragma once
#include <cmath>
#include <stdint.h>

namespace ataraxic_dsp {

// One-shot AD envelope with linear ramp attack and decay.
struct EnvelopeAD {
    enum State : uint8_t { OFF, ATTACK, DECAY };

    State state;
    float output;      // Current envelope output, 0.0 to 1.0
    float attackRate;  // 1.0f / (attackTimeSecs * sampleRate)
    float decayRate;   // 1.0f / (decayTimeSecs  * sampleRate)

    EnvelopeAD() : state(OFF), output(0.0f), attackRate(0.0f), decayRate(0.0f) {}

    void reset() {
        state  = OFF;
        output = 0.0f;
    }

    // Start the attack phase.
    void trigger() {
        state = ATTACK;
    }

    // Process one sample. Returns output in [0, 1].
    float process() {
        switch (state) {
            case OFF:
                output = 0.0f;
                break;
            case ATTACK:
                output += attackRate;
                if (output >= 1.0f) {
                    output = 1.0f;
                    state  = DECAY;
                }
                break;
            case DECAY:
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

// Gated AR envelope with linear ramp attack, gate-held sustain, and release.
struct EnvelopeAR {
    enum State : uint8_t { OFF, ATTACK, SUSTAIN, RELEASE };

    State state;
    float output;       // Current envelope output, 0.0 to 1.0
    float attackRate;   // 1.0f / (attackTimeSecs  * sampleRate)
    float releaseRate;  // 1.0f / (releaseTimeSecs * sampleRate)

    EnvelopeAR() : state(OFF), output(0.0f), attackRate(0.0f), releaseRate(0.0f) {}

    void reset() {
        state  = OFF;
        output = 0.0f;
    }

    // Start the attack phase. Call on the rising edge of the gate.
    void trigger() {
        state = ATTACK;
    }

    // Process one sample.
    //   gate = current gate level (true while gate is held high)
    // Returns output in [0, 1].
    float process(bool gate) {
        switch (state) {
            case OFF:
                output = 0.0f;
                break;
            case ATTACK:
                output += attackRate;
                if (output >= 1.0f) {
                    output = 1.0f;
                    state  = SUSTAIN;
                } else if (!gate) {
                    state = RELEASE;
                }
                break;
            case SUSTAIN:
                output = 1.0f;
                if (!gate) state = RELEASE;
                break;
            case RELEASE:
                output -= releaseRate;
                if (output <= 0.0f) {
                    output = 0.0f;
                    state  = OFF;
                }
                break;
        }
        return output;
    }
};


} // namespace ataraxic_dsp
