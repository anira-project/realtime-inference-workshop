# The model taken apart: encoder and decoder as two handlers, the latents between them.
workshop_setup_anira()

foreach(variant exercise solution)
    set(target step10_${variant})
    add_executable(${target} ${variant}/main.cpp)
    # anira's C++ header is C++20; the C ABI underneath is C11
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/steps")
    # The JSON files sit next to main.cpp; the model path inside them is relative
    # to that directory, which is all the program needs to know
    target_compile_definitions(${target} PRIVATE WORKSHOP_STEP_DIR="${CMAKE_CURRENT_LIST_DIR}/${variant}")
    target_link_libraries(${target} PRIVATE anira::anira)

    if(WORKSHOP_RTSAN)
        target_compile_options(${target} PRIVATE -fsanitize=realtime)
        target_link_options(${target} PRIVATE -fsanitize=realtime)
    endif()
endforeach()
