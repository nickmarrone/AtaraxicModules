cmake_minimum_required(VERSION 3.13)

# ataraxic_dsp: header-only portable C++ DSP library.
# Use as an INTERFACE library — no sources to compile, only include paths.
#
# In your CMakeLists.txt:
#   add_subdirectory(path/to/AtaraxicModules/lib)
#   target_link_libraries(my_target PRIVATE ataraxic_dsp)
#
# Then in your code:
#   #include "ataraxic_dsp/ataraxic_dsp.hpp"
#
# On embedded targets, link with -lm for std::pow() (newlib math library).

add_library(ataraxic_dsp INTERFACE)
target_include_directories(ataraxic_dsp INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_compile_features(ataraxic_dsp INTERFACE cxx_std_11)
