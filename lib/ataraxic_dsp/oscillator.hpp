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

// Evaluate a single waveform shape at phase ph with a timbre parameter.
// shapeIdx: 0=SINE, 1=TRIANGLE, 2=PULSE, 3=SAW  (morph ordering)
// timbre in [0, 1]; 0.5 is the neutral/default for all shapes.
//
//   SINE  (0): Phase distortion (Casio CZ style). Warps the phase before the LUT
//              lookup by splitting the cycle at `timbre`. timbre=0.5 → pure sine;
//              toward 0 or 1 → asymmetric waveform with added harmonics.
//
//   TRIANGLE (1): Variable slope. Controls the rise/fall breakpoint.
//              timbre=0.5 → symmetric triangle; toward 0 → ramp-down shape
//              (fast fall, slow rise); toward 1 → ramp-up/saw shape.
//
//   PULSE (2): Pulse width. timbre = duty cycle; 0.5 → square wave.
//
//   SAW  (3): Waveshaper. timbre=0.5 → pure saw.
//             toward 1 → tanh saturation (ramp clips toward a square wave — brighter).
//             toward 0 → S-curve softening via sin(π/2·saw) (rounded ramp — darker).
inline float osc_shape(float ph, int shapeIdx, float timbre) {
    switch (shapeIdx) {
        case 0: {
            // Phase distortion: warp phase before LUT lookup.
            // Clamp timbre away from 0/1 to avoid division by zero.
            float t = timbre < 0.001f ? 0.001f : (timbre > 0.999f ? 0.999f : timbre);
            float pd = (ph < t) ? (0.5f * ph / t)
                                : (0.5f + 0.5f * (ph - t) / (1.0f - t));
            return osc_sine(pd);
        }
        case 1: {
            // Variable slope triangle.
            float t = timbre < 0.001f ? 0.001f : (timbre > 0.999f ? 0.999f : timbre);
            return (ph < t) ? (-1.0f + 2.0f * (ph / t))
                            : ( 1.0f - 2.0f * ((ph - t) / (1.0f - t)));
        }
        case 2:
            // Pulse width modulation.
            return (ph < timbre) ? 1.0f : -1.0f;
        case 3: {
            float saw = 2.0f * ph - 1.0f;  // pure saw in [-1, 1]
            if (timbre >= 0.5f) {
                // Tanh saturation: clips toward a square wave (brighter).
                float dist  = (timbre - 0.5f) * 2.0f;   // [0, 1]
                float drive = dist * dist * 80.0f;        // [0, 80]
                if (drive < 0.001f) return saw;
                return std::tanh(drive * saw) / std::tanh(drive);
            } else {
                // S-curve softening: sin(π/2·saw) bows the ramp inward (darker).
                float soft     = (0.5f - timbre) * 2.0f; // [0, 1]
                float soft_saw = std::sin(saw * 1.5707963f);
                return (1.0f - soft) * saw + soft * soft_saw;
            }
        }
        default:
            return 0.0f;
    }
}

// Through-zero-safe phase wrap. Handles positive and negative phase increments.
// Equivalent to p - floor(p); always returns [0, 1).
inline float wrapPhase(float p) {
    return p - std::floor(p);
}

// ---------------------------------------------------------------------------
// Super saw helpers
// 7-oscillator super saw: one center accumulator + 6 detuned satellites.
// Detune offsets are three symmetric pairs, JP-8000-inspired.
// ---------------------------------------------------------------------------

// Relative-frequency detune offsets for the 6 satellite oscillators.
inline const float* super_saw_detune_table() {
    static const float kDetune[6] = {
        -0.01152f, +0.01152f,
        -0.01878f, +0.01878f,
        -0.06858f, +0.06858f,
    };
    return kDetune;
}

