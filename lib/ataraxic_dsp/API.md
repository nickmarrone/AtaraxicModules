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

Set `lowThreshold` and `highThreshold` once at init if non-default values are needed. Default thresholds (0.0 / 1.0) suit a 0–1 V gate; use e.g. (0.1, 2.0) for Eurorack 5 V gates.

```cpp
struct SchmittTrigger {
    float lowThreshold;   // default 0.0f
    float highThreshold;  // default 1.0f

    void reset();
    bool process(float in);
    bool isHigh() const;
};
```

| Member | Description |
|--------|-------------|
| `lowThreshold` | Input level at or below which the trigger transitions HIGH→LOW. |
| `highThreshold` | Input level at or above which the trigger transitions LOW→HIGH. |
| `reset()` | Returns to UNINITIALIZED state. The next `process()` call will set the initial state without firing a rising-edge event. |
| `process(in)` | Feed the current input voltage. Returns `true` only on the LOW→HIGH transition. |
| `isHigh()` | Returns `true` if currently in the HIGH state. |

**Example:**
```cpp
ataraxic_dsp::SchmittTrigger trig;
// For Eurorack 5V gates:
// trig.lowThreshold  = 0.1f;
// trig.highThreshold = 2.0f;

// In process loop:
if (trig.process(gateVoltage)) {
    // rising edge detected
}
```

---

## EnvelopeAD (`envelope.hpp`)

One-shot linear-ramp Attack-Decay envelope. Sequence: ATTACK → DECAY → OFF.

```cpp
struct EnvelopeAD {
    float output;      // current output, 0.0 to 1.0 (read-only)
    float attackRate;  // set to 1.0f / (attackTimeSecs * sampleRate)
    float decayRate;   // set to 1.0f / (decayTimeSecs  * sampleRate)

    void  reset();
    void  trigger();
    float process();
};
```

| Member | Description |
|--------|-------------|
| `output` | Current envelope value in `[0, 1]`. Readable at any time; also returned by `process()`. |
| `attackRate` | Attack increment per sample. Update each tick if the time is CV-modulated. |
| `decayRate` | Decay decrement per sample. Update each tick if the time is CV-modulated. |
| `reset()` | Forces envelope to OFF state with output 0. Does not clear rates. |
| `trigger()` | Starts the attack phase. |
| `process()` | Advances the envelope by one sample. Returns the new output value. |

**Example:**
```cpp
ataraxic_dsp::EnvelopeAD env;
ataraxic_dsp::SchmittTrigger trig;

// In process loop:
env.attackRate = 1.0f / (attackTimeSecs * sampleRate);
env.decayRate  = 1.0f / (decayTimeSecs  * sampleRate);

if (trig.process(trigInput))
    env.trigger();

float out = env.process();
// out is 0.0–1.0; scale to voltage: out * 10.f
```

---

## EnvelopeAR (`envelope.hpp`)

Gated linear-ramp Attack-Release envelope with sustain. Sequence: ATTACK → SUSTAIN (while gate held) → RELEASE → OFF.

```cpp
struct EnvelopeAR {
    float output;       // current output, 0.0 to 1.0 (read-only)
    float attackRate;   // set to 1.0f / (attackTimeSecs  * sampleRate)
    float releaseRate;  // set to 1.0f / (releaseTimeSecs * sampleRate)

    void  reset();
    void  trigger();
    float process(bool gate);
};
```

| Member | Description |
|--------|-------------|
| `output` | Current envelope value in `[0, 1]`. Readable at any time; also returned by `process()`. |
| `attackRate` | Attack increment per sample. Update each tick if the time is CV-modulated. |
| `releaseRate` | Release decrement per sample. Update each tick if the time is CV-modulated. |
| `reset()` | Forces envelope to OFF state with output 0. Does not clear rates. |
| `trigger()` | Starts the attack phase. Call on the rising edge of the gate. |
| `process(gate)` | Advances the envelope by one sample. `gate` should be `true` while the gate signal is high. Returns the new output value. |

