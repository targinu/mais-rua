#pragma once

#include "PluginProcessor.h"

// Slider "invisível": só captura o mouse, o desenho é todo feito no editor.
struct InvisibleKnobLNF : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&, int, int, int, int,
                           float, float, float, juce::Slider&) override {}
};

class MaisRuaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit MaisRuaAudioProcessorEditor (MaisRuaAudioProcessor&);
    ~MaisRuaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawKnob (juce::Graphics&, juce::Point<float> centre, juce::Colour accent);

    MaisRuaAudioProcessor& processor;
    InvisibleKnobLNF invisibleLnf;

    juce::Slider ruaKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ruaAttachment;

    juce::ComboBox modoBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modoAttachment;

    float rua        { 0.0f };   // valor suavizado para o display
    float phase      { 0.0f };   // fase de animação
    float levelDecay { 0.0f };   // pico com decay para o glow reagir ao áudio
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaisRuaAudioProcessorEditor)
};
