cmake_minimum_required(VERSION 3.20)
project(ai-arranger-engine VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(COREMIDI CoreMIDI)
find_library(COREFOUNDATION CoreFoundation)

# ── Engine library (vendor-agnostic runtime) ──────────────────────────
file(GLOB_RECURSE ENGINE_SRC
    src/engine/realtime/*.cpp
    src/engine/midi/*.cpp
    src/engine/arranger/*.cpp
    src/engine/uasf/*.cpp
    src/engine/music/*.cpp
    src/engine/articulation/*.cpp
    src/engine/performance/*.cpp
    src/engine/control/*.cpp
    src/engine/chord/*.cpp
    src/engine/integration/*.cpp
    src/session/*.cpp
    src/control/*.cpp
    src/ui/*.cpp
    src/midi/*.cpp
)

add_library(arranger-engine STATIC ${ENGINE_SRC})
target_include_directories(arranger-engine PUBLIC src)
target_link_libraries(arranger-engine ${COREAUDIO} ${AUDIOUNIT} ${COREMIDI} ${COREFOUNDATION})

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

# ── Gate 12 realtime performer-layer tests ────────────────────────────
file(GLOB REALTIME_TEST_SRC tests/realtime/*.cpp)
foreach(test_file ${REALTIME_TEST_SRC})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} arranger-engine Threads::Threads)
    target_compile_definitions(${test_name} PRIVATE
        REALTIME_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/fixtures/realtime")
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── Gate 14 headless app-shell tests (session/control/ui/midi) ────────
file(GLOB SHELL_TEST_SRC tests/integration/*.cpp tests/ui/*.cpp)
foreach(test_file ${SHELL_TEST_SRC})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} arranger-engine Threads::Threads)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── SFF1 parser CLI tool ──────────────────────────────────────────────
add_executable(sff1-parser tools/sff1_parser_cli.cpp)
target_link_libraries(sff1-parser importer-sff1 arranger-engine)

# ── Korg playback CLI (Gate 9 manual Musical Review driver) ───────────
add_executable(korg-playback tools/korg_playback_cli.cpp)
target_link_libraries(korg-playback arranger-engine Threads::Threads)

# ── KORG validation harness (Gate 11) ─────────────────────────────────
# Pre-hardware, CI-safe measurement tooling. Its CTest targets aggregate into
# the top-level test run (scripts/run_tests.sh + CI).
add_subdirectory(tools/korg_validation)

# ── Gate 14 macOS app shell (JUCE) — CI-optional ──────────────────────
# The headless app-shell modules (src/session|control|ui|midi) are always built
# into arranger-engine and CI-tested above. The JUCE GUI app is built ONLY when
# BUILD_MACOS_APP=ON (default OFF) so CI stays fast and needs no JUCE fetch.
option(BUILD_MACOS_APP "Build the JUCE macOS app shell (Gate 14)" OFF)
if(BUILD_MACOS_APP)
    add_subdirectory(apps/macos)
endif()
