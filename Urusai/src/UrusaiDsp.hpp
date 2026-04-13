#pragma once
#include "../../lib/ataraxic_dsp/ataraxic_dsp.hpp"

// ---------------------------------------------------------------------------
// UrusaiDsp — Full 7-output noise engine for the Urusai module.
// Combines all ataraxic_dsp noise generators with per-sample caching (white
// and pink are computed once per tick and reused by derived outputs) and
// one-pole filter state for the tone/character control.
//
// Usage per sample:
//   dsp.beginSample();
//   float w = dsp.getWhite();
//   float p = dsp.getPink();
//   ...
// ---------------------------------------------------------------------------
struct UrusaiDsp {
    ataraxic_dsp::WhiteNoise    whiteGen;
    ataraxic_dsp::PinkFilter    pinkFilter;
    ataraxic_dsp::BlueFilter    blueFilter;
    ataraxic_dsp::VioletFilter  violetFilter;
    ataraxic_dsp::VelvetNoise   velvetGen;
    ataraxic_dsp::CmosNoise     cmos;
    ataraxic_dsp::EightBitNoise eightBit;

    // One-pole filter state per toned output (two integrators each: LP and HP-tracking)
    float whiteLp,  whiteHpLp;
    float pinkLp,   pinkHpLp;
    float blueLp,   blueHpLp;
    float violetLp, violetHpLp;

    // Per-sample cache
    float _white, _pink;
    bool  _whiteReady, _pinkReady;

    UrusaiDsp() :
        whiteLp(0), whiteHpLp(0),
        pinkLp(0),  pinkHpLp(0),
        blueLp(0),  blueHpLp(0),
        violetLp(0), violetHpLp(0),
        _white(0), _pink(0),
        _whiteReady(false), _pinkReady(false)
    {}

    void init(ataraxic_dsp::RngFn rngFn, void* ctx = 0) {
        whiteGen.init(rngFn, ctx);
        velvetGen.init(rngFn, ctx);
        pinkFilter.reset();
        blueFilter.reset();
        violetFilter.reset();
        cmos.reset();
        eightBit.reset();
        whiteLp = whiteHpLp = pinkLp = pinkHpLp = 0.0f;
        blueLp  = blueHpLp  = violetLp = violetHpLp = 0.0f;
        _white = _pink = 0.0f;
        _whiteReady = _pinkReady = false;
    }

    // Call once at the start of each sample tick to reset the per-sample cache.
    void beginSample() {
        _whiteReady = false;
        _pinkReady  = false;
    }

    // White noise: generated once per sample, cached for reuse by blue/violet/velvet.
    float getWhite() {
        if (!_whiteReady) {
            _white = whiteGen.next();
            _whiteReady = true;
        }
        return _white;
    }

    // Pink noise: generated once per sample from the cached white sample.
    float getPink() {
        if (!_pinkReady) {
            _pink = pinkFilter.process(getWhite());
            _pinkReady = true;
        }
        return _pink;
    }

    // Blue noise: first-order difference of pink, gain 10x (+3 dB/oct slope).
    float getBlue() {
        return blueFilter.process(getPink());
    }

    // Violet noise: first-order difference of white, gain 0.707x (+6 dB/oct slope).
    float getViolet() {
        return violetFilter.process(getWhite());
    }

    // Velvet noise: sparse random impulses.
    // character in [0,1] sweeps impulse rate from 10 Hz to 10 kHz.
    // Uses a separate RNG draw for the probability check; uses the cached white
    // sample for the sign.
    float getVelvet(float character, float sampleTime) {
        return velvetGen.process(character, sampleTime, getWhite());
    }

    // CMOS (MM5837-style) noise.
    float getCmos(float character, float sampleTime) {
        return cmos.process(character, sampleTime);
    }

    // 8-bit (NES-style) noise.
    float getEightBit(float character, float sampleTime) {
        return eightBit.process(character, sampleTime);
    }
};
