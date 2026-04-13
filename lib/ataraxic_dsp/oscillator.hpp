#pragma once
#include <cmath>
#include <stdint.h>

namespace ataraxic_dsp {

namespace detail {

// 128-entry sine LUT: sin(2π * i / 256) for i = 0..127 (positive half-cycle).
// The negative half-cycle is recovered via sin(x + π) = -sin(x).
inline const float* sine_lut() {
    static const float table[128] = {
        0.000000f,  0.024541f,  0.049068f,  0.073565f,  0.098017f,  0.122411f,  0.146730f,  0.170962f,
        0.195090f,  0.219101f,  0.242980f,  0.266713f,  0.290285f,  0.313682f,  0.336890f,  0.359895f,
        0.382683f,  0.405241f,  0.427555f,  0.449611f,  0.471397f,  0.492898f,  0.514103f,  0.534998f,
        0.555570f,  0.575808f,  0.595699f,  0.615232f,  0.634393f,  0.653173f,  0.671559f,  0.689541f,
        0.707107f,  0.724247f,  0.740951f,  0.757209f,  0.773010f,  0.788346f,  0.803208f,  0.817585f,
        0.831470f,  0.844854f,  0.857729f,  0.870087f,  0.881921f,  0.893224f,  0.903989f,  0.914210f,
        0.923880f,  0.932993f,  0.941544f,  0.949528f,  0.956940f,  0.963776f,  0.970031f,  0.975702f,
        0.980785f,  0.985278f,  0.989177f,  0.992480f,  0.995185f,  0.997290f,  0.998795f,  0.999699f,
        1.000000f,  0.999699f,  0.998795f,  0.997290f,  0.995185f,  0.992480f,  0.989177f,  0.985278f,
        0.980785f,  0.975702f,  0.970031f,  0.963776f,  0.956940f,  0.949528f,  0.941544f,  0.932993f,
        0.923880f,  0.914210f,  0.903989f,  0.893224f,  0.881921f,  0.870087f,  0.857729f,  0.844854f,
        0.831470f,  0.817585f,  0.803208f,  0.788346f,  0.773010f,  0.757209f,  0.740951f,  0.724247f,
        0.707107f,  0.689541f,  0.671559f,  0.653173f,  0.634393f,  0.615232f,  0.595699f,  0.575808f,
        0.555570f,  0.534998f,  0.514103f,  0.492898f,  0.471397f,  0.449611f,  0.427555f,  0.405241f,
        0.382683f,  0.359895f,  0.336890f,  0.313682f,  0.290285f,  0.266713f,  0.242980f,  0.219101f,
        0.195090f,  0.170962f,  0.146730f,  0.122411f,  0.098017f,  0.073565f,  0.049068f,  0.024541f,
    };
    return table;
}

// Linearly interpolated sine LUT lookup. ph in [0, 1).
// Uses sin(x + π) = -sin(x) to recover the negative half-cycle from the 128-entry table.
inline float osc_sine(float ph) {
    float        idx  = ph * 256.0f;
    int          i    = (int)idx;
    int          i0   = i & 127;
    int          i1   = (i0 + 1) & 127;
    float        frac = idx - (float)i;
    const float* t    = sine_lut();
    float        val  = t[i0] + frac * (t[i1] - t[i0]);
    return (i & 128) ? -val : val;
}

// Evaluate a single waveform shape at phase ph.
// shapeIdx: 0=SINE, 1=TRIANGLE, 2=PULSE, 3=SAW  (morph ordering)
inline float osc_shape(float ph, int shapeIdx, float pulseWidth) {
    switch (shapeIdx) {
        case 0: return osc_sine(ph);
        case 1: return 1.0f - 4.0f * std::fabs(ph - 0.5f);
        case 2: return (ph < pulseWidth) ? 1.0f : -1.0f;
        case 3: return 2.0f * ph - 1.0f;
        default: return 0.0f;
    }
}

} // namespace detail

// ---------------------------------------------------------------------------
// Oscillator
// Phase-accumulator oscillator with four waveform shapes.
// Sine uses the shared 128-entry LUT; triangle, saw, and pulse are analytical.
// Output is in [-1, 1].
// ---------------------------------------------------------------------------
struct Oscillator {
    enum Shape : uint8_t { SINE, TRIANGLE, SAW, PULSE };

    float phase;  // Current phase in [0, 1)

    Oscillator() : phase(0.0f) {}

    void reset() { phase = 0.0f; }

    // Set phase directly. p is wrapped to [0, 1).
    void setPhase(float p) {
        p -= std::floor(p);
        phase = p;
    }

    // Process one sample. Advances phase and returns the output in [-1, 1].
    //   freqHz:     oscillator frequency in Hz
    //   sampleTime: 1.0f / sampleRate
    //   shape:      SINE, TRIANGLE, SAW, or PULSE
    //   pulseWidth: duty cycle for PULSE shape, in (0, 1); default 0.5
    float process(float freqHz, float sampleTime, Shape shape, float pulseWidth = 0.5f) {
        phase += freqHz * sampleTime;
        if (phase >= 1.0f) phase -= 1.0f;
        return _output(phase, shape, pulseWidth);
    }

private:
    static float _output(float ph, Shape shape, float pulseWidth) {
        switch (shape) {
            case SINE:     return detail::osc_sine(ph);
            case TRIANGLE: return 1.0f - 4.0f * std::fabs(ph - 0.5f);
            case SAW:      return 2.0f * ph - 1.0f;
            case PULSE:    return (ph < pulseWidth) ? 1.0f : -1.0f;
            default:       return 0.0f;
        }
    }
};

// ---------------------------------------------------------------------------
// MorphingOscillator
// Phase-accumulator oscillator that crossfades between four waveforms:
//   sine (0) → triangle (1) → pulse (2) → saw (3)
//
// The `morph` parameter in [0, 3] selects and blends adjacent shapes:
//   morph = 0.0  → pure sine
//   morph = 1.0  → pure triangle
//   morph = 2.0  → pure pulse
//   morph = 3.0  → pure saw
//   morph = 2.8  → 20% pulse + 80% saw
// ---------------------------------------------------------------------------
struct MorphingOscillator {
    float phase;  // Current phase in [0, 1)

    MorphingOscillator() : phase(0.0f) {}

    void reset() { phase = 0.0f; }

    // Set phase directly. p is wrapped to [0, 1).
    void setPhase(float p) {
        p -= std::floor(p);
        phase = p;
    }

    // Process one sample. Advances phase and returns the crossfaded output in [-1, 1].
    //   freqHz:     oscillator frequency in Hz
    //   sampleTime: 1.0f / sampleRate
    //   morph:      waveform position in [0, 3]; clamped at the boundaries
    //   pulseWidth: duty cycle for the pulse shape, in (0, 1); default 0.5
    float process(float freqHz, float sampleTime, float morph, float pulseWidth = 0.5f) {
        phase += freqHz * sampleTime;
        if (phase >= 1.0f) phase -= 1.0f;

        if (morph <= 0.0f) return detail::osc_shape(phase, 0, pulseWidth);
        if (morph >= 3.0f) return detail::osc_shape(phase, 3, pulseWidth);

        int   base = (int)morph;
        float frac = morph - (float)base;

        float a = detail::osc_shape(phase, base,     pulseWidth);
        float b = detail::osc_shape(phase, base + 1, pulseWidth);
        return a + frac * (b - a);
    }
};

} // namespace ataraxic_dsp
