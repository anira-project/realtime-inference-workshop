# Exercises build once every ? in them is filled in, so they stay out of the
# default build: only on request, with --target. Their filled-in copy, every ?
# taken from the solution (scripts/fill_exercise.py), is how the workshop
# checks them: it has to build and pass like the solution, and it fails if an
# exercise was committed filled in. These targets only exist with
# -DWORKSHOP_CHECK_EXERCISES=ON, as in CI; filling needs Python 3.
#
#   workshop_fill_exercise(<out_var> <file>...)
#     the step's exercise/<file>, filled in from solution/<file>, generated
#     into <build>/exercise_filled/<file>; the paths in <out_var>
#
#   workshop_exclude_from_all(<target>...)
#     the targets, and the format targets of a JUCE plugin, out of the
#     default build
#
#   workshop_add_exercise_and_solution(<step> <libraries...>)
#     a one-file step (exercise/main.cpp, solution/main.cpp):
#     <step>_solution, <step>_exercise and <step>_exercise_filled

option(WORKSHOP_CHECK_EXERCISES "Add the filled-in exercise targets that CI checks" OFF)
if(WORKSHOP_CHECK_EXERCISES)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif()

function(workshop_fill_exercise out_var)
    set(outputs)
    foreach(file ${ARGN})
        set(output "${CMAKE_CURRENT_BINARY_DIR}/exercise_filled/${file}")
        add_custom_command(
            OUTPUT "${output}"
            COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_SOURCE_DIR}/scripts/fill_exercise.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/exercise/${file}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/solution/${file}"
                    "${output}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exercise/${file}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/solution/${file}"
                    "${CMAKE_SOURCE_DIR}/scripts/fill_exercise.py"
            COMMENT "Filling exercise/${file} from the solution")
        list(APPEND outputs "${output}")
    endforeach()
    set(${out_var} ${outputs} PARENT_SCOPE)
endfunction()

function(workshop_exclude_from_all)
    foreach(target ${ARGN})
        foreach(suffix "" _All _Standalone _VST3 _AU _vst3_helper)
            if(TARGET ${target}${suffix})
                set_target_properties(${target}${suffix} PROPERTIES EXCLUDE_FROM_ALL ON)
            endif()
        endforeach()
    endforeach()
endfunction()

function(workshop_add_exercise_and_solution step)
    set(variants exercise solution)
    if(WORKSHOP_CHECK_EXERCISES)
        list(APPEND variants exercise_filled)
    endif()

    foreach(variant ${variants})
        set(target ${step}_${variant})
        if(variant STREQUAL "exercise_filled")
            workshop_fill_exercise(source main.cpp)
        else()
            set(source "${CMAKE_CURRENT_SOURCE_DIR}/${variant}/main.cpp")
        endif()

        add_executable(${target} "${source}")
        if(NOT variant STREQUAL "solution")
            workshop_exclude_from_all(${target})
        endif()
        target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/steps")
        target_compile_definitions(${target} PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
        target_link_libraries(${target} PRIVATE ${ARGN})
        workshop_copy_onnxruntime_runtime(${target})
    endforeach()
endfunction()
