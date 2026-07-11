cmake_minimum_required(VERSION 3.20)
# C/OBJC/OBJCXX are required by the JUCE macOS app (.c/.m/.mm); the engine
# itself is pure C++. Enabling them project-wide is harmless for engine targets.
project(ai-arranger-engine VERSION 0.2.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(COREMIDI CoreMIDI)
find_library(COREFOUNDATION CoreFoundation)
find_library(AUDIOTOOLBOX AudioToolbox)
find_package(Threads REQUIRED)

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
    src/performance/*.cpp
)

# Split the Apple-framework I/O adapters out of the portable core so the core
# links NO Apple framework (iOS-first portability — see
# docs/gate-plans/IOS_THIN_VERTICAL_SLICE_PLAN.md §1). The target boundary is what
# matters; physical relocation into src/platform/apple/ is a cosmetic follow-up.
set(PLATFORM_APPLE_SRC
    ${CMAKE_SOURCE_DIR}/src/engine/midi/coremidi_out.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/midi/coremidi_in.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/realtime/coreaudio_driver.cpp
)
list(REMOVE_ITEM ENGINE_SRC ${PLATFORM_APPLE_SRC})

# ── Portable core (NO Apple framework) ────────────────────────────────
add_library(arranger-engine STATIC ${ENGINE_SRC})
target_include_directories(arranger-engine PUBLIC src)
target_link_libraries(arranger-engine PUBLIC Threads::Threads)

# ── Apple platform layer (CoreMIDI / CoreAudio adapters) ──────────────
# Isolates every direct Apple-framework dependency. The core sees only the
# vendor-neutral interfaces (IMidiOutputProvider, IRealtimeClock, ...).
add_library(platform-apple STATIC ${PLATFORM_APPLE_SRC})
target_include_directories(platform-apple PUBLIC src)
target_link_libraries(platform-apple PUBLIC arranger-engine
    ${COREAUDIO} ${AUDIOUNIT} ${COREMIDI} ${COREFOUNDATION} ${AUDIOTOOLBOX})

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
    target_link_libraries(${test_name} importer-sff1 platform-apple arranger-engine Threads::Threads)
    target_compile_definitions(${test_name} PRIVATE
        CORPUS_DIR="${CMAKE_SOURCE_DIR}/corpus/yamaha/sff1")
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── Gate 12 realtime performer-layer tests ────────────────────────────
file(GLOB REALTIME_TEST_SRC tests/realtime/*.cpp)
foreach(test_file ${REALTIME_TEST_SRC})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} platform-apple arranger-engine Threads::Threads)
    target_compile_definitions(${test_name} PRIVATE
        REALTIME_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/fixtures/realtime")
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── Gate 14 headless app-shell tests (session/control/ui/midi) ────────
file(GLOB SHELL_TEST_SRC tests/integration/*.cpp tests/ui/*.cpp tests/midi/*.cpp tests/performance/*.cpp)
foreach(test_file ${SHELL_TEST_SRC})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} platform-apple arranger-engine Threads::Threads)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# ── SFF1 parser CLI tool ──────────────────────────────────────────────
add_executable(sff1-parser tools/sff1_parser_cli.cpp)
target_link_libraries(sff1-parser importer-sff1 arranger-engine)

# ── Korg playback CLI (Gate 9 manual Musical Review driver) ───────────
add_executable(korg-playback tools/korg_playback_cli.cpp)
target_link_libraries(korg-playback platform-apple arranger-engine Threads::Threads)

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
