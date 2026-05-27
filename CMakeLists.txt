cmake_minimum_required(VERSION 3.22)

project(Erode VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_DIR "D:/JUCE" CACHE PATH "Path to the JUCE checkout")

add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_plugin(Erode
    COMPANY_NAME "yourcompany"
    COMPANY_WEBSITE "www.yourcompany.com"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "D:/music/plugin/vst3"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Hvf1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Erode")

juce_generate_juce_header(Erode)

target_sources(Erode
    PRIVATE
        Source/ErodeLookAndFeel.cpp
        Source/NoiseFilterDisplay.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp)

target_compile_definitions(Erode
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(Erode
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