**Example:**
```cpp
ataraxic_dsp::EnvelopeAR env;
ataraxic_dsp::SchmittTrigger trig;

// In process loop:
env.attackRate  = 1.0f / (attackTimeSecs  * sampleRate);
env.releaseRate = 1.0f / (releaseTimeSecs * sampleRate);

bool gate = gateVoltage >= 1.0f;
if (trig.process(gateVoltage))
    env.trigger();

float out = env.process(gate);
// out is 0.0–1.0; scale to voltage: out * 10.f
```

---

## cubicPotScale (`pot.hpp`)

Maps a normalized potentiometer value in `[0, 1]` to a target range using a cubic (x³) curve. Gives a "log-pot" feel — finer resolution at the low end, compressed at the high end — without the discontinuities of a true logarithm near zero. Useful for any parameter that benefits from this response: time, frequency, gain, etc.

```cpp
inline float cubicPotScale(float param, float minVal, float maxVal);
```

| Parameter | Description |
|-----------|-------------|
| `param` | Knob/CV value in `[0, 1]`. |
| `minVal` | Output value when `param = 0` (e.g. `0.001f` for 1 ms). |
| `maxVal` | Output value when `param = 1` (e.g. `10.0f` for 10 s). |

**Example:**
```cpp
float attackTime = ataraxic_dsp::cubicPotScale(knobValue, 0.001f, 2.0f);
float attackRate = 1.0f / (attackTime * sampleRate);
```

---

## VCA (`vca.hpp`)

Voltage-controlled amplifier with a blendable gain response curve. Set `responseMix` once via `setResponse()` (e.g. on knob change); call `computeGain()` or `process()` each sample.

```cpp
struct VCA {
    void  setResponse(float responseMix);
    float computeGain(float gainNorm) const;
    float process(float in, float gainNorm) const;
};
```

| Member | Description |
|--------|-------------|
| `setResponse(responseMix)` | Pre-computes blend coefficients for the given response. `0.0` = exponential (x⁴, punchy), `0.5` = linear, `1.0` = logarithmic (x^0.25). |
| `computeGain(gainNorm)` | Returns gain in `[0, 1]`. `gainNorm` is CV normalized to `[0, 1]` (e.g. `clamp(voltage / 10.f, 0.f, 1.f)`). |
| `process(in, gainNorm)` | Returns `in * computeGain(gainNorm)`. |

**Example:**
```cpp
ataraxic_dsp::VCA vca;

// When the response knob changes:
vca.setResponse(params[RESPONSE_PARAM].getValue());

// Each sample:
float gainNorm = clamp(cvVoltage / 10.f, 0.f, 1.f);
float audioOut = vca.process(audioIn, gainNorm);
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

## Oscillator (`oscillator.hpp`)

Phase-accumulator oscillator with five waveform shapes. Sine is computed via a 128-entry LUT with linear interpolation; triangle, saw, and pulse are computed analytically; super saw runs 7 detuned accumulators. Output is in `[-1, 1]`.

```cpp
struct Oscillator {
    enum Shape : uint8_t { SINE, TRIANGLE, SAW, PULSE, SUPER_SAW };

    float phase;            // Current phase in [0, 1) (readable/writable)
    float superPhase[6];    // Satellite accumulators for SUPER_SAW (readable/writable)
    float superRmsSq;       // RMS² follower state for SUPER_SAW gain (init 1/3, reset on reset())

