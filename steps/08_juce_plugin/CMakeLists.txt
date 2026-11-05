# The plugin: step 7's processor inside JUCE, on the shell of steps/common/plugin.
# Solution only: there is nothing to fill in here, the point is to hear it.
workshop_setup_juce()
workshop_setup_readerwriterqueue()
include("${CMAKE_SOURCE_DIR}/steps/common/plugin/plugin.cmake")

# ONNX Runtime, static, so the bundle carries the engine inside it instead of
# looking for a shared library at load time.
workshop_setup_onnxruntime(STATIC)

workshop_add_plugin(step08_plugin
    NAME "Workshop Inference"
    CODE Wsin
    SOURCES solution/PluginProcessor.cpp)

target_compile_definitions(step08_plugin PRIVATE WORKSHOP_ONNX_MODEL_PATH="${WORKSHOP_ONNX_MODEL}")
target_include_directories(step08_plugin PRIVATE "${CMAKE_CURRENT_LIST_DIR}/solution")
target_link_libraries(step08_plugin PRIVATE workshop::onnxruntime_static readerwriterqueue)
