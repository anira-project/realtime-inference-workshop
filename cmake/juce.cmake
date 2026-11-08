# JUCE for the plugins of steps 8 and 9 and the app of step 10. On by default;
# -DWORKSHOP_JUCE=OFF leaves those steps out.
#
# The release's source archive, fetched like any other dependency into the
# build tree: 23 MB instead of about 100 MB for a shallow clone.

include_guard(GLOBAL)
include(FetchContent)

set(WORKSHOP_JUCE_VERSION "9.0.5")
set(_workshop_juce_sha256 "d92207f51ba728d3d195b6d5e93ecd049717a915fc250ff6844c32ea7272315a")

macro(workshop_setup_juce)
    if(NOT TARGET juce::juce_audio_processors)
        FetchContent_Declare(JUCE
            URL "https://github.com/juce-framework/JUCE/archive/refs/tags/${WORKSHOP_JUCE_VERSION}.tar.gz"
            URL_HASH "SHA256=${_workshop_juce_sha256}"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(JUCE)

        # juce_recommended_lto_flags pass a bare -flto. GCC then runs its
        # link-time (LTRANS) jobs in parallel only when it finds a GNU make
        # jobserver; without one, as in a make without -j, it runs them one
        # after another and warns "using serial compilation of N LTRANS jobs".
        # -flto=auto takes the jobserver when there is one and every CPU
        # thread otherwise; GCC 11 and up, like CMake's own IPO flags.
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
           AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11)
            set_target_properties(juce_recommended_lto_flags PROPERTIES
                INTERFACE_COMPILE_OPTIONS "$<$<CONFIG:Release>:-flto=auto>"
                INTERFACE_LINK_LIBRARIES "$<$<CONFIG:Release>:-flto=auto>")
        endif()
    endif()
endmacro()
