cmake_minimum_required(VERSION 3.22)
project(Eighty VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# JUCE: use sibling checkout if present, else fetch
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(EIGHTY_FORMATS Standalone AU VST3)

# AAX requires the Avid AAX SDK (licensed). Point AAX_SDK_PATH at it to enable.
if(DEFINED ENV{AAX_SDK_PATH} AND EXISTS "$ENV{AAX_SDK_PATH}")
    juce_set_aax_sdk_path("$ENV{AAX_SDK_PATH}")
    list(APPEND EIGHTY_FORMATS AAX)
    message(STATUS "AAX SDK found - AAX format enabled")
else()
    message(STATUS "AAX_SDK_PATH not set - building Standalone/AU/VST3 only")
endif()

juce_add_plugin(Eighty
    COMPANY_NAME "Arjun Rao"
    BUNDLE_ID com.arjunrao.eighty
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE ArjR
    PLUGIN_CODE Ei80
    FORMATS ${EIGHTY_FORMATS}
    PRODUCT_NAME "Eighty")

target_sources(Eighty PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Eighty PRIVATE Source)

target_compile_definitions(Eighty PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_PLUGINHOST_VST3=1)

target_link_libraries(Eighty PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
