cmake_minimum_required(VERSION 3.22)

project(JUCETutorial VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

juce_add_gui_app(App
    PRODUCT_NAME "Tutorial Demo")

target_sources(App PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h)

target_compile_definitions(App PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(App PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_events
    juce::juce_core)

juce_generate_juce_header(App)
