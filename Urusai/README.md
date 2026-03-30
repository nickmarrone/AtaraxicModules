# Ataraxic Urusai

Urusai is a slim 2HP noise generator module for VCV Rack that packs seven distinct flavors of noise into a single component. 

Every noise output has been mathematically equalized to match the perceived level (RMS) of standard white noise. This means you can freely swap between different noise colors in your patches without needing to drastically adjust your mixer or VCA levels.

## Features

Seven independent, simultaneously available noise outputs:

*   **WHT (White Noise):** Standard uniform random noise with equal power across all frequencies.
*   **PNK (Pink Noise):** Filtered for a -3dB/octave slope. Equal power per octave, sounding balanced and natural to the human ear.
*   **BLU (Blue Noise):** Filtered for a +3dB/octave slope (derivative of pink noise), emphasizing high frequencies without being overwhelmingly bright.
*   **VIO (Violet Noise):** Filtered for a +6dB/octave slope (derivative of white noise). Very bright and hissy.
*   **VLV (Velvet Noise):** Sparse, random impulses of audio (-1 or +1) mixed with silence. Creates unique textures perfect for exciting resonators or simulating crackly textures.
*   **CMOS (CMOS Noise):** Classic shift-register (LFSR) style chunky digital noise. Raw, full-amplitude unipolar logic signals scaled for audio use.
*   **8-BIT (8-Bit Noise):** A slower-clocked variant of CMOS noise reminiscent of vintage 8-bit video game sound chips.

## Build Instructions

You can build this plugin for VCV Rack using the standard plugin toolchain:

```sh
export RACK_DIR=<path-to-your-Rack-SDK>
make
make install
```
