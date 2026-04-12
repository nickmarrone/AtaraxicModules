#pragma once
#include <stdint.h>

namespace ataraxic_dsp {

// Schmitt trigger with hysteresis. Returns true on LOW->HIGH transition (rising edge).
// Direct port of rack::dsp::TSchmittTrigger<float>.
struct SchmittTrigger {
    enum State : uint8_t { LOW, HIGH, UNINITIALIZED };
    State state;

    SchmittTrigger() : state(UNINITIALIZED) {}

    void reset() { state = UNINITIALIZED; }

    bool process(float in, float lowThreshold = 0.0f, float highThreshold = 1.0f) {
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
