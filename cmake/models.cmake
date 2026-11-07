# The model files, downloaded from the repository's own release into
# steps/common/assets/models/, next to the audio in steps/common/assets/audio/:
#
#   rave/           model_forward.onnx, model_encoder.onnx, model_decoder.onnx
#   handtracking/   MediaPipe's four hand tracking models
#
# They are not in git: 250 MB of weights would make every clone slow, and Git
# LFS has a bandwidth quota that a roomful of people runs through in an hour.
# Release assets have neither problem. One archive holds every file the steps
# load, and nothing else. Files that are already there and intact are left
# alone, so a second configure needs no network.
#
#   workshop_fetch_models()   -> the assets/models folders hold every model,
#                                or configuring fails
#
# Bring your own instead with -DWORKSHOP_ONNX_MODEL=/path/to/model_forward.onnx.

include_guard(GLOBAL)

# One release for the models. A new export uploads a new archive there, under a
# new name, and the hashes below change with it; the hash check then fetches
# the new files into every checkout.
set(WORKSHOP_MODELS_TAG "workshop-model")
set(WORKSHOP_MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../steps/common/assets/models"
    CACHE PATH "Where the models live")
cmake_path(SET WORKSHOP_MODELS_DIR NORMALIZE "${WORKSHOP_MODELS_DIR}")

set(_workshop_models_archive "workshop-models.zip")
set(_workshop_models_archive_sha256 "64f7e8382303351cecb8df9c37d71be0275e58b208c994ea7e70f5426444b85e")

# Every file in the archive, by its path inside it, with its sha256, as
# "path|sha256": a semicolon would be a list separator.
set(_workshop_models
    # Steps 1 to 9: the forward model
    "rave/model_forward.onnx|a75d50597aa8d4287ba5de73fd933d2df1c0f07c99539f5c88b1d7f694ad6f26"
    # Steps 10 and 11: the model taken apart; step 11 runs the decoder alone
    "rave/model_encoder.onnx|7c202eede08c638b75a2cd67b571ad3d15a6d8bbd1928a1f71f5a96f564d6d9a"
    "rave/model_decoder.onnx|66ce39f2b5fc9122852c088b16cde5872a5541dceb59e601262d25f78bf3a4a8"
    # Step 11: MediaPipe's hand tracking chain as ONNX (Apache 2.0), via anira-cam
    "handtracking/hand_detector.onnx|431501d81309306a0526d333df5ff23eccb4e01987bc20d19c56e47a001f2873"
    "handtracking/hand_landmarks_detector.onnx|b4ac6e8ad824556264bd2e76141881904beeab80b63c9de56449dedbadf23471"
    "handtracking/gesture_embedder.onnx|b58c43055433d21c4075a0a5bdb9d641f418960dbbbf0a8673558a8b2ed6991e"
    "handtracking/canned_gesture_classifier.onnx|0878082b485c565b9d95ccd805bf2d83e9a7cebe198d7899f26094d626433cd4")

# Whether every model file is there and matches its SHA256.
function(_workshop_models_intact out)
    set(${out} TRUE PARENT_SCOPE)
    foreach(entry IN LISTS _workshop_models)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 expected_sha)
        set(file "${WORKSHOP_MODELS_DIR}/${name}")
        if(NOT EXISTS "${file}")
            set(${out} FALSE PARENT_SCOPE)
            return()
        endif()
        file(SHA256 "${file}" actual_sha)
        if(NOT actual_sha STREQUAL expected_sha)
            message(STATUS "Workshop: ${name} does not match the release")
            set(${out} FALSE PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# Nothing to do when every file is intact, else the archive is downloaded into
# the build tree, unpacked into the models folder, and deleted. A room
# configures at once, so a failed download is retried.
function(workshop_fetch_models)
    _workshop_models_intact(intact)
    if(intact)
        return()
    endif()

    set(url "https://github.com/anira-project/realtime-inference-workshop/releases/download/${WORKSHOP_MODELS_TAG}/${_workshop_models_archive}")
    set(archive "${CMAKE_BINARY_DIR}/${_workshop_models_archive}")

    foreach(attempt RANGE 1 3)
        message(STATUS "Workshop: downloading ${_workshop_models_archive} (about 245 MB)")
        file(DOWNLOAD "${url}" "${archive}" TLS_VERIFY ON STATUS status)
        list(GET status 0 code)
        if(code EQUAL 0)
            file(SHA256 "${archive}" actual_sha)
            if(actual_sha STREQUAL _workshop_models_archive_sha256)
                break()
            endif()
        endif()
        file(REMOVE "${archive}")
        if(attempt EQUAL 3)
            message(FATAL_ERROR "Workshop: could not download ${url}")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 3)
    endforeach()

    set(names "")
    foreach(entry IN LISTS _workshop_models)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(APPEND names "${name}")
    endforeach()
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${WORKSHOP_MODELS_DIR}" PATTERNS ${names})
    file(REMOVE "${archive}")

    _workshop_models_intact(intact)
    if(NOT intact)
        message(FATAL_ERROR "Workshop: the files of ${_workshop_models_archive} do not match their SHA256")
    endif()
endfunction()
