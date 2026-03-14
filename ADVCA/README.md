# Ataraxic ADVCA

ADVCA is a 4HP utility module for VCV Rack that combines an AD/AR Envelope Generator with a Voltage Controlled Amplifier (VCA) into a single, compact space.

## Features

### Envelope Generator
*   **Mode Switch:** Select between Attack-Decay (AD) and Attack-Release (AR) envelope modes.
*   **Attack & Decay Knobs:** Dedicated controls for Attack time (1ms to 2s) and Decay/Release time (1ms to 10s). The knobs use a cubic curve for a natural, "log pot" feel.
*   **Gate Input:** Triggers the envelope.
*   **Envelope Output:** Outputs the generated envelope signal (0 to 10V).

### Voltage Controlled Amplifier (VCA)
*   **Internal Normaling:** The Envelope Output is internally normalled to the VCA's CV Input. This allows you to use the module as a self-contained voice articulation tool without needing to patch the envelope into the VCA.
*   **CV Input & Attenuator:** External CV control for the VCA level, with a dedicated attenuator knob.
*   **Response Control:** A trimpot allows you to crossfade the VCA response curve continuously between Linear and Exponential (punchy x^4 mapping).
*   **Audio I/O:** Standard Input and Output for the signal to be processed.
*   **Feedback LED:** Provides visual feedback of the final VCA gain.

## Build Instructions

You can build this plugin for VCV Rack using the standard plugin toolchain:

```sh
export RACK_DIR=<path-to-your-Rack-SDK>
make
make install
```