// Advance 6 satellite phase accumulators (non-TZ).
// detuneScale = timbre * 2: 0 = unison (pure saw), 1 = standard spread, 2 = wide.
inline void super_saw_advance(float freqHz, float sampleTime, float detuneScale, float superPh[6]) {
    const float* d = super_saw_detune_table();
    for (int i = 0; i < 6; i++) {
        superPh[i] += freqHz * (1.0f + d[i] * detuneScale) * sampleTime;
        if (superPh[i] >= 1.0f) superPh[i] -= 1.0f;
    }
}

// Advance 6 satellite phase accumulators (through-zero FM).
inline void super_saw_advance_tz(float instFreqHz, float sampleTime, float detuneScale, float superPh[6]) {
    const float* d = super_saw_detune_table();
    for (int i = 0; i < 6; i++) {
        superPh[i] = wrapPhase(superPh[i] + instFreqHz * (1.0f + d[i] * detuneScale) * sampleTime);
    }
}

// Mix center + 6 satellite saws and apply dynamic RMS gain compensation.
//
// rmsSq is a persistent one-pole RMS² estimate (caller owns the state, init to 1/3).
// It tracks the actual raw output level with a ~100 ms time constant so that gain
// responds correctly regardless of how timbre changes over time:
//   - Oscillators just spreading from unison → gain climbs gradually as RMS falls.
//   - Oscillators already spread, timbre swept back toward 0 → gain follows the real
//     phase distribution rather than the theoretical steady-state value.
// Target RMS is 1/√3 (same as saw/triangle reference). Output may transiently exceed
// ±1 when oscillator phases briefly realign after being spread.
inline float super_saw_normalized(float ph, const float superPh[6],
                                   float sampleTime, float& rmsSq) {
    float raw = 2.0f * ph - 1.0f;
    for (int i = 0; i < 6; i++) raw += 2.0f * superPh[i] - 1.0f;
    raw *= (1.0f / 7.0f);
    // One-pole RMS² follower: tau ≈ 100 ms at any sample rate.
    float alpha = sampleTime * 10.0f;
    if (alpha > 0.1f) alpha = 0.1f;
    rmsSq = rmsSq + alpha * (raw * raw - rmsSq);
    // Gain = target_rms / measured_rms; guard against near-zero.
    float rms  = std::sqrt(rmsSq > 1e-8f ? rmsSq : 1e-8f);
    return raw * (0.5774f / rms);  // 0.5774 = 1/√3
}

} // namespace detail

// ---------------------------------------------------------------------------
// FM frequency helpers
//
// Compute an instantaneous frequency in Hz to pass to process() or processTZ().
// fmCV is caller-normalized — the library assumes no specific voltage standard.
// Example: divide Eurorack ±5 V by 5.0f to get fmCV in [-1, 1].
// ---------------------------------------------------------------------------

// Linear FM. Instantaneous frequency is clamped to 0 Hz minimum (no backward phase).
// depth: Hz per unit of fmCV (e.g. 100.0f → ±100 Hz at fmCV = ±1).
inline float fmLinear(float baseHz, float fmCV, float depth) {
    float f = baseHz + fmCV * depth;
    return f < 0.0f ? 0.0f : f;
}

// Exponential FM. 1V/oct style — pitch intervals are preserved regardless of carrier pitch.
// depth: octaves per unit of fmCV (e.g. 1.0f → ±1 octave at fmCV = ±1).
inline float fmExp(float baseHz, float fmCV, float depth) {
    return baseHz * std::pow(2.0f, fmCV * depth);
}

// Through-zero FM. Allows negative instantaneous frequency — phase runs backwards.
// Pass the result to processTZ(), not process().
// depth: Hz per unit of fmCV.
inline float fmThroughZero(float baseHz, float fmCV, float depth) {
    return baseHz + fmCV * depth;
}

// ---------------------------------------------------------------------------
// Oscillator
// Phase-accumulator oscillator with five waveform shapes.
// Sine uses the shared 128-entry LUT; triangle, saw, pulse, and super saw are analytical.
// Output is in [-1, 1].
// ---------------------------------------------------------------------------
struct Oscillator {
    enum Shape : uint8_t { SINE, TRIANGLE, SAW, PULSE, SUPER_SAW };

