# The part 1 model on anira: two JSON files and AniraProcessor (prepare and
# process) replace steps 3, 6 and 7. Per variant:
#   step09_<variant>_check  drives AniraProcessor on the fake host and compares
#                           the output with the reference; always built
#   step09_<variant>        the plugin around it (VST3, Standalone); with WORKSHOP_JUCE
workshop_setup_anira()
if(WORKSHOP_JUCE)
    workshop_setup_juce()
    include("${CMAKE_SOURCE_DIR}/steps/common/plugin/plugin.cmake")
endif()

foreach(variant exercise solution)
    # The JSON files sit next to AniraProcessor.cpp; the model path inside them
    # is relative to that directory, which is all the code needs to know
    set(step_dir "${CMAKE_CURRENT_LIST_DIR}/${variant}")

    set(check step09_${variant}_check)
    add_executable(${check} check.cpp ${variant}/AniraProcessor.cpp)
    # anira's C++ header is C++20; the C ABI underneath is C11
    target_compile_features(${check} PRIVATE cxx_std_20)
    target_include_directories(${check} PRIVATE "${CMAKE_SOURCE_DIR}/steps" source)
    target_compile_definitions(${check} PRIVATE WORKSHOP_STEP_DIR="${step_dir}")
    target_link_libraries(${check} PRIVATE anira::anira)
    if(WORKSHOP_RTSAN)
        target_compile_options(${check} PRIVATE -fsanitize=realtime)
        target_link_options(${check} PRIVATE -fsanitize=realtime)
    endif()

    if(WORKSHOP_JUCE)
        set(plugin step09_${variant})
        string(SUBSTRING ${variant} 0 1 initial)
        workshop_add_plugin(${plugin}
            NAME "Workshop anira ${variant}"
            CODE "Wa9${initial}"
            SOURCES source/Plugin.cpp ${variant}/AniraProcessor.cpp)
        target_compile_features(${plugin} PRIVATE cxx_std_20)
        target_include_directories(${plugin} PRIVATE source)
        target_compile_definitions(${plugin} PRIVATE WORKSHOP_STEP_DIR="${step_dir}")
        target_link_libraries(${plugin} PRIVATE anira::anira)
    endif()
endforeach()
