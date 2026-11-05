# Prebuilt inference engines, downloaded from the anira-project/backends release.
#
# Nothing is built in the room: each engine is a zip with include/ and lib/,
# unpacked into the build tree.

include_guard(GLOBAL)
include(FetchContent)
include(download)

# The release, its engine versions and the SHA256 of every archive the workshop
# can ask for, pinned: no GitHub API call at configure, and a broken download
# never passes. A new release replaces all of them together.
set(WORKSHOP_BACKENDS_VERSION "v2.4.0")
set(WORKSHOP_ONNXRUNTIME_VERSION "1.26.0")
set(WORKSHOP_ONNXRUNTIME_ROOTDIR "" CACHE PATH "Use this prebuilt ONNX Runtime tree instead of downloading")

set(_workshop_backend_archives
    "onnxruntime-1.26.0-Linux-aarch64-shared.zip|d569ceb3b865763bb2ce445176d57989f7bc0454c2d041285a1abfe4b2cd594f"
    "onnxruntime-1.26.0-Linux-aarch64-static.zip|053517f97c7749c574c8eef4e02ef151e64625b5e17b2e7646a5927c9c177374"
    "onnxruntime-1.26.0-Linux-x86_64-shared.zip|060a5baa102327aa3d0e0a2c0a23472284569784c8ad3295da6fb19e587d32e7"
    "onnxruntime-1.26.0-Linux-x86_64-static.zip|26860e47df15e29b8f2d6804973e8c6cef20d8fb9f1f2bf0bde83a7af6d9c72f"
    "onnxruntime-1.26.0-macOS-arm64-shared.zip|9b797518948185e62dbdd2442934afae21be58e78785d293d57f4e501d835c72"
    "onnxruntime-1.26.0-macOS-arm64-static.zip|3329d8a85ec54b61a898f84f3c3e6da180868f9dbbba4b6362692ad5df9e4998"
    "onnxruntime-1.26.0-macOS-x86_64-shared.zip|ca1b7c8b8868d04a254a0c5e860379a6cd8c6e0caaf10ae8935c301504faafcf"
    "onnxruntime-1.26.0-macOS-x86_64-static.zip|20dfa6796d5f43a6723b7f1e26286429377785babc729bdfb64d7cac55d16f33"
    "onnxruntime-1.26.0-Windows-arm64-shared.zip|516293a7d87b2cbb82c0b41329195f52d576d133ae9ea97e3918883b95510452"
    "onnxruntime-1.26.0-Windows-arm64-static.zip|4305337a904597c43ec99803342cab70253413a9cdff5a2a84e0df13a4da699b"
    "onnxruntime-1.26.0-Windows-arm64-static-debug.zip|9c27aa796a5bbc38e69bc778efa60196dd55775c8ef7f1933661abc452d75881"
    "onnxruntime-1.26.0-Windows-x86_64-shared.zip|acb07b475836bfa5659f4425bf0eddeef920dd19981a9a9f70c773787722d651"
    "onnxruntime-1.26.0-Windows-x86_64-static.zip|9abd843c91eb0309d8e21c29a9d775e11025368772b0a32e25d378391fbf9749"
    "onnxruntime-1.26.0-Windows-x86_64-static-debug.zip|0971d1c3f57edbe2bfe0cc86f70ab2160e59cf01d0176ef79c3f746cc54c8dff")

