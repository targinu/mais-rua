#include "PluginEditor.h"
#include <cmath>

namespace
{
    const float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
    const float kEndAngle   = juce::MathConstants<float>::pi * 2.8f;
}

MaisRuaAudioProcessorEditor::MaisRuaAudioProcessorEditor (MaisRuaAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    ruaKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    ruaKnob.setRotaryParameters (kStartAngle, kEndAngle, true);
    ruaKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    ruaKnob.setLookAndFeel (&invisibleLnf);
    addAndMakeVisible (ruaKnob);

    ruaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "rua", ruaKnob);

    modoBox.addItem ("Classico", 1);
    modoBox.addItem ("Grao", 2);
    modoBox.setJustificationType (juce::Justification::centred);
    modoBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff17171b));
    modoBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    modoBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.2f));
    addAndMakeVisible (modoBox);

    modoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "modo", modoBox);

    setSize (360, 440);
    startTimerHz (45);
}

MaisRuaAudioProcessorEditor::~MaisRuaAudioProcessorEditor()
{
    ruaKnob.setLookAndFeel (nullptr);
    stopTimer();
}

void MaisRuaAudioProcessorEditor::timerCallback()
{
    phase += 0.15f;
    const float target = processor.getRuaValue();
    rua += (target - rua) * 0.25f;
    levelDecay = juce::jmax (levelDecay * 0.88f, processor.getOutputLevel());
    repaint();
}

void MaisRuaAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    const juce::Colour cool (0xff20e0d0);
    const juce::Colour hot  (0xffff3b1f);
    const juce::Colour accent = cool.interpolatedWith (hot, rua);

    // ---------- camada de tela (estável) ----------
    g.fillAll (juce::Colour (0xff0a0a0c).interpolatedWith (juce::Colour (0xff1c0705), rua * 0.6f));

    // granulado / film grain
    const int grain = (int) (rua * 500.0f);
    for (int i = 0; i < grain; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (rng.nextFloat() * 0.12f * rua));
        g.fillRect (rng.nextFloat() * w, rng.nextFloat() * h, 1.0f, 1.0f);
    }

    // ---------- camada de conteúdo (treme com rua alto) ----------
    {
        juce::Graphics::ScopedSaveState save (g);
        if (rua > 0.5f)
        {
            const float amt = (rua - 0.5f) * 8.0f;
            g.addTransform (juce::AffineTransform::translation (
                (rng.nextFloat() - 0.5f) * amt,
                (rng.nextFloat() - 0.5f) * amt));
        }

        // glow radial atrás do knob, reagindo ao áudio
        auto centre = bounds.getCentre().translated (0.0f, 12.0f);
        const float pulse = std::sin (phase) * 6.0f;
        const float glowR = 130.0f + pulse * (0.3f + rua) + levelDecay * 60.0f;
        juce::ColourGradient glow (accent.withAlpha (0.30f + 0.45f * rua),
                                   centre.x, centre.y,
                                   accent.withAlpha (0.0f),
                                   centre.x, centre.y + glowR, true);
        g.setGradientFill (glow);
        g.fillEllipse (centre.x - glowR, centre.y - glowR, glowR * 2.0f, glowR * 2.0f);

        drawKnob (g, centre, accent);

        // fatias de glitch no topo do range
        if (rua > 0.65f)
        {
            const int slices = (int) ((rua - 0.65f) / 0.35f * 7.0f);
            for (int i = 0; i < slices; ++i)
            {
                const float sy  = rng.nextFloat() * h;
                const float sh  = 2.0f + rng.nextFloat() * 9.0f;
                const float off = (rng.nextFloat() - 0.5f) * 34.0f * rua;
                g.setColour (accent.withAlpha (0.22f));
                g.fillRect (off, sy, w, sh);
            }
        }

        // título com aberração cromática
        auto titleArea = juce::Rectangle<float> (0.0f, 24.0f, w, 70.0f);
        const float ab = rua * 5.0f + (rua > 0.6f ? (rng.nextFloat() - 0.5f) * rua * 6.0f : 0.0f);
        g.setFont (juce::Font (juce::FontOptions (46.0f, juce::Font::bold)));
        g.setColour (juce::Colour (0xff00ffff).withAlpha (0.55f * (0.35f + rua)));
        g.drawText ("MAIS RUA", titleArea.translated (-ab, 0.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xffff00ff).withAlpha (0.55f * (0.35f + rua)));
        g.drawText ("MAIS RUA", titleArea.translated (ab, 0.0f), juce::Justification::centred);
        g.setColour (juce::Colours::white);
        g.drawText ("MAIS RUA", titleArea, juce::Justification::centred);

        // percentual
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.setColour (accent);
        g.drawText (juce::String (juce::roundToInt (rua * 100.0f)) + "%",
                    bounds.withTop (h - 42.0f), juce::Justification::centred);
    }

    // scanlines por cima (camada de tela)
    g.setColour (juce::Colours::black.withAlpha (0.12f + 0.16f * rua));
    for (int y = 0; y < (int) h; y += 3)
        g.fillRect (0.0f, (float) y, w, 1.0f);

    // vinheta
    juce::ColourGradient vig (juce::Colours::transparentBlack, bounds.getCentre(),
                              juce::Colours::black.withAlpha (0.5f), bounds.getBottomLeft(), true);
    g.setGradientFill (vig);
    g.fillRect (bounds);
}

void MaisRuaAudioProcessorEditor::drawKnob (juce::Graphics& g, juce::Point<float> centre, juce::Colour accent)
{
    const float knobR = 58.0f;
    const float ringR = knobR + 14.0f;
    const float valA  = kStartAngle + rua * (kEndAngle - kStartAngle);

    // trilho
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringR, ringR, 0.0f, kStartAngle, kEndAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // arco de valor
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, ringR, ringR, 0.0f, kStartAngle, valA, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corpo do knob
    juce::ColourGradient body (juce::Colour (0xff17171b), centre.x, centre.y - knobR,
                               juce::Colour (0xff0c0c0f), centre.x, centre.y + knobR, false);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (accent.withAlpha (0.5f));
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);

    // ponteiro
    const float pr = knobR - 10.0f;
    juce::Point<float> tip  (centre.x + std::sin (valA) * pr,   centre.y - std::cos (valA) * pr);
    juce::Point<float> tail (centre.x + std::sin (valA) * 14.0f, centre.y - std::cos (valA) * 14.0f);
    g.setColour (accent.brighter (0.4f));
    g.drawLine ({ tail, tip }, 4.0f);
    g.setColour (accent);
    g.fillEllipse (tip.x - 4.0f, tip.y - 4.0f, 8.0f, 8.0f);
}

void MaisRuaAudioProcessorEditor::resized()
{
    ruaKnob.setBounds (getLocalBounds().withSizeKeepingCentre (150, 150).translated (0, 12));
    modoBox.setBounds (getWidth() - 120, 8, 110, 22);
}
