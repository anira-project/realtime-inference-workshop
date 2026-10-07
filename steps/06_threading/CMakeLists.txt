# Two threads and a lock-free queue between them.
workshop_setup_readerwriterqueue()

foreach(variant exercise solution)
    set(target step06_${variant})
    add_executable(${target} ${variant}/main.cpp)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
    target_link_libraries(${target} PRIVATE "${TORCH_LIBRARIES}" readerwriterqueue)
    workshop_copy_libtorch_runtime(${target})

    if(WORKSHOP_RTSAN)
        target_compile_options(${target} PRIVATE -fsanitize=realtime)
        target_link_options(${target} PRIVATE -fsanitize=realtime)
    endif()
endforeach()
