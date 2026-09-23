# The exercise and the solution build the same way: one file, LibTorch, and the
# path to the models baked in so the binaries run without arguments.

# Exercise
add_executable(step04_exercise exercise/main.cpp)
target_include_directories(step04_exercise PRIVATE "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step04_exercise PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step04_exercise PRIVATE "${TORCH_LIBRARIES}")
workshop_copy_libtorch_runtime(step04_exercise)

# Solution
add_executable(step04_solution solution/main.cpp)
target_include_directories(step04_solution PRIVATE "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step04_solution PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step04_solution PRIVATE "${TORCH_LIBRARIES}")
workshop_copy_libtorch_runtime(step04_solution)
