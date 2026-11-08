# anira for part 2, fetched and built from source, pinned to the v3.0.0-alpha.3
# tag (ABI 0.3): the measured budget, the until-stable warm-up, the optional
# window and the latency fixes. ONNX Runtime only, static: the plugin's choice from
# step 8, and the engine the model files of steps 9 and 10 and the bonus name.
#
#   workshop_setup_anira()   -> target anira::anira
#
# anira links the static ONNX Runtime of backends.cmake, the one the plugin
# links; the LibTorch of steps 4 and 5 is not involved.

include_guard(GLOBAL)
include(FetchContent)

set(_workshop_anira_pin "v3.0.0-alpha.3")
set(WORKSHOP_ANIRA_TAG "${_workshop_anira_pin}"
    CACHE STRING "anira commit or tag the part 2 steps build against")
# A build folder configured before this pin keeps the old one in its cache:
# move it along, unless someone chose another commit on purpose.
if(WORKSHOP_ANIRA_TAG MATCHES "^(561f0408317f94676aa3d067242367c0ca92a5b8|131b75de99b85fbe055aaff7cc5e8a4dcadf1dde)$")
    set(WORKSHOP_ANIRA_TAG "${_workshop_anira_pin}" CACHE STRING
        "anira commit or tag the part 2 steps build against" FORCE)
endif()

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
