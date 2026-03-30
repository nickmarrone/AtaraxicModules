# Ataraxic Modules

A collection of high-quality, utilitarian synthesizer modules for [VCV Rack](https://vcvrack.com/), designed with an emphasis on compactness, visual elegance, and playability.

## Included Modules

### [ADVCA](./ADVCA)

**ADVCA** is a 4HP utility block that packs an AD/AR Envelope Generator and a Voltage Controlled Amplifier (VCA) into a single, compact space.

*   Selectable Attack-Decay (AD) and Attack-Release (AR) envelope modes.
*   Logarithmic ("log pot") feel on the envelope time knobs.
*   Envelope Output is internally normalled to the VCA's CV Input, allowing the module to act as a self-contained voice articulation tool out-of-the-box.
*   VCA Response trimpot crossfades continuously between Linear and Exponential (punchy $x^4$ mapping).

[See the ADVCA README](./ADVCA/README.md) for more details.

---

### [Urusai](./Urusai)

**Urusai** is a slim 2HP multi-flavor noise generator that provides seven distinct, simultaneously available noise outputs.

1.  **WHT:** Standard uniform White Noise.
2.  **PNK:** -3dB/oct Pink Noise.
3.  **BLU:** +3dB/oct Blue Noise.
4.  **VIO:** +6dB/oct Violet Noise.
5.  **VLV:** Sparse, impulse-based Velvet Noise.
6.  **CMOS:** High-amplitude LFSR digital shift-register Noise.
7.  **8-BIT:** A slower, crunchier variant of the CMOS noise.

All seven outputs have been mathematically compensated to share the exact same perceived volume level, making it frictionless to swap noise profiles in your patches.

[See the Urusai README](./Urusai/README.md) for more details.

## Build Instructions

Each plugin is built entirely independently. You can find their respective source codes in their corresponding subdirectories.

To build an individual module, simply `cd` into its directory and use the standard VCV Rack plugin toolchain commands:

```bash
export RACK_DIR=<path-to-your-Rack-SDK>

# To build ADVCA
cd ADVCA
make
make install

# To build Urusai
cd ../Urusai
make
make install
```

## License

Ataraxic Modules are licensed under the [CC0 1.0 Universal License](./LICENSE).
