# ataraxic_dsp API Reference

Portable, header-only C++11 DSP library. Zero external dependencies (`<cmath>` and `<stdint.h>` only). No heap allocation — suitable for VCV Rack modules, RP2350, and STM32 targets.

Single include: `#include "ataraxic_dsp/ataraxic_dsp.hpp"`

All symbols are in the `ataraxic_dsp` namespace.

---

## RNG Injection (`noise.hpp`)

All noise generators that require randomness accept an injected RNG rather than calling a global function. This keeps the library portable across VCV Rack, embedded hardware RNGs, and test harnesses.

```cpp
typedef float (*RngFn)(void* ctx);
```

A function pointer that returns a uniform float in `[0.0, 1.0)`. The `ctx` pointer is passed through unchanged — use it to carry RNG state (e.g. a pointer to `std::mt19937`).

**VCV Rack:**
```cpp
static float rackRng(void*) { return rack::random::uniform(); }
```

**Desktop test / noise_sim:**
```cpp
static std::mt19937 gen(12345);
static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
static float testRng(void*) { return dist(gen); }
```

**RP2350 (Pico SDK):**
```cpp
static float picoRng(void*) {
    return (float)(get_rand_32() >> 8) / (float)(1 << 24);
}
```

---

## SchmittTrigger (`schmitt_trigger.hpp`)

Schmitt trigger with hysteresis. Returns `true` on a LOW→HIGH transition (rising edge). Direct port of `rack::dsp::SchmittTrigger`.

```cpp
struct SchmittTrigger {
    void reset();
    bool process(float in, float lowThreshold = 0.0f, float highThreshold = 1.0f);
    bool isHigh() const;
};
```

| Member | Description |
|--------|-------------|
| `reset()` | Returns to UNINITIALIZED state. The next `process()` call will set the initial state without firing a rising-edge event. |
| `process(in, low, high)` | Feed the current input voltage. Returns `true` only on the LOW→HIGH transition. Defaults match a 0–1 V gate signal; use `(0.1f, 2.0f)` for Eurorack 5 V gates. |
| `isHigh()` | Returns `true` if currently in the HIGH state. |

**Example:**
```cpp
ataraxic_dsp::SchmittTrigger trig;

// In process loop:
if (trig.process(gateVoltage)) {
    // rising edge detected
}
```

---

## EnvelopeADAR (`envelope.hpp`)

Linear-ramp AD/AR envelope generator. Supports both one-shot (Attack-Decay) and gated (Attack-Release) modes.

```cpp
struct EnvelopeADAR {
    float output;   // current output, 0.0 to 1.0 (read-only)

    void  reset();
    void  triggerAD();
    void  triggerAR();
    float process(float attackRate, float decayRate, bool gate);
};
```

| Member | Description |
|--------|-------------|
| `output` | Current envelope value in `[0, 1]`. Readable at any time; also returned by `process()`. |
| `reset()` | Forces envelope to OFF state with output 0. |
| `triggerAD()` | Starts a one-shot AD envelope. Ignores gate level. Sequence: ATTACK → DECAY_RELEASE → OFF. |
| `triggerAR()` | Starts a gated AR envelope. Call on the rising edge of a gate. Sequence: ATTACK → SUSTAIN (while gate is high) → DECAY_RELEASE → OFF. |
| `process(attackRate, decayRate, gate)` | Advances the envelope by one sample. Returns the new output value. |

**Rate calculation:**
```cpp
float attackRate = 1.0f / (attackTimeSecs * sampleRate);
float decayRate  = 1.0f / (decayTimeSecs  * sampleRate);
```

**Example:**
```cpp
ataraxic_dsp::EnvelopeADAR env;
ataraxic_dsp::SchmittTrigger trig;

// In process loop:
if (trig.process(trigInput))
    env.triggerAD();

float out = env.process(attackRate, decayRate, /*gate=*/false);
// out is 0.0–1.0; scale to voltage: out * 10.f
```

---

## advcaScaleTime (`envelope.hpp`)

Cubic "log-pot" time scaling. Maps a knob parameter in `[0, 1]` to a time in seconds using an x³ curve. Compared to a linear mapping, this spreads short times out more finely while compressing the upper range — similar to a logarithmic potentiometer.

```cpp
inline float advcaScaleTime(float param, float baseTime, float maxTime);
```

| Parameter | Description |
|-----------|-------------|
| `param` | Knob value in `[0, 1]`. |
| `baseTime` | Minimum time in seconds (e.g. `0.001f` = 1 ms). |
| `maxTime` | Maximum time in seconds (e.g. `2.0f` for attack, `10.0f` for decay). |

Returns time in seconds.

**Example:**
```cpp
float attackTime = ataraxic_dsp::advcaScaleTime(knobValue, 0.001f, 2.0f);
float attackRate = 1.0f / (attackTime * sampleRate);
```

---

## VCA (`vca.hpp`)

Stateless voltage-controlled amplifier with a blendable gain response curve. All methods are `static`.

```cpp
struct VCA {
    static float computeGain(float gainNorm, float responseMix);
    static float process(float in, float gainNorm, float responseMix);
};
```

| Parameter | Description |
|-----------|-------------|
| `gainNorm` | CV normalized to `[0, 1]` (e.g. `clamp(voltage / 10.f, 0.f, 1.f)`). |
| `responseMix` | `0.0` = exponential (x⁴, punchy), `0.5` = linear, `1.0` = logarithmic (x^0.25). Values between these crossfade smoothly. |

