# Two threads and a lock-free queue between them.
workshop_setup_readerwriterqueue()

add_executable(step06_solution solution/main.cpp)
target_include_directories(step06_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step06_solution PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step06_solution PRIVATE "${TORCH_LIBRARIES}" readerwriterqueue)
workshop_copy_libtorch_runtime(step06_solution)

if(WORKSHOP_RTSAN)
    target_compile_options(step06_solution PRIVATE -fsanitize=realtime)
    target_link_options(step06_solution PRIVATE -fsanitize=realtime)
endif()
