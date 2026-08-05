#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>

class MaisRuaAudioProcessor : public juce::AudioProcessor
{
public:
    MaisRuaAudioProcessor();
    ~MaisRuaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mais Rua"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // lidos pela UI para os visuais
    float getRuaValue()    const noexcept { return ruaParam != nullptr ? ruaParam->load() : 0.0f; }
    float getModoValue()   const noexcept { return modoParam != nullptr ? modoParam->load() : 0.0f; }
    float getOutputLevel() const noexcept { return outputLevel.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* ruaParam  { nullptr };
    std::atomic<float>* modoParam { nullptr };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ruaSmoothed;

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefs  = juce::dsp::IIR::Coefficients<float>;
    juce::dsp::ProcessorDuplicator<Filter, Coefs> lowShelf, highCut;

    // oversampling 2x só pra etapa não-linear (drive/saturação + bitcrush), reduz aliasing
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // atrasa o caminho dry pra alinhar com a latência que o oversampling introduz no wet,
    // senão o blend wet/dry vira um comb filter quando rua > 0
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 1 };

    juce::HeapBlock<float> ruaBuf;
    juce::AudioBuffer<float> dryBuffer;

    // sample-and-hold por canal (downsampling)
    std::array<float, 2> holdSample { { 0.0f, 0.0f } };
    std::array<int,   2> holdCount  { { 0, 0 } };

    double currentSampleRate { 44100.0 };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaisRuaAudioProcessor)
};
