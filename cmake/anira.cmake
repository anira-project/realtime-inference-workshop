# anira for part 2, fetched and built from source, pinned to one commit of the
# v3 line, v3.0.0-alpha.2. ONNX Runtime only, static: the plugin's choice from
# step 8, and the engine the model files of steps 9 to 11 name.
#
#   workshop_setup_anira()   -> target anira::anira
#
# anira links the static ONNX Runtime of backends.cmake, the one the plugin
# links; the LibTorch of steps 4 and 5 is not involved.

include_guard(GLOBAL)
include(FetchContent)

# TODO: once anira's prepare accepts "budget": "measured" and "warmup": "until_stable"
# (M2b PR 4, v3.0.0-alpha.3), bump this pin and switch the contract.json of steps 9,
# 10 and 11 from {"ms": 20} and {"fixed": 3} to them, as the slides already show.
set(WORKSHOP_ANIRA_TAG "561f0408317f94676aa3d067242367c0ca92a5b8"
    CACHE STRING "anira commit or tag the part 2 steps build against")

macro(workshop_setup_anira)
    if(NOT TARGET anira::anira)
        message(STATUS "Workshop: fetching anira ${WORKSHOP_ANIRA_TAG}")
        # Plain variables, not cache entries: anira reads them as its option()
        # defaults under CMP0077, and they stay out of the workshop's own cache.
        set(BUILD_SHARED_LIBS OFF)
        # anira links the workshop's static ONNX Runtime (backends.cmake) instead
        # of downloading its own: one download, retried and checked like every
        # other, and no GitHub API call at configure, which allows 60 requests an
        # hour per IP, and a room of participants shares one.
        workshop_setup_onnxruntime(STATIC)
        set(ANIRA_ONNXRUNTIME_ROOTDIR "${WORKSHOP_ONNXRUNTIME_STATIC_DIR}")
        set(ANIRA_BACKENDS_SKIP_REMOTE_CHECK ON)
        set(ANIRA_WITH_LIBTORCH OFF)
        set(ANIRA_WITH_LITERT OFF)
        set(ANIRA_WITH_TFLITE OFF)
        set(ANIRA_WITH_EXECUTORCH OFF)
        set(ANIRA_WITH_ONNXRUNTIME ON)
        set(ANIRA_WITH_TESTS OFF)
        set(ANIRA_WITH_EXAMPLES OFF)
        FetchContent_Declare(anira
            GIT_REPOSITORY https://github.com/anira-project/anira.git
            GIT_TAG "${WORKSHOP_ANIRA_TAG}"
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(anira)
    endif()
endmacro()
