# JUCE for the plugin step. Off by default: building JUCE takes minutes, and
# the first seven steps do not need it.
#
#   cmake -S . -B build -DWORKSHOP_JUCE=ON
#
# The tag's source archive, not a git clone: 23 MB instead of about 100 MB for
# a shallow clone, and it goes through download.cmake (retries, SHA256, mirror)
# like the engines and the models.

include_guard(GLOBAL)
include(FetchContent)
include(download)

set(WORKSHOP_JUCE_VERSION "8.0.14")
set(_workshop_juce_sha256 "ceb18e4ac9ab5ea71f3f20240d5852707767a1789ab43d06656a296da9e62f3e")

macro(workshop_setup_juce)
    if(NOT TARGET juce::juce_audio_processors)
        workshop_download(
            "https://github.com/juce-framework/JUCE/archive/refs/tags/${WORKSHOP_JUCE_VERSION}.tar.gz"
            "${_workshop_juce_sha256}" juce_archive
            NAME "JUCE-${WORKSHOP_JUCE_VERSION}.tar.gz")
        FetchContent_Declare(JUCE
            URL "${juce_archive}"
            URL_HASH "SHA256=${_workshop_juce_sha256}"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(JUCE)
    endif()
endmacro()
