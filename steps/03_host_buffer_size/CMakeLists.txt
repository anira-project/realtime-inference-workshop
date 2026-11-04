# Same build as step 1, plus the ring buffer from common/.
workshop_setup_onnxruntime()

foreach(variant exercise solution)
    set(target step03_${variant})
    add_executable(${target} ${variant}/main.cpp)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/steps")
    target_compile_definitions(${target} PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
    target_link_libraries(${target} PRIVATE workshop::onnxruntime)
    workshop_copy_onnxruntime_runtime(${target})
endforeach()
