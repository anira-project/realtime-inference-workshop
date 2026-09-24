# The exercise and the solution build the same way: one file, LibTorch, and the
# path to the models baked in so the binaries run without arguments.

# Exercise
add_executable(step01_exercise exercise/main.cpp)
target_include_directories(step01_exercise PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step01_exercise PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step01_exercise PRIVATE "${TORCH_LIBRARIES}")
workshop_copy_libtorch_runtime(step01_exercise)

# Solution
add_executable(step01_solution solution/main.cpp)
target_include_directories(step01_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step01_solution PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step01_solution PRIVATE "${TORCH_LIBRARIES}")
workshop_copy_libtorch_runtime(step01_solution)