    float phase;            // Current phase in [0, 1)
    float superPhase[6];    // Satellite phase accumulators for SUPER_SAW
    float superRmsSq;       // One-pole RMS² state for SUPER_SAW gain tracking

    Oscillator() : phase(0.0f), superRmsSq(1.0f / 3.0f) {
        // Start satellites in unison with the center so timbre=0 gives a pure saw.
        for (int i = 0; i < 6; i++) superPhase[i] = 0.0f;
    }

    void reset() {
        phase     = 0.0f;
        superRmsSq = 1.0f / 3.0f;
        for (int i = 0; i < 6; i++) superPhase[i] = 0.0f;
    }

    // Set phase directly. p is wrapped to [0, 1).
    void setPhase(float p) {
        p -= std::floor(p);
        phase = p;
    }

    // Process one sample. Advances phase and returns the output in [-1, 1].
    //   freqHz:     oscillator frequency in Hz
    //   sampleTime: 1.0f / sampleRate
    //   shape:      SINE, TRIANGLE, SAW, PULSE, or SUPER_SAW
    //   timbre:     shape-specific timbral control in [0, 1]; 0.5 is neutral for all shapes
    float process(float freqHz, float sampleTime, Shape shape, float timbre = 0.5f) {
        phase += freqHz * sampleTime;
        if (phase >= 1.0f) phase -= 1.0f;
        if (shape == SUPER_SAW) {
            detail::super_saw_advance(freqHz, sampleTime, timbre * 2.0f, superPhase);
            return detail::super_saw_normalized(phase, superPhase, sampleTime, superRmsSq);
        }
        return _output(phase, shape, timbre);
    }

    // Process one sample with through-zero FM.
    //   instFreqHz: signed instantaneous frequency from fmThroughZero(). May be negative.
    //               Negative Hz causes the phase accumulator to run backwards.
    float processTZ(float instFreqHz, float sampleTime, Shape shape, float timbre = 0.5f) {
        phase += instFreqHz * sampleTime;
        phase  = detail::wrapPhase(phase);
        if (shape == SUPER_SAW) {
            detail::super_saw_advance_tz(instFreqHz, sampleTime, timbre * 2.0f, superPhase);
            return detail::super_saw_normalized(phase, superPhase, sampleTime, superRmsSq);
        }
        return _output(phase, shape, timbre);
    }

private:
    // Map Oscillator::Shape enum to detail::osc_shape indices.
    // Oscillator enum: SINE=0, TRIANGLE=1, SAW=2, PULSE=3  (SUPER_SAW handled before this)
    // osc_shape index: SINE=0, TRIANGLE=1, PULSE=2, SAW=3  (morph ordering)
    static float _output(float ph, Shape shape, float timbre) {
        switch (shape) {
            case SINE:     return detail::osc_shape(ph, 0, timbre);
            case TRIANGLE: return detail::osc_shape(ph, 1, timbre);
            case SAW:      return detail::osc_shape(ph, 3, timbre);
            case PULSE:    return detail::osc_shape(ph, 2, timbre);
            default:       return 0.0f;
        }
    }
};

// ---------------------------------------------------------------------------
// MorphingOscillator
// Phase-accumulator oscillator that crossfades between five waveforms:
//   sine (0) → triangle (1) → pulse (2) → saw (3) → super saw (4)
//
// The `morph` parameter in [0, 4] selects and blends adjacent shapes:
//   morph = 0.0  → pure sine
//   morph = 1.0  → pure triangle
//   morph = 2.0  → pure pulse
//   morph = 3.0  → pure saw
//   morph = 4.0  → pure super saw
//   morph = 3.5  → 50% saw + 50% super saw (constant-power crossfade)
// ---------------------------------------------------------------------------
struct MorphingOscillator {
    float phase;            // Current phase in [0, 1)
    float superPhase[6];    // Satellite phase accumulators for super saw
    float superRmsSq;       // One-pole RMS² state for super saw gain tracking

    MorphingOscillator() : phase(0.0f), superRmsSq(1.0f / 3.0f) {
        for (int i = 0; i < 6; i++) superPhase[i] = 0.0f;
    }

