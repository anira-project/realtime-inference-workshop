# Every file the workshop fetches over HTTP goes through here: the engines, the
# models and JUCE. A room of a few hundred people configures at once, so a
# download is retried with a growing pause before it fails, checked against its
# SHA256, and kept in downloads/ for every later build directory.
#
#   workshop_download(<url> <sha256> <out_var> [NAME <file name>])
#       -> <out_var> is the local file; NAME when the URL's last part is no
#          file name of its own (a GitHub source archive ends in the tag)
#
# With -DWORKSHOP_MIRROR_URL=<url> every file comes from there instead of
# GitHub, by its file name: a laptop in the room (python3 -m http.server) or a
# USB stick (file:///Volumes/<stick>/downloads). Its content is a downloads/
# folder from a machine that has configured once.

include_guard(GLOBAL)

set(WORKSHOP_MIRROR_URL "" CACHE STRING
    "Fetch engines and models from this folder URL instead of GitHub")
set(WORKSHOP_DOWNLOAD_DIR "${CMAKE_CURRENT_LIST_DIR}/../downloads" CACHE PATH
    "Where downloaded archives are kept, shared by every build directory")
cmake_path(SET WORKSHOP_DOWNLOAD_DIR NORMALIZE "${WORKSHOP_DOWNLOAD_DIR}")

set(_workshop_download_attempts 5)

function(workshop_download url sha256 out_var)
    cmake_parse_arguments(PARSE_ARGV 3 arg "" "NAME" "")
    cmake_path(GET url FILENAME name)
    if(arg_NAME)
        set(name "${arg_NAME}")
    endif()
    set(file "${WORKSHOP_DOWNLOAD_DIR}/${name}")
    set(${out_var} "${file}" PARENT_SCOPE)

    # Already there and intact: nothing to do, and no network needed.
    if(EXISTS "${file}")
        file(SHA256 "${file}" actual)
        if(actual STREQUAL sha256)
            return()
        endif()
        message(STATUS "Workshop: ${name} does not match its SHA256, downloading again")
    endif()

    if(WORKSHOP_MIRROR_URL)
        string(REGEX REPLACE "/+$" "" mirror "${WORKSHOP_MIRROR_URL}")
        set(url "${mirror}/${name}")
    endif()

    set(pause 2)
    foreach(attempt RANGE 1 ${_workshop_download_attempts})
        message(STATUS "Workshop: downloading ${name} from ${url}")
        # The hash is checked here, not with EXPECTED_HASH: that turns a failed
        # request (a 429, a dropped connection) into a hard error, never retried.
        file(DOWNLOAD "${url}" "${file}.part" TLS_VERIFY ON STATUS status)
        list(GET status 0 code)
        list(GET status 1 reason)
        if(code EQUAL 0)
            file(SHA256 "${file}.part" actual)
            if(actual STREQUAL sha256)
                file(RENAME "${file}.part" "${file}")
                return()
            endif()
            set(reason "SHA256 ${actual}, expected ${sha256}")
        endif()

        file(REMOVE "${file}.part")
        if(attempt LESS _workshop_download_attempts)
            message(STATUS "Workshop: ${name} failed (${reason}), trying again in ${pause} s")
            execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${pause})
            math(EXPR pause "${pause} * 2")
        endif()
    endforeach()

    message(FATAL_ERROR
        "Workshop: could not download ${name} after ${_workshop_download_attempts} attempts (${reason}).\n"
        "Ask for the USB stick or the room's mirror and configure again with\n"
        "  -DWORKSHOP_MIRROR_URL=<the URL you are given>")
endfunction()
