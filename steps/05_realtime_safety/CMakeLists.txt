# Demonstration only: one target, both engines, optionally under RTSan.
workshop_setup_onnxruntime()

add_executable(step05_solution solution/main.cpp)
target_include_directories(step05_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step05_solution PRIVATE
    WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}"
    WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
target_link_libraries(step05_solution PRIVATE "${TORCH_LIBRARIES}" workshop::onnxruntime)
workshop_copy_libtorch_runtime(step05_solution)
workshop_copy_onnxruntime_runtime(step05_solution)

# RealtimeSanitizer needs a clang that has it (Homebrew LLVM ≥ 20 on macOS, not
# Apple Clang). Without it the program still runs, it just checks nothing.
if(WORKSHOP_RTSAN)
    target_compile_options(step05_solution PRIVATE -fsanitize=realtime)
    target_link_options(step05_solution PRIVATE -fsanitize=realtime)
endif()
