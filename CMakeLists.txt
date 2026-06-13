cmake_minimum_required(VERSION 3.20)
project(ai-arranger-engine VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)

# ── Engine library (vendor-agnostic runtime) ──────────────────────────
file(GLOB_RECURSE ENGINE_SRC
    src/engine/realtime/*.cpp
    src/engine/midi/*.cpp
    src/engine/arranger/*.cpp
    src/engine/uasf/*.cpp
)

add_library(arranger-engine STATIC ${ENGINE_SRC})
target_include_directories(arranger-engine PUBLIC src)
target_link_libraries(arranger-engine ${COREAUDIO} ${AUDIOUNIT})

# ── Importer library (vendor SFF1/SFF2 isolation) ─────────────────────
file(GLOB_RECURSE IMPORTER_SFF1_SRC
    src/importers/sff1/*.cpp
)

add_library(importer-sff1 STATIC ${IMPORTER_SFF1_SRC})
target_include_directories(importer-sff1 PUBLIC src)
target_link_libraries(importer-sff1 arranger-engine)

# ── Tests ─────────────────────────────────────────────────────────────
# Each test file is a standalone program with its own main(). We build
# one executable per file and register it with CTest, rather than linking
# them all into a single binary (which would collide on main()).
find_package(Threads REQUIRED)
enable_testing()

file(GLOB TEST_SRC tests/engine/*.cpp)
foreach(test_file ${TEST_SRC})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} importer-sff1 arranger-engine Threads::Threads)
    target_compile_definitions(${test_name} PRIVATE
        CORPUS_DIR="${CMAKE_SOURCE_DIR}/corpus/yamaha/sff1")
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── SFF1 parser CLI tool ──────────────────────────────────────────────
add_executable(sff1-parser tools/sff1_parser_cli.cpp)
target_link_libraries(sff1-parser importer-sff1 arranger-engine)
