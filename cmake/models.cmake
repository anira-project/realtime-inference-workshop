# The model files, downloaded from the repository's own release into
# steps/common/assets/models/, next to the audio in steps/common/assets/audio/:
#
#   rave/           model_forward.onnx, model_encoder.onnx, model_decoder.onnx,
#                   model_decoder_noise.onnx
#   handtracking/   MediaPipe's four hand tracking models
#
# They are not in git: 400 MB of weights would make every clone slow, and Git
# LFS has a bandwidth quota that a roomful of people runs through in an hour.
# Release assets have neither problem. They come as three archives, one per set
# of steps, and only those the enabled steps need are fetched. Files that are
# already there and intact are left alone, so a second configure needs no
# network.
#
#   workshop_fetch_models()   -> the assets/models folders hold what the
#                                enabled steps load, or configuring fails
#
# Bring your own instead with -DWORKSHOP_ONNX_MODEL=/path/to/model_forward.onnx.

include_guard(GLOBAL)

# One release for the model. A new export replaces its archives there and the
# hashes below; the hash check then fetches the new files into every checkout.
set(WORKSHOP_MODELS_TAG "workshop-model")
set(WORKSHOP_MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../steps/common/assets/models"
    CACHE PATH "Where the models live")
cmake_path(SET WORKSHOP_MODELS_DIR NORMALIZE "${WORKSHOP_MODELS_DIR}")

# name and sha256 of every file, as "name|sha256" — a semicolon would be a list
# separator and fall apart in the loops below. One list per archive, each
# archive with its own sha256 and the folder it goes to.
# Steps 1 to 9: the forward model
set(_workshop_models_forward_archive
    "workshop-models-forward-2.zip|c11134669ddb61340e37b3854715ec108ef7f59746cbd9531c63d3a758fd36fc")
set(_workshop_models_forward_folder "rave")
set(_workshop_models_forward
    "model_forward.onnx|a75d50597aa8d4287ba5de73fd933d2df1c0f07c99539f5c88b1d7f694ad6f26")
# Step 10 (WORKSHOP_ANIRA): the model taken apart
set(_workshop_models_split_archive
    "workshop-models-split.zip|35f4e06dbe14bca4b1069c529effca0a9660405d64b1cffb1b8245de88217ec6")
set(_workshop_models_split_folder "rave")
set(_workshop_models_split
    "model_encoder.onnx|7c202eede08c638b75a2cd67b571ad3d15a6d8bbd1928a1f71f5a96f564d6d9a"
    "model_decoder.onnx|66ce39f2b5fc9122852c088b16cde5872a5541dceb59e601262d25f78bf3a4a8")
# Step 11 (WORKSHOP_ANIRA and WORKSHOP_JUCE): the decoder as a generator, with the
# cropped latent dimensions filled with fresh noise on every block, as the model's
# own decode() does (scripts/export_model.py --decoder-noise). Not deterministic,
# so step 10 and the checks keep the zero-filled model_decoder.onnx.
set(_workshop_models_generator_archive
    "workshop-models-generator.zip|3ef4b11b3e6eb5e4d80d1caca490f3393d8e24d17849c99e032174412eaf4ab1")
set(_workshop_models_generator_folder "rave")
set(_workshop_models_generator
    "model_decoder_noise.onnx|49f2fc61b8650600dc459dce6db0d2fb288a0c6e714d9737ea27782a068929f0")
# Step 11 (WORKSHOP_ANIRA and WORKSHOP_JUCE): MediaPipe's hand tracking chain as
# ONNX (Apache 2.0), via anira-cam
set(_workshop_models_hands_archive
    "workshop-models-hands.zip|b5b945be0ade48d3cd9c88b7f41bdec0c019a2ab20dae2ef399debe685349f8f")
set(_workshop_models_hands_folder "handtracking")
set(_workshop_models_hands
    "hand_detector.onnx|431501d81309306a0526d333df5ff23eccb4e01987bc20d19c56e47a001f2873"
    "hand_landmarks_detector.onnx|b4ac6e8ad824556264bd2e76141881904beeab80b63c9de56449dedbadf23471"
    "gesture_embedder.onnx|b58c43055433d21c4075a0a5bdb9d641f418960dbbbf0a8673558a8b2ed6991e"
    "canned_gesture_classifier.onnx|0878082b485c565b9d95ccd805bf2d83e9a7cebe198d7899f26094d626433cd4")

# Whether every file of a set is in its folder and matches its SHA256.
function(_workshop_model_set_intact set out)
    set(${out} TRUE PARENT_SCOPE)
    foreach(entry IN LISTS _workshop_models_${set})
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 expected_sha)
        set(file "${WORKSHOP_MODELS_DIR}/${_workshop_models_${set}_folder}/${name}")
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

# One set: nothing to do when it is intact, else its archive is downloaded
# into the build tree, its files unpacked into their folder, and the archive
# deleted. A room configures at once, so a failed download is retried.
function(_workshop_fetch_model_set set)
    _workshop_model_set_intact(${set} intact)
    if(intact)
        return()
    endif()

    string(REPLACE "|" ";" parts "${_workshop_models_${set}_archive}")
    list(GET parts 0 archive_name)
    list(GET parts 1 archive_sha)
    set(url "https://github.com/anira-project/realtime-inference-workshop/releases/download/${WORKSHOP_MODELS_TAG}/${archive_name}")
    set(archive "${CMAKE_BINARY_DIR}/${archive_name}")

    foreach(attempt RANGE 1 3)
        message(STATUS "Workshop: downloading ${archive_name}")
        file(DOWNLOAD "${url}" "${archive}" TLS_VERIFY ON STATUS status)
        list(GET status 0 code)
        if(code EQUAL 0)
            file(SHA256 "${archive}" actual_sha)
            if(actual_sha STREQUAL archive_sha)
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
    foreach(entry IN LISTS _workshop_models_${set})
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(APPEND names "${name}")
    endforeach()
    file(ARCHIVE_EXTRACT INPUT "${archive}"
        DESTINATION "${WORKSHOP_MODELS_DIR}/${_workshop_models_${set}_folder}"
        PATTERNS ${names})
    file(REMOVE "${archive}")

    _workshop_model_set_intact(${set} intact)
    if(NOT intact)
        message(FATAL_ERROR "Workshop: the files of ${archive_name} do not match their SHA256")
    endif()
endfunction()

# Reads WORKSHOP_ANIRA and WORKSHOP_JUCE: call it after their option().
function(workshop_fetch_models)
    _workshop_fetch_model_set(forward)
    if(WORKSHOP_ANIRA)
        _workshop_fetch_model_set(split)
        if(WORKSHOP_JUCE)
            _workshop_fetch_model_set(generator)
            _workshop_fetch_model_set(hands)
        endif()
    endif()
endfunction()
