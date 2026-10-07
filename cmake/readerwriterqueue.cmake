# moodycamel's single-producer single-consumer queue, header only. The audio
# thread and the worker thread hand blocks over through it without locking.
#
#   workshop_setup_readerwriterqueue()  -> target readerwriterqueue

include_guard(GLOBAL)
include(FetchContent)

set(WORKSHOP_READERWRITERQUEUE_VERSION "v1.0.7" CACHE STRING "cameron314/readerwriterqueue tag")

macro(workshop_setup_readerwriterqueue)
    if(NOT TARGET readerwriterqueue)
        message(STATUS "Workshop: fetching readerwriterqueue ${WORKSHOP_READERWRITERQUEUE_VERSION}")
        FetchContent_Declare(workshop_readerwriterqueue
            GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
            GIT_TAG "${WORKSHOP_READERWRITERQUEUE_VERSION}"
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(workshop_readerwriterqueue)
    endif()
endmacro()
