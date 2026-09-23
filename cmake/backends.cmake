# Prebuilt inference engines, downloaded from the anira-project/backends release.
#
# Nothing is built in the room: each engine is a zip with include/ and lib/ (and
# share/ for LibTorch), unpacked into the build tree.
#
#   workshop_setup_libtorch()  -> target Torch::Torch (from LibTorch's own config)
#
# Bring your own build instead with -DWORKSHOP_LIBTORCH_ROOTDIR=/path/to/libtorch.

include_guard(GLOBAL)
include(FetchContent)

set(WORKSHOP_BACKENDS_VERSION "v2.4.0" CACHE STRING "anira-project/backends release tag")
set(WORKSHOP_LIBTORCH_VERSION "2.12.0" CACHE STRING "LibTorch version in that release")
set(WORKSHOP_LIBTORCH_ROOTDIR "" CACHE PATH "Use this prebuilt LibTorch tree instead of downloading")

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

# A macro, not a function: find_package() sets TORCH_INCLUDE_DIRS and friends in
# the calling scope, and a function would keep them to itself.
macro(workshop_setup_libtorch)
    if(WORKSHOP_LIBTORCH_ROOTDIR)
        set(root "${WORKSHOP_LIBTORCH_ROOTDIR}")
    else()
        _workshop_platform(platform)
        set(asset "libtorch-${WORKSHOP_LIBTORCH_VERSION}-${platform}-shared.zip")
        set(url "https://github.com/anira-project/backends/releases/download/${WORKSHOP_BACKENDS_VERSION}/${asset}")
        message(STATUS "Workshop: fetching ${asset}")

        FetchContent_Declare(workshop_libtorch
            URL "${url}"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            SOURCE_SUBDIR no-cmake-project  # it is a binary tree, not a project to add
        )
        FetchContent_MakeAvailable(workshop_libtorch)
        set(root "${workshop_libtorch_SOURCE_DIR}")
    endif()

    # LibTorch ships its own CMake config; it defines the `torch` target and
    # TORCH_LIBRARIES, and needs the shared libraries on the runtime path.
    set(Torch_DIR "${root}/share/cmake/Torch" CACHE PATH "" FORCE)
    find_package(Torch REQUIRED CONFIG)
    set(WORKSHOP_LIBTORCH_LIB_DIR "${root}/lib" CACHE PATH "" FORCE)
endmacro()

# Copy the engine's shared libraries next to an executable, so it runs from the
# build tree without DYLD_/LD_LIBRARY_PATH.
function(workshop_copy_libtorch_runtime target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WORKSHOP_LIBTORCH_LIB_DIR}" "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Copying LibTorch runtime next to ${target}"
        VERBATIM
    )
endfunction()
