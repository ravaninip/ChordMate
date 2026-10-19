cmake_minimum_required(VERSION 3.22)
project(ChordMate VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHORDMATE_BUILD_PLUGIN "Build the JUCE plugin (downloads JUCE)" ON)
option(CHORDMATE_BUILD_TESTS  "Build the theory-engine tests" ON)
option(CHORDMATE_BUILD_PLUGIN_TESTS "Host-less smoke test of the plugin processor and editor" OFF)
option(CHORDMATE_COPY_AFTER_BUILD "Copy the plugin into the user's VST3/AU folder after building" OFF)
set(CHORDMATE_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (empty = download from GitHub)")
set(CHORDMATE_JUCE_TAG "8.0.15" CACHE STRING "JUCE tag to download")

# ---------------------------------------------------------------- theory engine (pure C++)
add_library(chordmate_engine STATIC
    Source/Engine/Theory.cpp
    Source/Engine/MidiWriter.cpp
    Source/Engine/Project.cpp
    Source/Engine/Style.cpp)
target_include_directories(chordmate_engine PUBLIC Source)
set_target_properties(chordmate_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(chordmate_engine PRIVATE /utf-8)
endif()

if(CHORDMATE_BUILD_TESTS)
    enable_testing()
    add_executable(chordmate_tests Tests/theory_tests.cpp)
    target_link_libraries(chordmate_tests PRIVATE chordmate_engine)
    if(MSVC)
        target_compile_options(chordmate_tests PRIVATE /utf-8)
    endif()
    add_test(NAME engine COMMAND chordmate_tests)
endif()

# ---------------------------------------------------------------- plugin
if(CHORDMATE_BUILD_PLUGIN)
    if(CHORDMATE_JUCE_PATH)
        add_subdirectory(${CHORDMATE_JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG ${CHORDMATE_JUCE_TAG}
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    set(CHORDMATE_FORMATS VST3)
    if(APPLE)
        list(APPEND CHORDMATE_FORMATS AU)
    endif()
    if(CHORDMATE_STANDALONE)
        list(APPEND CHORDMATE_FORMATS Standalone)
    endif()

    juce_add_plugin(ChordMate
        COMPANY_NAME "Paolo"
        PLUGIN_MANUFACTURER_CODE Paol
        PLUGIN_CODE Cmat
        PRODUCT_NAME "ChordMate"
        FORMATS ${CHORDMATE_FORMATS}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES Instrument Synth
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        COPY_PLUGIN_AFTER_BUILD ${CHORDMATE_COPY_AFTER_BUILD})

    target_sources(ChordMate PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    target_compile_definitions(ChordMate PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0)

    if(MSVC)
        target_compile_options(ChordMate PRIVATE /utf-8)
    endif()

    target_link_libraries(ChordMate
        PRIVATE
            chordmate_engine
            juce::juce_audio_basics
            juce::juce_audio_processors
            juce::juce_gui_basics
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    if(CHORDMATE_BUILD_PLUGIN_TESTS)
        enable_testing()
        add_executable(chordmate_plugin_smoke
            Tests/plugin_smoke.cpp
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp)
        target_include_directories(chordmate_plugin_smoke PRIVATE Source)
        target_compile_definitions(chordmate_plugin_smoke PRIVATE
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
        target_link_libraries(chordmate_plugin_smoke PRIVATE
            chordmate_engine
            juce::juce_audio_basics
            juce::juce_audio_processors
            juce::juce_gui_basics
            juce::juce_recommended_config_flags)
        if(MSVC)
            target_compile_options(chordmate_plugin_smoke PRIVATE /utf-8)
        endif()
        add_test(NAME plugin_smoke COMMAND chordmate_plugin_smoke)
    endif()
endif()
