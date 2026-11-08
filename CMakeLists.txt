cmake_minimum_required(VERSION 3.22)
project(realtime-inference-workshop LANGUAGES C CXX)  # C for JUCE's few C sources

# FetchContent without its sub-build (CMake 3.30 and up), also in anira: the
# sub-build's paths are too long for Windows in a checkout of ordinary depth.
if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd, see .clangd

# MSBuild builds projects side by side, but the files of one project one after
# another unless cl gets /MP. JUCE sets it on its own targets; this gives it to
# anira's and ours. Set here, before the dependencies are added, and not as
# CMAKE_CXX_FLAGS in a preset, which would replace MSVC's default flags.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_GENERATOR MATCHES "^Visual Studio")
    add_compile_options(/MP)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(compile-commands)
workshop_link_compile_commands()  # build/compile_commands.json -> this tree's
include(backends)
include(readerwriterqueue)
include(juce)
include(models)
include(anira)
include(exercises)

set(WORKSHOP_ONNX_MODEL "${WORKSHOP_MODELS_DIR}/rave/model_forward.onnx"
    CACHE FILEPATH "ONNX model to run")
# A build directory configured before the models moved still points at models/
string(FIND "${WORKSHOP_ONNX_MODEL}" "${CMAKE_CURRENT_LIST_DIR}/models/" _workshop_old_model)
if(_workshop_old_model EQUAL 0)
    set(WORKSHOP_ONNX_MODEL "${WORKSHOP_MODELS_DIR}/rave/model_forward.onnx"
        CACHE FILEPATH "ONNX model to run" FORCE)
endif()

# RealtimeSanitizer for step 5; needs a clang that has it, see that step's README.
option(WORKSHOP_RTSAN "Build the real-time safety step with -fsanitize=realtime" OFF)

# JUCE, for the plugins of steps 8 and 9 and the app of step 11. The workshop
# relies on it, so it is on; JUCE only compiles when one of those is built.
option(WORKSHOP_JUCE "Build the JUCE plugin and app steps" ON)

# Part 2 builds anira from source, which takes a couple of minutes once.
option(WORKSHOP_ANIRA "Build the anira steps (9 and up)" ON)

# How many times slower than real time the host loop of steps 6 and 7 calls
# back. 1 everywhere but CI: a shared runner does not keep up with real time,
# so CI sets 4, and the checks still test every alignment and latency claim.
set(WORKSHOP_HOST_SLOWDOWN 1 CACHE STRING "Pace the host loop of steps 6 and 7 this many times slower (CI)")

# The models are downloaded from Hugging Face, or the repository's own release,
# into steps/common/assets/models/, unless they are already there. The test
# signal and the expected output are compiled in
# (steps/common/assets/test_signal.h).
workshop_fetch_models()

# Build one step (-DWORKSHOP_STEP=01) or all of them.
set(WORKSHOP_STEP "all" CACHE STRING "Which step to build: a number like 01 or 02, or all")

set(WORKSHOP_STEPS 01_minimal_inference 02_state 03_host_buffer_size 04_benchmark 05_realtime_safety 06_threading 07_latency)

if(WORKSHOP_JUCE)
    list(APPEND WORKSHOP_STEPS 08_juce_plugin)
endif()
if(WORKSHOP_ANIRA)
    list(APPEND WORKSHOP_STEPS 09_anira 10_split)
    if(WORKSHOP_JUCE)
        list(APPEND WORKSHOP_STEPS 11_hands)
    endif()
endif()

foreach(step ${WORKSHOP_STEPS})
    if(WORKSHOP_STEP STREQUAL "all" OR step MATCHES "^${WORKSHOP_STEP}")
        add_subdirectory(steps/${step})
    endif()
endforeach()
