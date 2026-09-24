# Same build as step 1, plus the ring buffer and the fake host from common/.

# Exercise
add_executable(step02_exercise exercise/main.cpp)
target_include_directories(step02_exercise PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step02_exercise PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step02_exercise PRIVATE "${TORCH_LIBRARIES}")
workshop_copy_libtorch_runtime(step02_exercise)

# Solution
add_executable(step02_solution solution/main.cpp)
target_include_directories(step02_solution PRIVATE "${CMAKE_SOURCE_DIR}/steps" "${CMAKE_CURRENT_LIST_DIR}" ${TORCH_INCLUDE_DIRS})
target_compile_definitions(step02_solution PRIVATE WORKSHOP_MODEL_PATH="${WORKSHOP_MODEL}")
target_link_libraries(step02_solution PRIVATE "${TORCH_LIBRARIES}")
workshop_copy_libtorch_runtime(step02_solution)
