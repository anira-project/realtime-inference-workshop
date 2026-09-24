cmake_minimum_required(VERSION 3.22)
project(realtime-inference-workshop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd, see .clangd

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(backends)

# Fetch the engines once, so every step links the same build.
workshop_setup_libtorch()

# The model file. It is not in the repo; see models/README.md. The test signal
# and the expected output are compiled in (steps/*/test_signal.h).
set(WORKSHOP_MODEL "${CMAKE_CURRENT_LIST_DIR}/models/forward_stateful.pt"
    CACHE FILEPATH "TorchScript model to run")

# Build one step (-DWORKSHOP_STEP=01) or all of them.
set(WORKSHOP_STEP "all" CACHE STRING "Which step to build: a number like 01, or all")

set(WORKSHOP_STEPS 01_minimal_inference 02_host_buffer_size)

foreach(step ${WORKSHOP_STEPS})
    if(WORKSHOP_STEP STREQUAL "all" OR step MATCHES "^${WORKSHOP_STEP}")
        add_subdirectory(steps/${step})
    endif()
endforeach()
