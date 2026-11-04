# Demonstration only: Google Benchmark, and both engines side by side.
workshop_setup_onnxruntime()

add_executable(step04_solution solution/main.cpp)
target_include_directories(step04_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step04_solution PRIVATE
    WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}"
    WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
target_link_libraries(step04_solution PRIVATE "${TORCH_LIBRARIES}" workshop::onnxruntime benchmark::benchmark)
workshop_copy_libtorch_runtime(step04_solution)
workshop_copy_onnxruntime_runtime(step04_solution)
