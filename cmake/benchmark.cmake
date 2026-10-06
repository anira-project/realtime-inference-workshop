# Google Benchmark, fetched at configure time — the same library anira uses for
# its own benchmarks.
#
#   workshop_setup_benchmark()  -> target benchmark::benchmark

include_guard(GLOBAL)
include(FetchContent)

set(WORKSHOP_BENCHMARK_VERSION "v1.9.5" CACHE STRING "google/benchmark tag to build against")

macro(workshop_setup_benchmark)
    if(NOT TARGET benchmark::benchmark)
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
        # Apple Silicon: the std::regex probe needs an answer when cross-building
        set(RUN_HAVE_STD_REGEX 1 CACHE INTERNAL "")

        message(STATUS "Workshop: fetching google/benchmark ${WORKSHOP_BENCHMARK_VERSION}")
        FetchContent_Declare(workshop_benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG "${WORKSHOP_BENCHMARK_VERSION}"
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(workshop_benchmark)
    endif()
endmacro()
