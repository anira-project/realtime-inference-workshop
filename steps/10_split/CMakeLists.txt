# The model taken apart: encoder and decoder as two handlers, the latents
# between them, in SplitProcessor (prepare and process). Per variant:
#   step10_<variant>_check  drives SplitProcessor on the fake host and compares
#                           the output with the reference; always built
#   step10_<variant>        the plugin around it, eight offset knobs (VST3,
#                           Standalone); with WORKSHOP_JUCE
workshop_setup_anira()
if(WORKSHOP_JUCE)
    workshop_setup_juce()
    include("${CMAKE_SOURCE_DIR}/steps/common/plugin/plugin.cmake")
endif()

foreach(variant exercise solution)
    # The JSON files sit next to SplitProcessor.cpp; the model path inside them
    # is relative to that directory, which is all the code needs to know
    set(step_dir "${CMAKE_CURRENT_LIST_DIR}/${variant}")

    set(check step10_${variant}_check)
    add_executable(${check} check.cpp ${variant}/SplitProcessor.cpp)
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
        set(plugin step10_${variant})
        string(SUBSTRING ${variant} 0 1 initial)
        workshop_add_plugin(${plugin}
            NAME "Workshop split ${variant}"
            CODE "Ws0${initial}"
            SOURCES source/PluginProcessor.cpp ${variant}/SplitProcessor.cpp)
        target_compile_features(${plugin} PRIVATE cxx_std_20)
        target_include_directories(${plugin} PRIVATE source)
        target_compile_definitions(${plugin} PRIVATE WORKSHOP_STEP_DIR="${step_dir}")
        target_link_libraries(${plugin} PRIVATE anira::anira)
    endif()
endforeach()
