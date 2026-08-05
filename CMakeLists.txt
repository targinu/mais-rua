cmake_minimum_required(VERSION 3.22)
project(MaisRua VERSION 1.2.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY    https://github.com/juce-framework/JUCE.git
    GIT_TAG           8.0.4        # troque a tag se quiser outra release
    GIT_SHALLOW       TRUE
    UPDATE_DISCONNECTED TRUE)      # não fica checando/atualizando o clone a cada reconfigure
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MaisRua
    COMPANY_NAME             "FrozenShade"
    PLUGIN_MANUFACTURER_CODE Rua1
    PLUGIN_CODE              Mru1
    FORMATS                  VST3
    PRODUCT_NAME             "Mais Rua"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    VST3_CATEGORIES          Fx Distortion
    COPY_PLUGIN_AFTER_BUILD  TRUE)

target_sources(MaisRua PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Achievements.cpp)

target_compile_definitions(MaisRua PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    MAISRUA_VERSION="${PROJECT_VERSION}")

target_link_libraries(MaisRua
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
