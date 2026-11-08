# One plugin on the shared shell of steps/common/plugin, for steps 8 and 9 and the bonus:
# PluginBase, the source player, the window. The step adds its own sources.
#
#   workshop_add_plugin(<target> NAME <product name> CODE <4 chars> [SOURCES ...])
#
# VST3 and Standalone. The example is common/assets/audio/example_1.wav,
# compiled in once as workshop_plugin_audio and shared by every plugin. Needs
# JUCE (workshop_setup_juce) before the first call.

include_guard(GLOBAL)

function(workshop_add_plugin target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "NAME;CODE" "SOURCES")
    set(shell "${CMAKE_SOURCE_DIR}/steps/common/plugin")

    if(NOT TARGET workshop_plugin_audio)
        juce_add_binary_data(workshop_plugin_audio SOURCES
            "${CMAKE_SOURCE_DIR}/steps/common/assets/audio/example_1.wav")
    endif()

    juce_add_plugin(${target}
        VERSION 0.1.0
        PRODUCT_NAME "${arg_NAME}"
        COMPANY_NAME "anira-project"
        PLUGIN_MANUFACTURER_CODE Anir
        PLUGIN_CODE ${arg_CODE}
        FORMATS VST3 Standalone
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(${target} PRIVATE
        "${shell}/PluginBase.cpp"
        "${shell}/ui/PluginEditor.cpp"
        ${arg_SOURCES})

    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_include_directories(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}/steps" "${shell}" "${shell}/ui" "${shell}/dsp")

    target_link_libraries(${target}
        PRIVATE
            workshop_plugin_audio
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags)
endfunction()