# Download one engine archive of the release (or take it from downloads/) and
# unpack it into the build tree as <name>_SOURCE_DIR, returned in <out_var>.
function(_workshop_fetch_backend name asset out_var)
    set(sha256 "")
    foreach(entry IN LISTS _workshop_backend_archives)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 entry_asset)
        if(entry_asset STREQUAL asset)
            list(GET parts 1 sha256)
        endif()
    endforeach()
    if(sha256 STREQUAL "")
        message(FATAL_ERROR "Workshop: ${asset} is not an archive of the release this workshop pins")
    endif()

    workshop_download(
        "https://github.com/anira-project/backends/releases/download/${WORKSHOP_BACKENDS_VERSION}/${asset}"
        "${sha256}" archive)
    FetchContent_Declare(${name}
        URL "${archive}"
        URL_HASH "SHA256=${sha256}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR no-cmake-project  # a binary tree, not a project to add
    )
    FetchContent_MakeAvailable(${name})
    set(${out_var} "${${name}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

# <OS>-<arch> part of the asset name, as the release names them.
function(_workshop_platform out)
    if(APPLE)
        set(os "macOS")
    elseif(WIN32)
        set(os "Windows")
    else()
        set(os "Linux")
    endif()

    set(arch "${CMAKE_SYSTEM_PROCESSOR}")
    if(APPLE AND CMAKE_OSX_ARCHITECTURES)
        set(arch "${CMAKE_OSX_ARCHITECTURES}")
    endif()
    if(arch MATCHES "^(x86_64|AMD64|amd64)$")
        set(arch "x86_64")
    elseif(arch MATCHES "^(arm64|aarch64|ARM64)$")
        if(APPLE OR WIN32)
            set(arch "arm64")
        else()
            set(arch "aarch64")
        endif()
    endif()

    set(${out} "${os}-${arch}" PARENT_SCOPE)
endfunction()

# ONNX Runtime: the same release, unpacked to include/ + lib/. It ships no CMake
# config, so the imported target is built here.
#
#   workshop_setup_onnxruntime()          -> workshop::onnxruntime, the dylib
#   workshop_setup_onnxruntime(STATIC)    -> workshop::onnxruntime_static
#
# The plugin links the static one: one self-contained bundle instead of a
# shared library that has to be found at load time.
macro(workshop_setup_onnxruntime)
    set(ort_linkage "shared")
    set(ort_target "workshop::onnxruntime")
    if("${ARGN}" MATCHES "STATIC")
        set(ort_linkage "static")
        set(ort_target "workshop::onnxruntime_static")
    endif()

    if(NOT TARGET ${ort_target})
        if(WORKSHOP_ONNXRUNTIME_ROOTDIR)
            set(ort_root "${WORKSHOP_ONNXRUNTIME_ROOTDIR}")
        else()
            _workshop_platform(ort_platform)
            # MSVC's debug runtime needs the static library built against it
            set(ort_token "${ort_linkage}")
            if(WIN32 AND ort_linkage STREQUAL "static" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(ort_token "static-debug")
            endif()
            _workshop_fetch_backend(workshop_onnxruntime_${ort_linkage}
                "onnxruntime-${WORKSHOP_ONNXRUNTIME_VERSION}-${ort_platform}-${ort_token}.zip" ort_root)
        endif()

        if(ort_linkage STREQUAL "static")
            file(GLOB ort_library "${ort_root}/lib/libonnxruntime.a" "${ort_root}/lib/onnxruntime.lib")
            list(GET ort_library 0 ort_library)

            add_library(workshop::onnxruntime_static STATIC IMPORTED GLOBAL)
            set_target_properties(workshop::onnxruntime_static PROPERTIES
                IMPORTED_LOCATION "${ort_library}"
                INTERFACE_INCLUDE_DIRECTORIES "${ort_root}/include")
            # anira links the same tree (anira.cmake): one download, one copy
            set(WORKSHOP_ONNXRUNTIME_STATIC_DIR "${ort_root}" CACHE INTERNAL "")
        else()
            # The file to load at runtime, and on Windows the import library the
            # linker needs next to it.
            file(GLOB ort_library
                "${ort_root}/lib/libonnxruntime.dylib"
                "${ort_root}/lib/libonnxruntime.so*"
                "${ort_root}/lib/onnxruntime.dll")
            list(GET ort_library 0 ort_library)

            add_library(workshop::onnxruntime SHARED IMPORTED GLOBAL)
            set_target_properties(workshop::onnxruntime PROPERTIES
                IMPORTED_LOCATION "${ort_library}"
                INTERFACE_INCLUDE_DIRECTORIES "${ort_root}/include")

            if(WIN32)
                set_target_properties(workshop::onnxruntime PROPERTIES
                    IMPORTED_IMPLIB "${ort_root}/lib/onnxruntime.lib")
            endif()
            set(WORKSHOP_ONNXRUNTIME_LIB_DIR "${ort_root}/lib" CACHE PATH "" FORCE)
        endif()
    endif()
endmacro()

# Copy the ONNX Runtime shared library next to an executable.
function(workshop_copy_onnxruntime_runtime target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WORKSHOP_ONNXRUNTIME_LIB_DIR}" "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Copying the ONNX Runtime next to ${target}"
        VERBATIM
    )
endfunction()
