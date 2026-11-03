# Demonstration only: Google Benchmark on top of the usual step build.
add_executable(step03_solution solution/main.cpp)
target_include_directories(step03_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step03_solution PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step03_solution PRIVATE "${TORCH_LIBRARIES}" benchmark::benchmark)
workshop_copy_libtorch_runtime(step03_solution)
