# The model files, downloaded from the repository's own release.
#
# They are not in git: 124 MB of weights would make every clone slow, and Git
# LFS has a bandwidth quota that a roomful of people runs through in an hour.
# Release assets have neither problem.
#
#   workshop_fetch_models()   -> models/ is complete, or configuring fails
#
# Bring your own instead with -DWORKSHOP_MODEL=/path/to/forward_stateful.pt
# (and -DWORKSHOP_ONNX_MODEL=...), which skips the download.

include_guard(GLOBAL)

set(WORKSHOP_MODELS_TAG "models-v1" CACHE STRING "Release tag the model files come from")
set(WORKSHOP_MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../models" CACHE PATH "Where the models live")
cmake_path(SET WORKSHOP_MODELS_DIR NORMALIZE "${WORKSHOP_MODELS_DIR}")

# name and sha256 of every file in the release, as "name|sha256" — a semicolon
# would be a list separator and fall apart in the loop below.
set(_workshop_model_files
    "forward_stateful.pt|a53723d1519fbf0a574da32e229b437db39b9a6ec93ae470e11b4d0114684183"
    "forward.onnx|32e18cc1582d4a9064b69fed120cafcd09fa9d9f7c0936eab6b2b57883d185ae"
    "forward.onnx.data|36e322fef6fbd5c3b4ad7aaaa3017625b85ad5fc96eaa2d221daf0cf263ce75c")

function(workshop_fetch_models)
    set(base
        "https://github.com/anira-project/realtime-inference-workshop/releases/download/${WORKSHOP_MODELS_TAG}")

    foreach(entry IN LISTS _workshop_model_files)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 expected_sha)
        set(destination "${WORKSHOP_MODELS_DIR}/${name}")

        # Already there and intact: nothing to do. This is the common case, and
        # it keeps a second configure run offline-friendly.
        if(EXISTS "${destination}")
            file(SHA256 "${destination}" actual_sha)
            if(actual_sha STREQUAL expected_sha)
                continue()
            endif()
            message(STATUS "Workshop: ${name} does not match the release, downloading again")
        endif()

        message(STATUS "Workshop: downloading ${name}")
        file(DOWNLOAD "${base}/${name}" "${destination}"
            EXPECTED_HASH "SHA256=${expected_sha}"
            SHOW_PROGRESS
            STATUS status)

        list(GET status 0 code)
        if(NOT code EQUAL 0)
            list(GET status 1 reason)
            file(REMOVE "${destination}")
            message(FATAL_ERROR
                "Workshop: could not download ${name} (${reason}).\n"
                "Get it from ${base}/${name} and put it in ${WORKSHOP_MODELS_DIR}, "
                "or point the build at your own copy with -DWORKSHOP_MODEL=...")
        endif()
    endforeach()
endfunction()
