cmake_minimum_required(VERSION 3.22)
project(realtime-inference-workshop LANGUAGES C CXX)  # C for JUCE's few C sources

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd, see .clangd

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

# The models are downloaded from the repository's own release into
# steps/common/assets/models/, unless they are already there: only those the
# enabled steps load. The test signal and the expected output are compiled in
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
