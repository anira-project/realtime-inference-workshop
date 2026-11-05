// The audio callback's promise, given complete.
//
// Goal: mark the audio callback [[clang::nonblocking]]: it returns in bounded
//       time, so it must not allocate, lock, or call the kernel. RealtimeSanitizer
//       checks that promise at runtime for everything underneath (step 5).
//       Compilers without the attribute get nothing.
#pragma once

#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif
