# ── Gate 14 — JUCE macOS app shell (CI-optional) ──────────────────────
# Configured only when BUILD_MACOS_APP=ON (see top-level CMakeLists). Fetches a
# pinned JUCE 7.x. The app is a pure client of arranger-engine: it sends intent
# through the engine bridge and renders ViewModel state. No DSP/audio is done by
# the app. Requires network to fetch JUCE; intended for local/dev builds.

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4    # macOS 15+/SDK-26 compatible (ScreenCaptureKit)
)
FetchContent_MakeAvailable(juce)

file(GLOB MACOS_APP_SRC Source/*.cpp)

juce_add_gui_app(aiarranger-macos
    PRODUCT_NAME "AI Arranger"
    COMPANY_NAME "AIArranger"
    BUNDLE_ID "com.aiarranger.macos")

target_sources(aiarranger-macos PRIVATE ${MACOS_APP_SRC})

target_compile_definitions(aiarranger-macos PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="AI Arranger"
    JUCE_APPLICATION_VERSION_STRING="0.14.0")

# The shell consumes the engine + the headless app-shell modules (session/
# control/ui/midi) that live in arranger-engine.
target_link_libraries(aiarranger-macos PRIVATE
    arranger-engine
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices)
