# Demonstration only: one target, ONNX Runtime, optionally under RTSan.
workshop_setup_onnxruntime()

add_executable(step05_solution solution/main.cpp)
target_include_directories(step05_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps")
target_compile_definitions(step05_solution PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
target_link_libraries(step05_solution PRIVATE workshop::onnxruntime)
workshop_copy_onnxruntime_runtime(step05_solution)

# RealtimeSanitizer needs clang >= 20 from LLVM with its runtime, on macOS
# (Homebrew LLVM, not Apple Clang) or Linux: compiler-rt builds it for no other
# system, so there is none on Windows. Without it the program still runs, it
# just checks nothing.
if(WORKSHOP_RTSAN)
    if(WIN32)
        message(FATAL_ERROR "WORKSHOP_RTSAN: RealtimeSanitizer exists on macOS and Linux only, "
                            "not on Windows. Configure without it; see steps/05_realtime_safety/README.md")
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20)
        message(FATAL_ERROR "WORKSHOP_RTSAN needs clang >= 20 from LLVM (on macOS Homebrew LLVM, "
                            "not Apple Clang), not ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. "
                            "See steps/05_realtime_safety/README.md")
    endif()
    target_compile_options(step05_solution PRIVATE -fsanitize=realtime)
    target_link_options(step05_solution PRIVATE -fsanitize=realtime)
endif()
