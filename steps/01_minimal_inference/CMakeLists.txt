# The exercise and the solution build the same way: one file, ONNX Runtime, and
# the path to the model baked in so the binaries run without arguments.
workshop_setup_onnxruntime()

workshop_add_exercise_and_solution(step01 workshop::onnxruntime)
