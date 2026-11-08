# Two threads and a lock-free queue between them.
workshop_setup_readerwriterqueue()
workshop_setup_onnxruntime()
add_compile_definitions(WORKSHOP_HOST_SLOWDOWN=${WORKSHOP_HOST_SLOWDOWN})

workshop_add_exercise_and_solution(step06 workshop::onnxruntime readerwriterqueue)
