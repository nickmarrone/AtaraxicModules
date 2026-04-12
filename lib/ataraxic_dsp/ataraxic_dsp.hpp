#pragma once

// Ataraxic DSP Library
// Portable, header-only C++11 DSP utilities for audio synthesis.
// Zero external dependencies — only <cmath> and <stdint.h>.
// Compatible with desktop (x64/ARM64), RP2350, and STM32 targets.
//
// Usage in VCV Rack modules:
//   #include "../../lib/ataraxic_dsp/ataraxic_dsp.hpp"
//
// Usage in embedded CMake projects:
//   add_subdirectory(path/to/AtaraxicModules/lib)
//   target_link_libraries(my_target PRIVATE ataraxic_dsp)
//   #include "ataraxic_dsp/ataraxic_dsp.hpp"

#include "schmitt_trigger.hpp"
#include "envelope.hpp"
#include "pot.hpp"
#include "vca.hpp"
#include "onepole.hpp"
#include "noise.hpp"
#include "oscillator.hpp"
