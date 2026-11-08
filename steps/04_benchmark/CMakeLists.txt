# Demonstration only: the usual step build, timed with std::chrono.
workshop_setup_onnxruntime()

add_executable(step04_solution solution/main.cpp)
target_include_directories(step04_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps")
target_compile_definitions(step04_solution PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}"
                                                   WORKSHOP_HOST_SLOWDOWN=${WORKSHOP_HOST_SLOWDOWN})
target_link_libraries(step04_solution PRIVATE workshop::onnxruntime)
workshop_copy_onnxruntime_runtime(step04_solution)
