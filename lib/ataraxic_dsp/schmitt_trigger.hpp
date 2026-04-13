#pragma once
#include <stdint.h>

namespace ataraxic_dsp {

// Schmitt trigger with hysteresis. Returns true on LOW->HIGH transition (rising edge).
// Direct port of rack::dsp::TSchmittTrigger<float>.
//
// Set lowThreshold and highThreshold once at init if non-default values are needed.
// Default thresholds (0.0 / 1.0) suit a 0–1 V gate; use e.g. (0.1, 2.0) for
// Eurorack 5 V gates.
struct SchmittTrigger {
    enum State : uint8_t { LOW, HIGH, UNINITIALIZED };

    State state;
    float lowThreshold;   // transition HIGH→LOW when input falls at or below this
    float highThreshold;  // transition LOW→HIGH when input rises at or above this

    SchmittTrigger() : state(UNINITIALIZED), lowThreshold(0.0f), highThreshold(1.0f) {}

    void reset() { state = UNINITIALIZED; }

    bool process(float in) {
        if (state == LOW && in >= highThreshold) {
            state = HIGH;
            return true;
        } else if (state == HIGH && in <= lowThreshold) {
            state = LOW;
        } else if (state == UNINITIALIZED) {
            state = (in >= highThreshold) ? HIGH : LOW;
        }
        return false;
    }

    bool isHigh() const { return state == HIGH; }
};

} // namespace ataraxic_dsp