    void  reset();
    void  setPhase(float p);
    float process(float freqHz, float sampleTime, Shape shape, float timbre = 0.5f);
    float processTZ(float instFreqHz, float sampleTime, Shape shape, float timbre = 0.5f);
};
```

| Member | Description |
|--------|-------------|
| `reset()` | Resets all phases to their default spread. |
| `setPhase(p)` | Sets the center phase directly. `p` is wrapped to `[0, 1)`. Satellite phases are unaffected. |
| `process(freqHz, sampleTime, shape, timbre)` | Advances phase by `freqHz * sampleTime` and returns the output in `[-1, 1]`. `timbre` in `[0, 1]` shapes the waveform; `0.5` is neutral for all shapes. |
| `processTZ(instFreqHz, sampleTime, shape, timbre)` | Like `process()` but uses a floor-based phase wrap that handles negative `instFreqHz`. Feed the result of `fmThroughZero()` here. |

**Waveform shapes and timbre effect:**

| Shape | `timbre = 0` | `timbre = 0.5` (neutral) | `timbre = 1` |
|-------|-------------|--------------------------|-------------|
| `SINE` | Phase distorted early — asymmetric harmonics | Pure sine | Phase distorted late — asymmetric harmonics |
| `TRIANGLE` | Ramp-down shape (fast fall, slow rise) | Symmetric triangle | Ramp-up/saw shape (slow fall, fast rise) |
| `SAW` | S-curve ramp — bowed inward (fewer high harmonics, darker) | Pure sawtooth | Tanh-saturated ramp — approaches square wave (more harmonics, brighter) |
| `PULSE` | Narrow negative pulse | Square wave | Narrow positive pulse |
| `SUPER_SAW` | All 7 oscillators in unison — pure saw | Standard JP-8000-style spread | Wide detune — broad, chorused tone |

Gain compensation uses a one-pole RMS² follower (~100 ms time constant) so level tracks the oscillators' actual phase spread rather than a static formula. This means the gain responds correctly whether timbre is held steady, swept slowly, or snapped. Output may transiently exceed ±1 when oscillator phases briefly realign after being spread; this bloom is characteristic of the super saw sound.

**Example:**
```cpp
ataraxic_dsp::Oscillator osc;

// In process loop (sampleTime = 1.0f / sampleRate):
float out = osc.process(440.0f, sampleTime, ataraxic_dsp::Oscillator::SINE);

// Super saw with standard spread:
float ss = osc.process(440.0f, sampleTime, ataraxic_dsp::Oscillator::SUPER_SAW);

// Pulse wave with 30% duty cycle:
float pw = osc.process(440.0f, sampleTime, ataraxic_dsp::Oscillator::PULSE, 0.3f);
```

RAM budget on Cortex-M: ~28 bytes (phase + 6 satellite phases). The 128-entry LUT is `static const` in `detail::sine_lut()` — shared between `Oscillator` and `MorphingOscillator`, stored once in flash.

---

## MorphingOscillator (`oscillator.hpp`)

Phase-accumulator oscillator that continuously crossfades between five waveforms. A single `morph` parameter selects and blends adjacent shapes. Output is in `[-1, 1]`.

Waveform order: **sine → triangle → pulse → saw → super saw**

```cpp
struct MorphingOscillator {
    float phase;            // Current phase in [0, 1) (readable/writable)
    float superPhase[6];    // Satellite accumulators for super saw (readable/writable)
    float superRmsSq;       // RMS² follower state for super saw gain (init 1/3, reset on reset())

    void  reset();
    void  setPhase(float p);
    float process(float freqHz, float sampleTime, float morph, float timbre = 0.5f);
    float processTZ(float instFreqHz, float sampleTime, float morph, float timbre = 0.5f);
};
```

| Member | Description |
|--------|-------------|
| `reset()` | Resets all phases to their default spread. |
| `setPhase(p)` | Sets the center phase directly. `p` is wrapped to `[0, 1)`. Satellite phases are unaffected. |
| `process(freqHz, sampleTime, morph, timbre)` | Advances phase by `freqHz * sampleTime` and returns the crossfaded output. Constant-power crossfade and per-shape RMS normalization keep apparent volume consistent as `morph` sweeps. `timbre` in `[0, 1]` shapes the waveform(s); `0.5` is neutral for all shapes. |
| `processTZ(instFreqHz, sampleTime, morph, timbre)` | Like `process()` but uses a floor-based phase wrap that handles negative `instFreqHz`. Feed the result of `fmThroughZero()` here. |

**`morph` parameter:**

| Value | Result |
|-------|--------|
| `0.0` | Pure sine |
| `1.0` | Pure triangle |
| `2.0` | Pure pulse |
| `3.0` | Pure saw |
| `4.0` | Pure super saw |
| `0.0`–`1.0` | Crossfade sine → triangle |
| `1.0`–`2.0` | Crossfade triangle → pulse |
| `2.0`–`3.0` | Crossfade pulse → saw |
| `3.0`–`4.0` | Crossfade saw → super saw |

Values outside `[0, 4]` are clamped. `timbre` in `[0, 1]` applies to whichever shape(s) are active; see the `Oscillator` timbre table above for per-shape behavior (default `0.5`). Gain compensation is applied automatically — see the `Oscillator` super saw note above.

**Example:**
```cpp
ataraxic_dsp::MorphingOscillator osc;

