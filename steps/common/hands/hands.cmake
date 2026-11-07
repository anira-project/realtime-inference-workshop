# The hands, given and hidden: a camera, MediaPipe's hand tracking on anira and
# the features of each hand (HandTracker.h). Step 11 includes this file and
# calls workshop_add_hands(<target>) on its app; it never opens what is here.
set(WORKSHOP_HANDS_DIR "${CMAKE_CURRENT_LIST_DIR}")

# MediaPipe's hand tracking chain (four ONNX models, downloaded with the other
# models) compiled into the app, under the names the tracker expects.
juce_add_binary_data(workshop_hand_models
    NAMESPACE BinaryData
    SOURCES
        "${WORKSHOP_MODELS_DIR}/handtracking/hand_detector.onnx"
        "${WORKSHOP_MODELS_DIR}/handtracking/hand_landmarks_detector.onnx"
        "${WORKSHOP_MODELS_DIR}/handtracking/gesture_embedder.onnx"
        "${WORKSHOP_MODELS_DIR}/handtracking/canned_gesture_classifier.onnx")

# The tracker from anira-cam (src/handtracking): palm detector, landmarks,
# gesture, one anira v3 handler per model on one context. The app sees it only
# through HandTracker.h.
add_library(workshop_handtracking STATIC
    ${WORKSHOP_HANDS_DIR}/handtracking/src/HandTracker.cpp
    ${WORKSHOP_HANDS_DIR}/handtracking/src/StreamedHandTracker.cpp)
target_compile_features(workshop_handtracking PUBLIC cxx_std_20)
target_include_directories(workshop_handtracking
    PUBLIC ${WORKSHOP_HANDS_DIR}/handtracking/include
    PRIVATE ${WORKSHOP_HANDS_DIR}/handtracking/src)
target_link_libraries(workshop_handtracking PUBLIC anira::anira workshop_hand_models)

# The camera on Linux: OpenCV, if installed (a warning if not). WORKSHOP_OPENCV_CAMERA
# requires it, and uses it on macOS and Windows too, to try that path without a
# Linux machine; CI sets it on Linux.
option(WORKSHOP_OPENCV_CAMERA "Step 11: read the camera through OpenCV on every platform" OFF)
set(WORKSHOP_HANDS_OPENCV OFF)
if(WORKSHOP_OPENCV_CAMERA OR (UNIX AND NOT APPLE))
    find_package(OpenCV QUIET COMPONENTS core imgproc videoio)
    if(OpenCV_FOUND)
        set(WORKSHOP_HANDS_OPENCV ON)
    elseif(WORKSHOP_OPENCV_CAMERA)
        message(FATAL_ERROR "WORKSHOP_OPENCV_CAMERA is on, but OpenCV was not found: sudo apt install libopencv-dev (macOS: brew install opencv)")
    else()
        message(WARNING
            "Step 11 has no camera on Linux without OpenCV: the app builds, but the mouse pad plays "
            "instead of your hands. Install it and configure again:\n"
            "    sudo apt install libopencv-dev")
    endif()
endif()

# The camera and the tracker's front, compiled into a JUCE app: they use its
# modules, so they are sources of the app, not a library of their own.
function(workshop_add_hands target)
    target_sources(${target} PRIVATE ${WORKSHOP_HANDS_DIR}/HandTracker.cpp)

    # macOS: AVFoundation frames (ARC for that file only; JUCE's own .mm files are MRC).
    # Windows: juce::CameraDevice. Linux: JUCE has no camera, so OpenCV
    # (apt install libopencv-dev); without it the pad plays.
    if(WORKSHOP_HANDS_OPENCV)
        target_sources(${target} PRIVATE ${WORKSHOP_HANDS_DIR}/CameraInput_opencv.cpp)
        target_link_libraries(${target} PRIVATE ${OpenCV_LIBS})
        target_include_directories(${target} SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
    elseif(APPLE)
        target_sources(${target} PRIVATE ${WORKSHOP_HANDS_DIR}/CameraInput.mm)
        set_source_files_properties(${WORKSHOP_HANDS_DIR}/CameraInput.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
        target_link_libraries(${target} PRIVATE
            "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo")
    elseif(WIN32)
        target_sources(${target} PRIVATE ${WORKSHOP_HANDS_DIR}/CameraInput.cpp)
        target_link_libraries(${target} PRIVATE juce::juce_video)
    else()
        target_sources(${target} PRIVATE ${WORKSHOP_HANDS_DIR}/CameraInput_none.cpp)
    endif()

    target_include_directories(${target} PRIVATE ${WORKSHOP_HANDS_DIR})
    target_compile_definitions(${target} PRIVATE JUCE_USE_CAMERA=$<IF:$<BOOL:${WIN32}>,1,0>)
    target_link_libraries(${target} PRIVATE workshop_handtracking)
endfunction()
