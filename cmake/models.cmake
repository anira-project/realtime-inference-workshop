# The model files, downloaded from the repository's own release.
#
# They are not in git: 400 MB of weights would make every clone slow, and Git
# LFS has a bandwidth quota that a roomful of people runs through in an hour.
# Release assets have neither problem. They come as three archives, one per set
# of steps, so a configure makes one to three requests and loads only what the
# enabled steps need; download.cmake retries them and keeps them in downloads/.
#
#   workshop_fetch_models()   -> models/ holds what the enabled steps load,
#                                or configuring fails
#
# Bring your own instead with -DWORKSHOP_MODEL=/path/to/model.pt
# (and -DWORKSHOP_ONNX_MODEL=...), which skips the download.

include_guard(GLOBAL)
include(download)

# One release for the model. A new export replaces its archives there and the
# hashes below; the hash check then fetches the new files into every checkout.
set(WORKSHOP_MODELS_TAG "workshop-model")
set(WORKSHOP_MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../models" CACHE PATH "Where the models live")
cmake_path(SET WORKSHOP_MODELS_DIR NORMALIZE "${WORKSHOP_MODELS_DIR}")

# name and sha256 of every file, as "name|sha256" — a semicolon would be a list
# separator and fall apart in the loops below. One list per archive, each
# archive with its own sha256.
# Steps 1 to 9: the forward model, as TorchScript and as ONNX
set(_workshop_models_forward_archive
    "workshop-models-forward.zip|94912bab17ce4ea06483e3622b81336f3cdfa54dec3fef09f5e6e3e64bedb1d4")
set(_workshop_models_forward
    "model.pt|f9d732030a859b56ed5a6417aa18bde7a9d06f17b43379bec2492ef19a21f540"
    "model_forward.onnx|a75d50597aa8d4287ba5de73fd933d2df1c0f07c99539f5c88b1d7f694ad6f26")
# Steps 10 and 11 (WORKSHOP_ANIRA): the model taken apart
set(_workshop_models_split_archive
    "workshop-models-split.zip|35f4e06dbe14bca4b1069c529effca0a9660405d64b1cffb1b8245de88217ec6")
set(_workshop_models_split
    "model_encoder.onnx|7c202eede08c638b75a2cd67b571ad3d15a6d8bbd1928a1f71f5a96f564d6d9a"
    "model_decoder.onnx|66ce39f2b5fc9122852c088b16cde5872a5541dceb59e601262d25f78bf3a4a8")
# Step 11 (WORKSHOP_ANIRA and WORKSHOP_JUCE): MediaPipe's hand tracking chain as
# ONNX (Apache 2.0), via anira-cam
set(_workshop_models_hands_archive
    "workshop-models-hands.zip|b5b945be0ade48d3cd9c88b7f41bdec0c019a2ab20dae2ef399debe685349f8f")
set(_workshop_models_hands
    "hand_detector.onnx|431501d81309306a0526d333df5ff23eccb4e01987bc20d19c56e47a001f2873"
    "hand_landmarks_detector.onnx|b4ac6e8ad824556264bd2e76141881904beeab80b63c9de56449dedbadf23471"
    "gesture_embedder.onnx|b58c43055433d21c4075a0a5bdb9d641f418960dbbbf0a8673558a8b2ed6991e"
    "canned_gesture_classifier.onnx|0878082b485c565b9d95ccd805bf2d83e9a7cebe198d7899f26094d626433cd4")
set(_workshop_model_files ${_workshop_models_forward} ${_workshop_models_split} ${_workshop_models_hands})

# One set: nothing to do when every file of it is in models/ and intact, else its
# archive is downloaded (or taken from downloads/) and unpacked into models/.
function(_workshop_fetch_model_set set)
    set(missing FALSE)
    foreach(entry IN LISTS _workshop_models_${set})
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 expected_sha)
        set(file "${WORKSHOP_MODELS_DIR}/${name}")
        if(NOT EXISTS "${file}")
            set(missing TRUE)
            break()
        endif()
        file(SHA256 "${file}" actual_sha)
        if(NOT actual_sha STREQUAL expected_sha)
            message(STATUS "Workshop: ${name} does not match the release")
            set(missing TRUE)
            break()
        endif()
    endforeach()
    if(NOT missing)
        return()
    endif()

    string(REPLACE "|" ";" parts "${_workshop_models_${set}_archive}")
    list(GET parts 0 archive_name)
    list(GET parts 1 archive_sha)
    workshop_download(
        "https://github.com/anira-project/realtime-inference-workshop/releases/download/${WORKSHOP_MODELS_TAG}/${archive_name}"
        "${archive_sha}" archive)
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${WORKSHOP_MODELS_DIR}")

    foreach(entry IN LISTS _workshop_models_${set})
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 expected_sha)
        file(SHA256 "${WORKSHOP_MODELS_DIR}/${name}" actual_sha)
        if(NOT actual_sha STREQUAL expected_sha)
            message(FATAL_ERROR "Workshop: ${name} from ${archive_name} does not match its SHA256")
        endif()
    endforeach()
endfunction()

# Reads WORKSHOP_ANIRA and WORKSHOP_JUCE: call it after their option().
function(workshop_fetch_models)
    _workshop_fetch_model_set(forward)
    if(WORKSHOP_ANIRA)
        _workshop_fetch_model_set(split)
        if(WORKSHOP_JUCE)
            _workshop_fetch_model_set(hands)
        endif()
    endif()
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
