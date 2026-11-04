# The exercise and the solution build the same way: one file, ONNX Runtime, and
# the path to the model baked in so the binaries run without arguments.
workshop_setup_onnxruntime()

foreach(variant exercise solution)
    set(target step01b_${variant})
    add_executable(${target} ${variant}/main.cpp)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/steps")
    target_compile_definitions(${target} PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
    target_link_libraries(${target} PRIVATE workshop::onnxruntime)
    workshop_copy_onnxruntime_runtime(${target})
endforeach()