// In process loop (sampleTime = 1.0f / sampleRate):

// Pure triangle (neutral timbre):
float out = osc.process(440.0f, sampleTime, 1.0f);

// 20% pulse + 80% saw (morph = 2.8), brighter timbre:
float out = osc.process(440.0f, sampleTime, 2.8f, 0.75f);

// Super saw with standard spread:
float out = osc.process(440.0f, sampleTime, 4.0f);

// Sweep morph from a CV value in [0, 1] mapped to [0, 4]:
float morphCV = clamp(cv / 10.f, 0.f, 1.f) * 4.0f;
float out = osc.process(440.0f, sampleTime, morphCV);
```

RAM budget on Cortex-M: ~28 bytes (phase + 6 satellite phases). Shares the `detail::sine_lut()` LUT with `Oscillator`.

---

## FM Helpers (`oscillator.hpp`)

Three free functions that compute an instantaneous frequency in Hz for use with `Oscillator` or `MorphingOscillator`. They are stateless and reusable across both oscillator types.

`fmCV` is a caller-normalized value — the library assumes no specific voltage standard. For Eurorack divide the raw voltage by the CV rail range before passing it (e.g. `voltage / 5.0f` for a ±5 V signal).

```cpp
inline float fmLinear(float baseHz, float fmCV, float depth);
inline float fmExp(float baseHz, float fmCV, float depth);
inline float fmThroughZero(float baseHz, float fmCV, float depth);
```

| Function | Formula | Notes |
|----------|---------|-------|
| `fmLinear` | `baseHz + fmCV * depth`, clamped ≥ 0 | `depth` in Hz/unit. Carrier cannot go negative — pass to `process()`. |
| `fmExp` | `baseHz * 2^(fmCV * depth)` | `depth` in octaves/unit. Preserves pitch intervals at any carrier frequency. Pass to `process()`. |
| `fmThroughZero` | `baseHz + fmCV * depth`, signed | `depth` in Hz/unit. Result may be negative (phase runs backwards). Pass to `processTZ()`. |

**Example:**
```cpp
ataraxic_dsp::Oscillator        osc;
ataraxic_dsp::MorphingOscillator morphOsc;

const float sampleTime = 1.0f / 48000.0f;
const float baseHz     = 220.0f;
const float fmCV       = inputs[FM_INPUT].getVoltage() / 5.0f;  // normalize ±5 V → [-1, 1]

// Linear FM: ±50 Hz deviation, clamped at 0 Hz minimum
float out1 = osc.process(ataraxic_dsp::fmLinear(baseHz, fmCV, 50.0f),
                          sampleTime, ataraxic_dsp::Oscillator::SAW);

// Exponential FM: ±1 octave at fmCV = ±1
float out2 = osc.process(ataraxic_dsp::fmExp(baseHz, fmCV, 1.0f),
                          sampleTime, ataraxic_dsp::Oscillator::SINE);

// Through-zero FM: phase reverses when instFreq < 0
float out3 = osc.processTZ(ataraxic_dsp::fmThroughZero(baseHz, fmCV, 200.0f),
                             sampleTime, ataraxic_dsp::Oscillator::SINE);

// MorphingOscillator with through-zero FM (20% pulse + 80% saw blend)
float out4 = morphOsc.processTZ(ataraxic_dsp::fmThroughZero(baseHz, fmCV, 300.0f),
                                  sampleTime, 2.8f);
```

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
