# clangd reads build/compile_commands.json (.clangd). The presets build into
# build/release/ and build/debug/, so every configure links build/compile_commands.json
# to its own tree's database: clangd follows the build folder configured last. As in
# anira (tanh_link_compile_commands in tanh-tooling).
#
# Nothing to do when the tree is build/ itself (the database is already there), when the
# generator writes no database (Visual Studio, Xcode), or when build/compile_commands.json
# is a file of a tree configured into build/ directly: that one is left as it is. The link
# is made at configure time and the database written at generate time, so on a tree's
# first configure it points nowhere for a moment.
function(workshop_link_compile_commands)
    set(dir "${CMAKE_SOURCE_DIR}/build")
    get_filename_component(dir_real "${dir}" REALPATH)
    get_filename_component(tree_real "${CMAKE_BINARY_DIR}" REALPATH)
    if(dir_real STREQUAL tree_real)
        return()
    endif()
    if(NOT CMAKE_EXPORT_COMPILE_COMMANDS OR NOT CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
        return()
    endif()

    set(link "${dir}/compile_commands.json")
    if(EXISTS "${link}" AND NOT IS_SYMLINK "${link}")
        message(STATUS "compile_commands.json: ${link} belongs to a build in build/ itself, not linked")
        return()
    endif()

    # Relative, so the link survives moving the checkout
    file(RELATIVE_PATH target "${dir_real}" "${tree_real}/compile_commands.json")
    if(CMAKE_HOST_WIN32)
        file(TO_NATIVE_PATH "${target}" target)
    endif()
    file(MAKE_DIRECTORY "${dir}")
    file(CREATE_LINK "${target}" "${link}" SYMBOLIC RESULT result)
    if(result EQUAL 0)
        message(STATUS "compile_commands.json for clangd: ${link} -> ${target}")
    else()
        message(STATUS "compile_commands.json: cannot link ${link} -> ${target}: ${result}")
    endif()
endfunction()