`computeGain` returns a gain multiplier in `[0, 1]`. `process` returns `in * computeGain(...)`.

**Example:**
```cpp
float gain = ataraxic_dsp::VCA::computeGain(gainNorm, responseMix);
float audioOut = audioIn * gain;
```

---

## OnePole (`onepole.hpp`)

One-pole lowpass filter with a free highpass tap (`HP = input - LP`). Suitable for tone-shaping, DC blocking, and envelope smoothing.

```cpp
struct OnePole {
    float lp;   // LP integrator state (readable)

    void  reset();
    float processLP(float in, float g);
    static float computeG(float cutoffHz, float sampleTime);
};
```

| Member | Description |
|--------|-------------|
| `reset()` | Clears the integrator state to 0. |
| `processLP(in, g)` | Advances the filter by one sample. Returns the LP output. HP output = `in - result`. |
| `computeG(cutoffHz, sampleTime)` | Computes the filter coefficient `g` from a cutoff frequency. Result is clamped to `[0, 1]`. |

**Example:**
```cpp
ataraxic_dsp::OnePole filter;

float g  = ataraxic_dsp::OnePole::computeG(1000.f, 1.f / sampleRate);
float lp = filter.processLP(input, g);
float hp = input - lp;
```

---

## Noise Generators (`noise.hpp`)

### WhiteNoise

Uniform white noise in `[-1, 1)` via an injected RNG.

```cpp
struct WhiteNoise {
    void  init(RngFn rngFn, void* ctx = 0);
    float next();       // returns [-1, 1)
    float uniform();    // returns [0, 1)
};
```

`uniform()` is a raw RNG draw without the `[-1, 1]` mapping — used internally by `UrusaiDsp` for probability checks.

---

### PinkFilter

7-stage cascaded IIR filter producing approximately −3 dB/octave (1/f) pink noise from a white noise input. Coefficients from the Voss-McCartney / Isao Yamaha method.

```cpp
struct PinkFilter {
    void  reset();
    float process(float white);   // feed white noise, returns pink noise
};
```

Output amplitude is roughly 1/5 of the input white noise amplitude before gain compensation.

---

### CmosNoise

17-bit Galois LFSR modelling the MM5837 chip used in analog synthesizers. Tap positions: bits 16 and 13.

```cpp
struct CmosNoise {
    void  reset();
    float process(float character, float sampleTime);   // returns -1.0 or +1.0
};
```

`character` in `[0, 1]` sweeps the LFSR clock rate from 1 kHz (`character=0`) to ~63 kHz (`character=1`). `sampleTime = 1.0f / sampleRate`.

---

### EightBitNoise

15-bit Galois LFSR modelling the NES noise channel. Tap positions: bits 14 and 13.

```cpp
struct EightBitNoise {
    void  reset();
    float process(float character, float sampleTime);   // returns -1.0 or +1.0
};
```

`character` in `[0, 1]` sweeps the LFSR clock rate from 10 Hz (`character=0`) to 10 kHz (`character=1`).

---

### BlueFilter

First-order differencer applied to pink noise, producing a +3 dB/octave spectral slope. Feed it one pink noise sample per tick.

```cpp
struct BlueFilter {
    void  reset();
    float process(float pink);   // feed pink noise, returns blue noise (gain 10×)
};
```

---

### VioletFilter

First-order differencer applied to white noise, producing a +6 dB/octave spectral slope. Feed it one white noise sample per tick.

```cpp
struct VioletFilter {
    void  reset();
    float process(float white);  // feed white noise, returns violet noise (gain 0.707×)
};
```

---

### VelvetNoise

Sparse random impulses (+1 or −1) at a rate controlled by `character`. Requires an injected RNG for the probability check; the sign is determined by the caller-supplied white noise sample so the per-sample white cache can be shared.

```cpp
struct VelvetNoise {
    void  init(RngFn rngFn, void* ctx = 0);
    float process(float character, float sampleTime, float white);
};
```

`character` in `[0, 1]` sweeps impulse rate from 10 Hz to 10 kHz. `white` is the current white noise sample (used for sign only).

---

## Gain Calibration Constants (`noise.hpp`)

RMS-equalized output gain multipliers for each noise type, calibrated so all outputs have the same perceived loudness.

| Constant | Value | Noise type |
|----------|-------|------------|
| `NOISE_GAIN_WHITE`  | 5.0   | White noise |
| `NOISE_GAIN_PINK`   | 15.3  | Pink noise |
| `NOISE_GAIN_BLUE`   | 2.5   | Blue noise |
| `NOISE_GAIN_VIOLET` | 5.0   | Violet noise |
| `NOISE_GAIN_VELVET` | 11.0  | Velvet noise |
| `NOISE_GAIN_CMOS`   | 2.88  | CMOS noise |
| `NOISE_GAIN_8BIT`   | 2.88  | 8-bit noise |

Multiply raw generator output by the corresponding constant before sending to a DAC or output port. With these gains applied, each output produces approximately the same RMS level as white noise at `NOISE_GAIN_WHITE` (≈ 2.9 V RMS).

---

## Embedded CMake Integration

```cmake
add_subdirectory(path/to/AtaraxicModules/lib)
target_link_libraries(my_firmware PRIVATE ataraxic_dsp)
# Also link -lm for std::pow() on newlib targets
```

Then include:
```cpp
#include "ataraxic_dsp/ataraxic_dsp.hpp"
```

RAM budget for `EnvelopeADAR`: ~10 bytes.
