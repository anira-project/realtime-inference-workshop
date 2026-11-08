# The model files, downloaded into steps/common/assets/models/, next to the
# audio in steps/common/assets/audio/:
#
#   rave/           model_forward.onnx, model_encoder.onnx, model_decoder.onnx
#   handtracking/   MediaPipe's four hand tracking models
#
# They are not in git: 250 MB of weights would make every clone slow, and Git
# LFS has a bandwidth quota that a roomful of people runs through in an hour.
# Each file comes from Hugging Face, or from the repository's own release when
# Hugging Face fails. Files that are already there and intact are left alone,
# so a second configure needs no network.
#
#   workshop_fetch_models()   -> the assets/models folders hold every model,
#                                or configuring fails
#
# Bring your own instead with -DWORKSHOP_ONNX_MODEL=/path/to/model_forward.onnx.

include_guard(GLOBAL)

set(WORKSHOP_MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../steps/common/assets/models"
    CACHE PATH "Where the models live")
cmake_path(SET WORKSHOP_MODELS_DIR NORMALIZE "${WORKSHOP_MODELS_DIR}")

# The Hugging Face repository holds the files at the same paths as the models
# folder; the release mirrors every file under its plain name. A new export
# replaces the files in both places and the hashes below; the hash check then
# fetches the new files into every checkout.
set(_workshop_models_hf
    "https://huggingface.co/anira-project/realtime-inference-workshop/resolve/main")
set(_workshop_models_release
    "https://github.com/anira-project/realtime-inference-workshop/releases/download/workshop-model")

# Every file, by its path in the models folder, with its sha256, as
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

# Whether the file exists and matches its SHA256.
function(_workshop_model_intact file expected_sha out)
    set(${out} FALSE PARENT_SCOPE)
    if(EXISTS "${file}")
        file(SHA256 "${file}" actual_sha)
        if(actual_sha STREQUAL expected_sha)
            set(${out} TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Every file that is missing or does not match is downloaded next to its place
# and moved there once its SHA256 matches. A room configures at once, so each
# source is tried twice, Hugging Face first, then the release.
function(workshop_fetch_models)
    foreach(entry IN LISTS _workshop_models)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 path)
        list(GET parts 1 expected_sha)
        set(file "${WORKSHOP_MODELS_DIR}/${path}")
        _workshop_model_intact("${file}" "${expected_sha}" intact)
        if(intact)
            continue()
        endif()

        cmake_path(GET path FILENAME name)
        set(hf "${_workshop_models_hf}/${path}")
        set(release "${_workshop_models_release}/${name}")
        set(partial "${file}.part")
        set(done FALSE)
        foreach(url IN ITEMS "${hf}" "${hf}" "${release}" "${release}")
            message(STATUS "Workshop: downloading ${url}")
            file(DOWNLOAD "${url}" "${partial}" TLS_VERIFY ON STATUS status)
            list(GET status 0 code)
            if(code EQUAL 0)
                _workshop_model_intact("${partial}" "${expected_sha}" intact)
                if(intact)
                    file(RENAME "${partial}" "${file}")
                    set(done TRUE)
                    break()
                endif()
                message(STATUS "Workshop: ${name} from ${url} does not match its SHA256")
            endif()
            file(REMOVE "${partial}")
            execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 3)
        endforeach()
        if(NOT done)
            message(FATAL_ERROR "Workshop: could not download ${name}")
        endif()
    endforeach()
endfunction()
