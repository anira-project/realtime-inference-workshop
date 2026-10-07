# JUCE for the plugin step. Off by default: cloning and building JUCE takes
# minutes, and the first seven steps do not need it.
#
#   cmake -S . -B build -DWORKSHOP_JUCE=ON

include_guard(GLOBAL)
include(FetchContent)

set(WORKSHOP_JUCE_VERSION "8.0.14" CACHE STRING "JUCE tag to build the plugin against")

macro(workshop_setup_juce)
    if(NOT TARGET juce::juce_audio_processors)
        message(STATUS "Workshop: fetching JUCE ${WORKSHOP_JUCE_VERSION}")
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG "${WORKSHOP_JUCE_VERSION}"
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(JUCE)
    endif()
endmacro()
