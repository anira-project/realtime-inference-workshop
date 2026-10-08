# The plugin: steps 1 to 7, inside JUCE. Solution only — there is nothing to
# fill in here, the point is to hear it.
workshop_setup_juce()
workshop_setup_readerwriterqueue()

# Static, so the bundle carries the engine inside it instead of looking for a
# shared library at load time. LibTorch has no static build — see step 4.
workshop_setup_onnxruntime(STATIC)

juce_add_plugin(step08_plugin
    VERSION 0.1.0
    PRODUCT_NAME "Workshop Inference"
    COMPANY_NAME "anira-project"
    PLUGIN_MANUFACTURER_CODE Anir
    PLUGIN_CODE Wsin
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(step08_plugin PRIVATE source/PluginProcessor.cpp)

target_compile_definitions(step08_plugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}"
)

target_include_directories(step08_plugin PRIVATE
    "${CMAKE_SOURCE_DIR}/steps"
    "${CMAKE_CURRENT_LIST_DIR}/source")

target_link_libraries(step08_plugin PRIVATE
    workshop::onnxruntime_static
    readerwriterqueue
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)
