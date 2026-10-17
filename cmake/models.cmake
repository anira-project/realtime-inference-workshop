# The model files, downloaded from the repository's own release.
#
# They are not in git: 380 MB of weights would make every clone slow, and Git
# LFS has a bandwidth quota that a roomful of people runs through in an hour.
# Release assets have neither problem.
#
#   workshop_fetch_models()   -> models/ is complete, or configuring fails
#
# Bring your own instead with -DWORKSHOP_MODEL=/path/to/model.pt
# (and -DWORKSHOP_ONNX_MODEL=...), which skips the download.

include_guard(GLOBAL)

# One release for the model. A new export replaces its files there and the
# hashes below; the hash check then fetches the new files into every checkout.
set(WORKSHOP_MODELS_TAG "workshop-model")
set(WORKSHOP_MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../models" CACHE PATH "Where the models live")
cmake_path(SET WORKSHOP_MODELS_DIR NORMALIZE "${WORKSHOP_MODELS_DIR}")

# name and sha256 of every file in the release, as "name|sha256" — a semicolon
# would be a list separator and fall apart in the loop below.
set(_workshop_model_files
    "model.pt|f9d732030a859b56ed5a6417aa18bde7a9d06f17b43379bec2492ef19a21f540"
    "model_forward.onnx|a75d50597aa8d4287ba5de73fd933d2df1c0f07c99539f5c88b1d7f694ad6f26"
    "model_encoder.onnx|7c202eede08c638b75a2cd67b571ad3d15a6d8bbd1928a1f71f5a96f564d6d9a"
    "model_decoder.onnx|66ce39f2b5fc9122852c088b16cde5872a5541dceb59e601262d25f78bf3a4a8")

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

# A build directory configured against an older release still has the old file
# names cached, and would keep loading them. Point such a path at the current
# file; a path outside models/ is someone's own copy and stays.
#   workshop_use_current_model(WORKSHOP_MODEL model.pt)
function(workshop_use_current_model variable current_name)
    set(names "")
    foreach(entry IN LISTS _workshop_model_files)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(APPEND names "${name}")
    endforeach()

    cmake_path(GET ${variable} PARENT_PATH directory)
    cmake_path(GET ${variable} FILENAME name)
    cmake_path(COMPARE "${directory}" EQUAL "${WORKSHOP_MODELS_DIR}" in_models)
    if(in_models AND NOT name IN_LIST names)
        message(STATUS "Workshop: ${name} is not part of the release anymore, using ${current_name}")
        get_property(help CACHE ${variable} PROPERTY HELPSTRING)
        set(${variable} "${WORKSHOP_MODELS_DIR}/${current_name}" CACHE FILEPATH "${help}" FORCE)
    endif()
endfunction()
