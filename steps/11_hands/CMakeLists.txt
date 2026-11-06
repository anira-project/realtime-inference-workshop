# Step 11: a camera, hand tracking and the decoder, one anira pool. A JUCE app,
# and a playground: no solution, the exercise has every mapping (HandMapping.cpp).
workshop_setup_juce()
workshop_setup_anira()

# The camera, the hand tracker and its models: given, in steps/common/hands.
include(${CMAKE_CURRENT_LIST_DIR}/../common/hands/hands.cmake)

foreach(variant exercise)
    set(target step11_${variant})
    juce_add_gui_app(${target}
        VERSION 0.1.0
        PRODUCT_NAME "Workshop Hands (${variant})"
        COMPANY_NAME "anira-project"
        BUNDLE_ID "io.github.anira-project.workshop-hands-${variant}"
        CAMERA_PERMISSION_ENABLED TRUE
        CAMERA_PERMISSION_TEXT "The camera tracks your hands to play the model.")

    target_sources(${target} PRIVATE
        source/Main.cpp
        source/MainComponent.cpp
        source/LatentDecoder.cpp
        source/HandControls.cpp
        ${variant}/HandMapping.cpp)
    workshop_add_hands(${target})

    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE source)
    target_compile_definitions(${target} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${target},JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${target},JUCE_VERSION>"
        WORKSHOP_STEP_DIR="${CMAKE_CURRENT_LIST_DIR}")

    target_link_libraries(${target}
        PRIVATE
            anira::anira
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags)
endforeach()
