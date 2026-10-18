cmake_minimum_required(VERSION 3.22)
project(realtime-inference-workshop LANGUAGES C CXX)  # C for JUCE's few C sources

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd, see .clangd

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(backends)
include(benchmark)
include(readerwriterqueue)
include(juce)
include(models)
include(anira)

# Fetch the engines once, so every step links the same build.
workshop_setup_libtorch()
workshop_setup_benchmark()

# The models are downloaded from the repository's own release, unless they are
# already in models/. The test signal and the expected output are compiled in
# (steps/common/assets/test_signal.h).
workshop_fetch_models()

set(WORKSHOP_MODEL "${CMAKE_CURRENT_LIST_DIR}/models/model.pt"
    CACHE FILEPATH "TorchScript model to run")
set(WORKSHOP_ONNX_MODEL "${CMAKE_CURRENT_LIST_DIR}/models/model_forward.onnx"
    CACHE FILEPATH "ONNX model to run")
workshop_use_current_model(WORKSHOP_MODEL model.pt)
workshop_use_current_model(WORKSHOP_ONNX_MODEL model_forward.onnx)

# RealtimeSanitizer for step 5; needs a clang that has it, see that step's README.
option(WORKSHOP_RTSAN "Build the real-time safety step with -fsanitize=realtime" OFF)

# The plugin step clones and builds JUCE, which takes minutes — off by default.
option(WORKSHOP_JUCE "Build the JUCE plugin step" OFF)

# Part 2 builds anira from source, which takes a couple of minutes once.
option(WORKSHOP_ANIRA "Build the anira steps (9 and up)" ON)

# Build one step (-DWORKSHOP_STEP=01) or all of them.
set(WORKSHOP_STEP "all" CACHE STRING "Which step to build: a number like 01, or all")

set(WORKSHOP_STEPS 01_minimal_inference 02_host_buffer_size 03_benchmark 05_realtime_safety 06_threading 07_latency)

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
