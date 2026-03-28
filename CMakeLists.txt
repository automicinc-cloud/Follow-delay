cmake_minimum_required(VERSION 3.22)
project(FollowDelay VERSION 1.0.0)

# ── Point this to your JUCE installation ───────────────────────
# Option A: Set JUCE_DIR to your JUCE folder path
# Option B: Place JUCE as a subdirectory at ./JUCE
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE framework")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

# ── Plugin target ──────────────────────────────────────────────
juce_add_plugin(FollowDelay
    COMPANY_NAME           "FollowDelay"
    PLUGIN_MANUFACTURER_CODE Fldl
    PLUGIN_CODE             Fdly
    FORMATS                 VST3 Standalone     # Add AU for macOS
    PRODUCT_NAME            "Follow Delay"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE                # Copies VST3 to system plugin folder
)

# ── Source files ───────────────────────────────────────────────
target_sources(FollowDelay
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp

        Source/util/Constants.h
        Source/util/SafeSmoother.h
        Source/util/ParameterLayoutFactory.h
        Source/util/DataStructures.h

        Source/dsp/DelayEngine.h
        Source/dsp/FeedbackToneBlock.h
        Source/dsp/AdaptiveFitEngine.h
        Source/dsp/MotionPlaybackEngine.h

        Source/midi/TriggerEngine.h
        Source/midi/MidiGestureCapture.h

        Source/model/MotionGraphModel.h
        Source/model/FractalMotionGenerator.h
        Source/model/StateSerializer.h

        Source/ui/MotionGraphComponent.h
        Source/ui/FollowDelayLookAndFeel.h
)

target_include_directories(FollowDelay
    PRIVATE
        Source
)

# ── JUCE modules ──────────────────────────────────────────────
target_link_libraries(FollowDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(FollowDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
