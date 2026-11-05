# Latency and the dry/wet mix, on top of step 6's threading.
workshop_setup_readerwriterqueue()
workshop_setup_onnxruntime()

foreach(variant exercise solution)
    set(target step07_${variant})
    add_executable(${target} ${variant}/main.cpp)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/steps")
    target_compile_definitions(${target} PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
    target_link_libraries(${target} PRIVATE workshop::onnxruntime readerwriterqueue)
    workshop_copy_onnxruntime_runtime(${target})
endforeach()
