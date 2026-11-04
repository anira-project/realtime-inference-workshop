# Demonstration only: Google Benchmark on top of the usual step build.
add_executable(step04_solution solution/main.cpp)
target_include_directories(step04_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step04_solution PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step04_solution PRIVATE "${TORCH_LIBRARIES}" benchmark::benchmark)
workshop_copy_libtorch_runtime(step04_solution)
