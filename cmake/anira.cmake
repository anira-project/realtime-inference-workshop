# anira for part 2, fetched and built from source, pinned to one commit of the
# v3 line. ONNX Runtime only, static: the plugin's choice from step 8, and the
# engine the model files of steps 9 to 11 name.
#
#   workshop_setup_anira()   -> target anira::anira
#
# anira downloads its own prebuilt ONNX Runtime; the LibTorch of steps 1 to 7
# is not involved.

include_guard(GLOBAL)
include(FetchContent)

set(WORKSHOP_ANIRA_TAG "994e31dfc4f829a8faf60eed0afd0f19053ed255"
    CACHE STRING "anira commit or tag the part 2 steps build against")

macro(workshop_setup_anira)
    if(NOT TARGET anira::anira)
        message(STATUS "Workshop: fetching anira ${WORKSHOP_ANIRA_TAG}")
        # Plain variables, not cache entries: anira reads them as its option()
        # defaults under CMP0077, and they stay out of the workshop's own cache.
        set(BUILD_SHARED_LIBS OFF)
        set(ANIRA_WITH_LIBTORCH OFF)
        set(ANIRA_WITH_LITERT OFF)
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
