# Step 11: a camera, hand tracking and the decoder, one anira pool. A JUCE app;
# solution and exercise differ in HandControls.cpp only.
workshop_setup_juce()
workshop_setup_anira()

# MediaPipe's hand tracking chain (four ONNX models, downloaded with the other
# models) compiled into the app, under the names the tracker expects.
juce_add_binary_data(step11_models
    NAMESPACE BinaryData
    SOURCES
        "${WORKSHOP_MODELS_DIR}/handtracking/hand_detector.onnx"
        "${WORKSHOP_MODELS_DIR}/handtracking/hand_landmarks_detector.onnx"
        "${WORKSHOP_MODELS_DIR}/handtracking/gesture_embedder.onnx"
        "${WORKSHOP_MODELS_DIR}/handtracking/canned_gesture_classifier.onnx")

# The tracker from anira-cam (src/handtracking), given: palm detector, landmarks,
# gesture, on anira's 2.x C++ API, which the v3 library still carries. The app
# sees it only through source/HandTracker.h.
add_library(step11_handtracking STATIC
    handtracking/src/HandTracker.cpp
    handtracking/src/StreamedHandTracker.cpp
    handtracking/src/SyncHandTracker.cpp)
target_compile_features(step11_handtracking PUBLIC cxx_std_20)
target_include_directories(step11_handtracking PUBLIC handtracking/include PRIVATE handtracking/src)
target_link_libraries(step11_handtracking PUBLIC anira::anira step11_models)

# The camera on Linux: OpenCV, if installed (a warning if not). WORKSHOP_OPENCV_CAMERA
# requires it, and uses it on macOS and Windows too, to try that path without a
# Linux machine; CI sets it on Linux.
option(WORKSHOP_OPENCV_CAMERA "Step 11: read the camera through OpenCV on every platform" OFF)
set(_step11_opencv_camera OFF)
if(WORKSHOP_OPENCV_CAMERA OR (UNIX AND NOT APPLE))
    find_package(OpenCV QUIET COMPONENTS core imgproc videoio)
    if(OpenCV_FOUND)
        set(_step11_opencv_camera ON)
    elseif(WORKSHOP_OPENCV_CAMERA)
        message(FATAL_ERROR "WORKSHOP_OPENCV_CAMERA is on, but OpenCV was not found: sudo apt install libopencv-dev (macOS: brew install opencv)")
    else()
        message(WARNING
            "Step 11 has no camera on Linux without OpenCV: the app builds, but the mouse pad plays "
            "instead of your hands. Install it and configure again:\n"
            "    sudo apt install libopencv-dev")
    endif()
endif()

foreach(variant exercise solution)
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
        source/HandTracker.cpp
        ${variant}/HandControls.cpp)

    # macOS: AVFoundation frames (ARC for that file only; JUCE's own .mm files are MRC).
    # Windows: juce::CameraDevice. Linux: JUCE has no camera, so OpenCV
    # (apt install libopencv-dev); without it the pad plays.
    if(_step11_opencv_camera)
        target_sources(${target} PRIVATE source/CameraInput_opencv.cpp)
        target_link_libraries(${target} PRIVATE ${OpenCV_LIBS})
        target_include_directories(${target} SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
    elseif(APPLE)
        target_sources(${target} PRIVATE source/CameraInput.mm)
        set_source_files_properties(source/CameraInput.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
        target_link_libraries(${target} PRIVATE
            "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
    elseif(WIN32)
        target_sources(${target} PRIVATE source/CameraInput.cpp)
        target_link_libraries(${target} PRIVATE juce::juce_video)
    else()
        target_sources(${target} PRIVATE source/CameraInput_none.cpp)
    endif()

    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE source)
    target_compile_definitions(${target} PRIVATE
        JUCE_USE_CAMERA=$<IF:$<BOOL:${WIN32}>,1,0>
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${target},JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${target},JUCE_VERSION>"
        WORKSHOP_STEP_DIR="${CMAKE_CURRENT_LIST_DIR}")

    target_link_libraries(${target}
        PRIVATE
            step11_handtracking
            anira::anira
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags)
endforeach()