    void reset() {
        phase      = 0.0f;
        superRmsSq = 1.0f / 3.0f;
        for (int i = 0; i < 6; i++) superPhase[i] = 0.0f;
    }

    // Set phase directly. p is wrapped to [0, 1).
    void setPhase(float p) {
        p -= std::floor(p);
        phase = p;
    }

    // Process one sample. Advances phase and returns the crossfaded output in [-1, 1].
    //   freqHz:     oscillator frequency in Hz
    //   sampleTime: 1.0f / sampleRate
    //   morph:      waveform position in [0, 4]; clamped at the boundaries
    //   timbre:     shape-specific timbral control in [0, 1]; 0.5 is neutral for all shapes
    float process(float freqHz, float sampleTime, float morph, float timbre = 0.5f) {
        phase += freqHz * sampleTime;
        if (phase >= 1.0f) phase -= 1.0f;
        detail::super_saw_advance(freqHz, sampleTime, timbre * 2.0f, superPhase);
        return _morphOutput(phase, morph, timbre, superPhase, sampleTime, superRmsSq);
    }

    // Process one sample with through-zero FM.
    //   instFreqHz: signed instantaneous frequency from fmThroughZero(). May be negative.
    //               Negative Hz causes the phase accumulator to run backwards.
    float processTZ(float instFreqHz, float sampleTime, float morph, float timbre = 0.5f) {
        phase += instFreqHz * sampleTime;
        phase  = detail::wrapPhase(phase);
        detail::super_saw_advance_tz(instFreqHz, sampleTime, timbre * 2.0f, superPhase);
        return _morphOutput(phase, morph, timbre, superPhase, sampleTime, superRmsSq);
    }

private:
    // Per-shape RMS normalization gains (at neutral timbre = 0.5):
    //   Sine:      RMS = 1/√2  → gain = √(2/3) ≈ 0.8165
    //   Triangle:  RMS = 1/√3  → gain = 1.0 (reference)
    //   Pulse:     RMS = 1.0   → gain = 1/√3  ≈ 0.5774
    //   Saw:       RMS = 1/√3  → gain = 1.0
    //   Super saw: gain = 1.0; RMS compensation is applied by super_saw_normalized() per-sample.
    // Normalizing to triangle/saw keeps all peak outputs within [-1, 1].
    static float _shapeGain(int idx) {
        static const float kGain[5] = { 0.8165f, 1.0f, 0.5774f, 1.0f, 1.0f };
        return kGain[idx];
    }

    static float _morphOutput(float ph, float morph, float timbre, const float superPh[6],
                               float sampleTime, float& superRmsSq) {
        if (morph <= 0.0f) return _shapeGain(0) * detail::osc_shape(ph, 0, timbre);
        if (morph >= 4.0f)
            return _shapeGain(4) * detail::super_saw_normalized(ph, superPh, sampleTime, superRmsSq);
        if (morph >= 3.0f) {
            // Crossfade: saw → super saw
            float frac = morph - 3.0f;
            float ga   = std::cos(frac * (3.14159265f * 0.5f));
            float gb   = std::sin(frac * (3.14159265f * 0.5f));
            float ss   = detail::super_saw_normalized(ph, superPh, sampleTime, superRmsSq);
            return ga * (_shapeGain(3) * detail::osc_shape(ph, 3, timbre))
                 + gb * (_shapeGain(4) * ss);
        }
        int   base = (int)morph;
        float frac = morph - (float)base;
        // Constant-power crossfade: maintains RMS through the blend.
        float ga   = std::cos(frac * (3.14159265f * 0.5f));
        float gb   = std::sin(frac * (3.14159265f * 0.5f));
        float a    = _shapeGain(base)     * detail::osc_shape(ph, base,     timbre);
        float b    = _shapeGain(base + 1) * detail::osc_shape(ph, base + 1, timbre);
        return ga * a + gb * b;
    }
};

} // namespace ataraxic_dsp
