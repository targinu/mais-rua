#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // curva original: tanh puro
    inline float shapeClassic (float x) noexcept
    {
        return std::tanh (x);
    }

    // waveshaper assimétrico: metade positiva morde mais cedo (clipper racional,
    // mais harmônicos ímpares = mais "grão"), metade negativa satura mais forte
    // e mais cedo que a positiva (assimetria = harmônicos pares extras)
    inline float shapeGrit (float x) noexcept
    {
        float y;
        if (x >= 0.0f)
            y = (x / (1.0f + x)) * 1.2f;
        else
            y = std::tanh (x * 1.6f) * 0.8f;

        return juce::jlimit (-1.0f, 1.0f, y);
    }
}

MaisRuaAudioProcessor::MaisRuaAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    ruaParam  = apvts.getRawParameterValue ("rua");
    modoParam = apvts.getRawParameterValue ("modo");

    // 2x, filtro FIR equiripple: melhor rejeição de aliasing (é o ponto de
    // oversamplear), latência inteira reportada corretamente ao host
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
}

juce::AudioProcessorValueTreeState::ParameterLayout MaisRuaAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rua", 1 }, "Rua",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "modo", 1 }, "Modo",
        juce::StringArray { "Classico", "Grao" }, 0));
    return layout;
}

void MaisRuaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    ruaSmoothed.reset (sampleRate, 0.02);
    ruaSmoothed.setCurrentAndTargetValue (getRuaValue());

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) 2 };
    lowShelf.prepare (spec);
    highCut.prepare (spec);
    lowShelf.reset();
    highCut.reset();

    oversampling->initProcessing ((size_t) juce::jmax (1, samplesPerBlock));
    oversampling->reset();

    // useIntegerLatency=true garante que isto já é um inteiro exato
    const auto osLatency = (int) std::round (oversampling->getLatencyInSamples());
    setLatencySamples (osLatency);

    // atrasa o dry pelo mesmo tanto que o oversampling atrasa o wet, pra o
    // blend continuar sample-accurate em qualquer valor de rua
    dryDelay.setMaximumDelayInSamples (osLatency + 4);
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) osLatency);
    dryDelay.reset();

    ruaBuf.allocate ((size_t) juce::jmax (1, samplesPerBlock), true);

    // pré-alocado para o blend wet/dry: processBlock nunca pode alocar
    dryBuffer.setSize (2, juce::jmax (1, samplesPerBlock), false, false, true);

    holdSample.fill (0.0f);
    holdCount.fill (0);
    outputLevel.store (0.0f);
}

bool MaisRuaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void MaisRuaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh  = buffer.getNumChannels();
    const int numSmp = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSmp);

    ruaSmoothed.setTargetValue (getRuaValue());

    // cópia limpa para o blend wet/dry (buffer pré-alocado, sem allocs aqui)
    dryBuffer.setSize (numCh, numSmp, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSmp);

    // atrasa o dry pra ficar alinhado com a latência que o oversampling
    // introduz no caminho wet (senão o blend vira um comb filter)
    {
        juce::dsp::AudioBlock<float> dryBlock (dryBuffer);
        juce::dsp::ProcessContextReplacing<float> dryCtx (dryBlock);
        dryDelay.process (dryCtx);
    }

    // filtros de tom seguem o knob (por bloco basta, é um controle lento)
    const float ruaBlock  = ruaSmoothed.getTargetValue();
    const float shelfGain = juce::Decibels::decibelsToGain (juce::jmap (ruaBlock, 0.0f, 1.0f, 0.0f, 9.0f));
    *lowShelf.state = *Coefs::makeLowShelf (currentSampleRate, 120.0f, 0.7f, shelfGain);

    const float cutFreq = juce::jlimit (1000.0f, 20000.0f,
                                        juce::jmap (ruaBlock, 0.0f, 1.0f, 18000.0f, 4000.0f));
    *highCut.state = *Coefs::makeLowPass (currentSampleRate, cutFreq);

    // valores do knob por amostra (evita zipper noise)
    for (int n = 0; n < numSmp; ++n)
        ruaBuf[(size_t) n] = ruaSmoothed.getNextValue();

    const bool gritMode = modoParam != nullptr && modoParam->load() >= 0.5f;

    // estágio não-linear (drive + saturação + bitcrush + downsample) roda em
    // 2x oversampling: a saturação e o degrau do bitcrush geram harmônicos
    // que sem isso aliasariam de volta pro áudio audível
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto osBlock = oversampling->processSamplesUp (block);

        const int numOS  = (int) osBlock.getNumSamples();
        const int factor = (int) oversampling->getOversamplingFactor();

        for (int j = 0; j < numOS; ++j)
        {
            const int   n     = juce::jmin (j / factor, numSmp - 1);
            const float r     = ruaBuf[(size_t) n];
            const float drive = juce::jmap (r, 0.0f, 1.0f, 1.0f, 14.0f);
            const float crush = juce::jlimit (0.0f, 1.0f, (r - 0.35f) / 0.65f);   // entra ~35%
            const float steps = std::pow (2.0f, juce::jmap (crush, 0.0f, 1.0f, 16.0f, 5.0f));
            const int   hold  = factor * (1 + (int) std::floor (crush * 10.0f));  // mesma duração real, em amostras 2x
            const float comp  = 1.0f / (1.0f + r * 1.2f);                         // compensa loudness

            for (int ch = 0; ch < numCh; ++ch)
            {
                const int idx = juce::jmin (ch, 1);
                auto* data = osBlock.getChannelPointer ((size_t) ch);

                float s = data[j] * drive;
                s = gritMode ? shapeGrit (s) : shapeClassic (s);

                if (crush > 0.0f)
                    s = std::round (s * steps) / steps;

                if (holdCount[(size_t) idx] <= 0)
                {
                    holdSample[(size_t) idx] = s;
                    holdCount[(size_t) idx]  = hold;
                }
                --holdCount[(size_t) idx];
                s = holdSample[(size_t) idx];

                data[j] = s * comp;
            }
        }

        oversampling->processSamplesDown (block);
    }

    // shaping de tom no sinal molhado (na taxa base, depois do downsampling)
    juce::dsp::AudioBlock<float> toneBlock (buffer);
    juce::dsp::ProcessContextReplacing<float> toneCtx (toneBlock);
    lowShelf.process (toneCtx);
    highCut.process (toneCtx);

    // blend wet/dry (garante clean em rua = 0; dry já atrasado pra alinhar com o wet)
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        const auto* dryData = dryBuffer.getReadPointer (ch);
        for (int n = 0; n < numSmp; ++n)
        {
            const float mix = juce::jmin (1.0f, ruaBuf[(size_t) n] * 1.4f);
            wet[n] = dryData[n] * (1.0f - mix) + wet[n] * mix;
        }
    }

    // nível de saída para a UI
    float peak = 0.0f;
    for (int ch = 0; ch < numCh; ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSmp));
    outputLevel.store (peak);
}

juce::AudioProcessorEditor* MaisRuaAudioProcessor::createEditor()
{
    return new MaisRuaAudioProcessorEditor (*this);
}

void MaisRuaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MaisRuaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MaisRuaAudioProcessor();
}
