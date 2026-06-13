cmake_minimum_required(VERSION 3.20)
project(ai-arranger-engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Engine library
file(GLOB_RECURSE ENGINE_SRC
    src/engine/realtime/*.cpp
    src/engine/midi/*.cpp
    src/engine/uasf/*.cpp
)

add_library(arranger-engine STATIC ${ENGINE_SRC})
target_include_directories(arranger-engine PUBLIC src)

# Benchmark executable
add_executable(timing-benchmark src/engine/realtime/timing_benchmark_main.cpp)
target_link_libraries(timing-benchmark arranger-engine)

# Tests
find_package(Threads REQUIRED)

add_executable(run-tests
    tests/engine/test_clock.cpp
    tests/engine/test_scheduler.cpp
    tests/engine/test_panic.cpp
    tests/engine/test_timing_benchmark.cpp
)
target_link_libraries(run-tests arranger-engine Threads::Threads)
