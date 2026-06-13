cmake_minimum_required(VERSION 3.20)
project(ai-arranger-engine VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)

# Engine library
file(GLOB_RECURSE ENGINE_SRC
    src/engine/realtime/*.cpp
    src/engine/midi/*.cpp
    src/engine/arranger/*.cpp
)

add_library(arranger-engine STATIC ${ENGINE_SRC})
target_include_directories(arranger-engine PUBLIC src)
target_link_libraries(arranger-engine ${COREAUDIO} ${AUDIOUNIT})

# Tests
find_package(Threads REQUIRED)

add_executable(run-tests
    tests/engine/test_clock.cpp
    tests/engine/test_scheduler.cpp
    tests/engine/test_panic.cpp
    tests/engine/test_timing_benchmark.cpp
    tests/engine/test_chord_input.cpp
    tests/engine/test_section_sequencer.cpp
    tests/engine/test_style_player.cpp
    tests/engine/test_serializer.cpp
    tests/engine/test_deserializer.cpp
    tests/engine/test_validator.cpp
)
target_link_libraries(run-tests arranger-engine Threads::Threads)
