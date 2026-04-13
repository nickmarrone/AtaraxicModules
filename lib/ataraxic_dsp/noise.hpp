#pragma once
#include <cmath>
#include <stdint.h>

namespace ataraxic_dsp {

// RNG injection: function pointer returning a uniform float in [0.0, 1.0).
// Use a plain function pointer + void* context to stay heap-free and
// compatible with bare-metal newlib (no std::function, no virtuals).
typedef float (*RngFn)(void* ctx);

// ---------------------------------------------------------------------------
// White Noise
// Generates uniform white noise in [-1, 1) using an injected RNG.
// ---------------------------------------------------------------------------
struct WhiteNoise {
    RngFn  rng;
    void*  rng_ctx;

    WhiteNoise() : rng(0), rng_ctx(0) {}

    void init(RngFn rngFn, void* ctx = 0) {
        rng     = rngFn;
        rng_ctx = ctx;
    }

    // Returns one white noise sample in [-1, 1).
    float next() {
        return rng(rng_ctx) * 2.0f - 1.0f;
    }

    // Returns one uniform sample in [0, 1) without the [-1,1] mapping.
    float uniform() {
        return rng(rng_ctx);
    }
};

// ---------------------------------------------------------------------------
// Pink Noise
// 7-stage cascaded IIR filter applied to white noise.
// Produces approximately -3 dB/octave (1/f) spectral slope.
// Filter coefficients from the Voss-McCartney / Isao Yamaha method.
// ---------------------------------------------------------------------------
struct PinkFilter {
    float b0, b1, b2, b3, b4, b5, b6;

    PinkFilter() : b0(0), b1(0), b2(0), b3(0), b4(0), b5(0), b6(0) {}

    void reset() { b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f; }

    // Pass in a white noise sample, returns a pink noise sample.
    float process(float white) {
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f;
    }
};

// ---------------------------------------------------------------------------
// CMOS Noise
// 17-bit Galois LFSR (models the MM5837 chip used in analog synthesizers).
// Tap positions: 17 and 14 (bit indices 16 and 13).
// Clock rate swept via character parameter.
// ---------------------------------------------------------------------------
struct CmosNoise {
    uint32_t lfsr;
    float    phase;

    CmosNoise() : lfsr(0xACE1u), phase(0.0f) {}

    void reset() { lfsr = 0xACE1u; phase = 0.0f; }

    // character in [0, 1] sweeps LFSR clock from 1 kHz to ~63 kHz.
    // sampleTime = 1 / sampleRate.
    // Returns -1.0 or +1.0.
    float process(float character, float sampleTime) {
        float rate = std::pow(10.0f, 3.0f + 1.8f * character);
        phase += rate * sampleTime;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            unsigned bit = ((lfsr >> 13) ^ (lfsr >> 16)) & 1u;
            lfsr = (lfsr << 1) | bit;
        }
        return ((lfsr >> 16) & 1u) ? 1.0f : -1.0f;
    }
};

// ---------------------------------------------------------------------------
// 8-Bit Noise
// 15-bit Galois LFSR (NES-style noise channel).
// Tap positions: 14 and 13 (bit indices 14 and 13).
// Clock rate swept via character parameter.
// ---------------------------------------------------------------------------
struct EightBitNoise {
    uint32_t lfsr;
    float    phase;

    EightBitNoise() : lfsr(0xACE1u), phase(0.0f) {}

    void reset() { lfsr = 0xACE1u; phase = 0.0f; }

    // character in [0, 1] sweeps LFSR clock from 10 Hz to 10 kHz.
    // sampleTime = 1 / sampleRate.
    // Returns -1.0 or +1.0.
    float process(float character, float sampleTime) {
        float rate = std::pow(10.0f, 1.0f + 3.0f * character);
        phase += rate * sampleTime;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            unsigned bit = ((lfsr >> 13) ^ (lfsr >> 14)) & 1u;
            lfsr = (lfsr << 1) | bit;
        }
        return ((lfsr >> 14) & 1u) ? 1.0f : -1.0f;
    }
};

// ---------------------------------------------------------------------------
// BlueFilter
// First-order differencer applied to pink noise, producing +3 dB/octave slope.
// Feed it the pink noise sample each tick; it stores the previous value.
// ---------------------------------------------------------------------------
struct BlueFilter {
    float _lastPink;

    BlueFilter() : _lastPink(0) {}

    void reset() { _lastPink = 0.0f; }

    // Feed a pink noise sample, returns a blue noise sample (gain 10×).
    float process(float pink) {
        float blue = (pink - _lastPink) * 10.0f;
        _lastPink  = pink;
        return blue;
    }
};

// ---------------------------------------------------------------------------
// VioletFilter
// First-order differencer applied to white noise, producing +6 dB/octave slope.
// Feed it the white noise sample each tick; it stores the previous value.
// ---------------------------------------------------------------------------
struct VioletFilter {
    float _lastWhite;

    VioletFilter() : _lastWhite(0) {}

    void reset() { _lastWhite = 0.0f; }

    // Feed a white noise sample, returns a violet noise sample (gain 0.707×).
    float process(float white) {
        float violet = (white - _lastWhite) * 0.707f;
        _lastWhite   = white;
        return violet;
    }
};

// ---------------------------------------------------------------------------
// VelvetNoise
// Sparse random impulses (+1 or -1) at a rate swept by the character parameter.
// Uses an injected RNG for probability checks; the sign is determined by the
// caller-supplied white noise sample so it can share the per-sample cache.
// ---------------------------------------------------------------------------
struct VelvetNoise {
    RngFn rng;
    void* rng_ctx;

    VelvetNoise() : rng(0), rng_ctx(0) {}

    void init(RngFn rngFn, void* ctx = 0) {
        rng     = rngFn;
        rng_ctx = ctx;
    }

    // character in [0,1] sweeps impulse rate from 10 Hz to 10 kHz.
    // white: current white noise sample used for sign (+1/-1).
    // sampleTime = 1 / sampleRate.
    float process(float character, float sampleTime, float white) {
        float rate = std::pow(10.0f, 1.0f + 3.0f * character);
        float p    = rate * sampleTime;
        if (rng(rng_ctx) < p) {
            return (white > 0.0f) ? 1.0f : -1.0f;
        }
        return 0.0f;
    }
};

// Gain calibration constants for each noise type.
// These values equalize the RMS output level across all noise sources.
// Multiply the raw generator output by the corresponding constant before
// sending to a DAC or output port.
static const float NOISE_GAIN_WHITE  = 5.0f;
static const float NOISE_GAIN_PINK   = 15.3f;
static const float NOISE_GAIN_BLUE   = 2.5f;
static const float NOISE_GAIN_VIOLET = 5.0f;
static const float NOISE_GAIN_VELVET = 11.0f;
static const float NOISE_GAIN_CMOS   = 2.88f;
static const float NOISE_GAIN_8BIT   = 2.88f;

} // namespace ataraxic_dsp
