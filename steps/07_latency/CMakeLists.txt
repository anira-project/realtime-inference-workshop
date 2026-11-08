# Latency and the dry/wet mix, on top of step 6's threading. Demonstration
# only: the solution, ready to run.
workshop_setup_readerwriterqueue()
workshop_setup_onnxruntime()
add_compile_definitions(WORKSHOP_HOST_SLOWDOWN=${WORKSHOP_HOST_SLOWDOWN})

add_executable(step07_solution solution/main.cpp)
target_include_directories(step07_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps")
target_compile_definitions(step07_solution PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
target_link_libraries(step07_solution PRIVATE workshop::onnxruntime readerwriterqueue)
workshop_copy_onnxruntime_runtime(step07_solution)
