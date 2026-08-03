#include "PluginEditor.h"
#include <cmath>

// Layout inteiro desenhado pra uma tela fixa de 360x440 (mesmo tamanho da
// janela). Espelha 1:1 o conceito validado em mais-rua-ui-concept.html —
// mesmas coordenadas, thresholds e cores.
namespace
{
    constexpr float kW = 360.0f, kH = 440.0f;
    constexpr float kGroundY = 380.0f, kGroundH = 24.0f;
    constexpr float kMargin = 18.0f;
    constexpr int   kSlots = 12;
    constexpr float kSlotW = (kW - kMargin * 2.0f) / (float) kSlots;

    constexpr float kKnobCx = 180.0f, kKnobCy = 196.0f, kKnobR = 82.0f, kArcR = kKnobR + 16.0f;
    const float kStartAngle = juce::MathConstants<float>::pi * -0.75f; // -135°, "fechado"
    const float kEndAngle   = juce::MathConstants<float>::pi *  0.75f; //  135°, "aberto"

    struct BuildingSpec { float h, w; bool roof; };
    const BuildingSpec kBuildings[kSlots] = {
        {  90, 24, false }, { 130, 20, false }, {  60, 26, true  }, { 150, 18, false },
        {  80, 24, true  }, { 110, 22, false }, {  50, 28, true  }, { 140, 20, false },
        {  70, 24, true  }, { 100, 26, false }, { 160, 18, false }, {  85, 24, false }
    };

    // em que valor (0-100) cada prédio some, espalhado pela largura pra
    // sumir de forma uniforme. Slots 0 e 9 ficam sempre vazios de propósito
    // — é onde os carros estacionam.
    const float kThreshold[kSlots] = { 0, 29, 54, 6, 87, 37, 62, 13, 79, 0, 70, 20 };

    constexpr float slotX (int i) { return kMargin + kSlotW * ((float) i + 0.5f); }

    struct CarSpec { float x; };
    const CarSpec kCars[2] = { { slotX (0) }, { slotX (9) } };

    struct HoleSpec { float x, at; };
    const HoleSpec kPotholes[3] = { { 100.0f, 35.0f }, { 158.0f, 60.0f }, { 246.0f, 85.0f } };

    constexpr float kHouseX = 196.0f, kHouseW = 36.0f, kHouseH = 46.0f;
    constexpr float kLampX = 344.0f, kLampTop = 286.0f;

    const juce::Colour kColStreet (0xffE4572E);
    const juce::Colour kColInk    (0xff1E1A20);
    const juce::Colour kColCream  (0xffF6EFDD);
    const juce::Colour kColSignal (0xffF2A93B);

    juce::Point<float> pointOnCircle (juce::Point<float> c, float r, float angleRad)
    {
        return { c.x + r * std::sin (angleRad), c.y - r * std::cos (angleRad) };
    }

    void drawBuilding (juce::Graphics& g, const BuildingSpec& b, float cx, juce::Colour fill)
    {
        const float x = cx - b.w * 0.5f;
        const float top = kGroundY - b.h;
        g.setColour (fill);
        g.fillRect (juce::Rectangle<float> (x, top, b.w, b.h));

        if (b.roof)
        {
            const float roofH = b.w * 0.62f;
            juce::Path roof;
            roof.startNewSubPath (x, top);
            roof.lineTo (cx, top - roofH);
            roof.lineTo (x + b.w, top);
            roof.closeSubPath();
            g.fillPath (roof);
        }
    }

    void drawCar (juce::Graphics& g, float carX, juce::Colour figureColour, juce::Colour skyColour)
    {
        const float bodyW = 32.0f, bodyH = 12.0f, cabinW = 16.0f, cabinH = 9.0f, wheel = 6.0f;
        const float bx = carX - bodyW * 0.5f, by = kGroundY - bodyH;
        const float cx = carX - cabinW * 0.5f, cy = by - cabinH;

        g.setColour (figureColour);
        g.fillRect (juce::Rectangle<float> (cx, cy, cabinW, cabinH));
        g.fillRect (juce::Rectangle<float> (bx, by, bodyW, bodyH));

        g.setColour (skyColour);
        g.fillRect (juce::Rectangle<float> (bx + 5.0f, kGroundY - wheel * 0.5f, wheel, wheel));
        g.fillRect (juce::Rectangle<float> (bx + bodyW - 5.0f - wheel, kGroundY - wheel * 0.5f, wheel, wheel));
    }

    void drawHouse (juce::Graphics& g, juce::Colour figureColour, juce::Colour skyColour, float opacity)
    {
        if (opacity <= 0.02f)
            return;

        const float top = kGroundY - kHouseH;
        const float x = kHouseX - kHouseW * 0.5f;
        const float roofH = kHouseW * 0.58f;
        const float winSize = 8.0f;

        g.setColour (figureColour.withAlpha (opacity));
        juce::Path roof;
        roof.startNewSubPath (x, top);
        roof.lineTo (kHouseX, top - roofH);
        roof.lineTo (x + kHouseW, top);
        roof.closeSubPath();
        g.fillPath (roof);
        g.fillRect (juce::Rectangle<float> (x, top, kHouseW, kHouseH));

        g.setColour (skyColour.withAlpha (opacity));
        g.fillRect (juce::Rectangle<float> (x + 7.0f, top + 10.0f, winSize, winSize));

        // pichação só na casinha
        const float tx = kHouseX + 6.0f, ty = top + kHouseH - 14.0f;
        juce::Path tag;
        tag.startNewSubPath (tx - 8.0f, ty + 6.0f);
        tag.lineTo (tx - 3.0f, ty - 7.0f);
        tag.lineTo (tx,        ty + 1.0f);
        tag.lineTo (tx + 7.0f, ty - 8.0f);
        g.setColour (skyColour.withAlpha (opacity));
        g.strokePath (tag, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawPothole (juce::Graphics& g, const HoleSpec& hole, float value, juce::Colour skyColour)
    {
        const float opacity = juce::jlimit (0.0f, 1.0f, (value - hole.at) / 8.0f);
        if (opacity <= 0.02f)
            return;
        const float rx = 7.0f, ry = 3.6f;
        g.setColour (skyColour.withAlpha (opacity));
        g.fillEllipse (hole.x - rx, kGroundY + kGroundH * 0.55f - ry, rx * 2.0f, ry * 2.0f);
    }

    void drawLamp (juce::Graphics& g, juce::Colour figureColour, float glowT)
    {
        const float poleW = 4.0f;
        g.setColour (figureColour);
        g.fillRect (juce::Rectangle<float> (kLampX - poleW * 0.5f, kLampTop, poleW, kGroundY - kLampTop));

        if (glowT > 0.02f)
        {
            const float glowR = 26.0f * (0.4f + 0.6f * glowT);
            g.setColour (kColSignal.withAlpha (glowT * 0.4f));
            g.fillEllipse (kLampX - glowR, kLampTop - glowR, glowR * 2.0f, glowR * 2.0f);
        }

        g.setColour (kColSignal);
        g.fillRect (juce::Rectangle<float> (kLampX - 6.0f, kLampTop - 6.0f, 12.0f, 12.0f));
    }
}

MaisRuaAudioProcessorEditor::MaisRuaAudioProcessorEditor (MaisRuaAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    ruaKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    // setRotaryParameters exige ângulos >= 0 (diferente de Path::addCentredArc,
    // usado no resto do arquivo) — mesmo arco de kStartAngle/kEndAngle, só
    // deslocado +2π pra ficar no domínio que a API aceita.
    ruaKnob.setRotaryParameters (kStartAngle + juce::MathConstants<float>::twoPi,
                                  kEndAngle + juce::MathConstants<float>::twoPi, true);
    ruaKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    ruaKnob.setLookAndFeel (&invisibleLnf);
    addAndMakeVisible (ruaKnob);

    ruaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "rua", ruaKnob);

    modoBox.addItem ("Classico", 1);
    modoBox.addItem ("Grao", 2);
    modoBox.setJustificationType (juce::Justification::centred);
    modoBox.setColour (juce::ComboBox::backgroundColourId, kColInk);
    modoBox.setColour (juce::ComboBox::textColourId, kColCream);
    modoBox.setColour (juce::ComboBox::outlineColourId, kColCream.withAlpha (0.25f));
    modoBox.setColour (juce::ComboBox::arrowColourId, kColCream);
    addAndMakeVisible (modoBox);

    modoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "modo", modoBox);

    setSize ((int) kW, (int) kH);
    startTimerHz (30);
}

MaisRuaAudioProcessorEditor::~MaisRuaAudioProcessorEditor()
{
    ruaKnob.setLookAndFeel (nullptr);
    stopTimer();
}

void MaisRuaAudioProcessorEditor::timerCallback()
{
    const float target = processor.getRuaValue();
    rua += (target - rua) * 0.25f;
    repaint();
}

void MaisRuaAudioProcessorEditor::drawKnob (juce::Graphics& g)
{
    const juce::Point<float> centre (kKnobCx, kKnobCy);
    const float angle = kStartAngle + (kEndAngle - kStartAngle) * rua;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, kArcR, kArcR, 0.0f, kStartAngle, kEndAngle, true);
    g.setColour (kColInk.withAlpha (0.18f));
    g.strokePath (track, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (rua > 0.0f)
    {
        juce::Path fillArc;
        fillArc.addCentredArc (centre.x, centre.y, kArcR, kArcR, 0.0f, kStartAngle, angle, true);
        g.setColour (kColSignal);
        g.strokePath (fillArc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (kColCream);
    g.fillEllipse (centre.x - kKnobR, centre.y - kKnobR, kKnobR * 2.0f, kKnobR * 2.0f);
    g.setColour (kColInk);
    g.drawEllipse (centre.x - kKnobR, centre.y - kKnobR, kKnobR * 2.0f, kKnobR * 2.0f, 7.0f);

    const auto tip  = pointOnCircle (centre, kKnobR - 14.0f, angle);
    const auto tail = pointOnCircle (centre, 16.0f, angle);
    g.setColour (kColInk);
    g.drawLine (juce::Line<float> (tail, tip), 6.0f);
}

void MaisRuaAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float value = rua * 100.0f;

    // últimos 20%: vira noite. Fundo e chão trocam de papel entre as mesmas
    // 4 cores (nenhuma cor nova é usada).
    const float tSky    = juce::jlimit (0.0f, 1.0f, (value - 80.0f) / 20.0f);
    const float tFigure = juce::jlimit (0.0f, 1.0f, tSky * 1.8f);
    const auto skyColour    = kColStreet.interpolatedWith (kColInk, tSky);
    const auto figureColour = kColInk.interpolatedWith (kColCream, tFigure);

    g.fillAll (skyColour);

    g.setColour (figureColour);
    g.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
    g.drawText ("mais rua", juce::Rectangle<float> (0.0f, 26.0f, kW, 40.0f), juce::Justification::centred);

    for (int i = 0; i < kSlots; ++i)
        if (value < kThreshold[i])
            drawBuilding (g, kBuildings[i], slotX (i), figureColour);

    drawLamp (g, figureColour, tSky);

    g.setColour (figureColour);
    g.fillRect (juce::Rectangle<float> (0.0f, kGroundY, kW, kGroundH));

    {
        juce::Path lane;
        lane.startNewSubPath (10.0f, kGroundY + kGroundH * 0.5f);
        lane.lineTo (kW - 10.0f, kGroundY + kGroundH * 0.5f);
        juce::Path dashed;
        const float dashLengths[] = { 12.0f, 10.0f };
        juce::PathStrokeType (2.5f).createDashedStroke (dashed, lane, dashLengths, 2);
        g.setColour (skyColour.withAlpha (0.85f));
        g.fillPath (dashed);
    }

    for (auto& hole : kPotholes)
        drawPothole (g, hole, value, skyColour);

    drawHouse (g, figureColour, skyColour, tSky);

    for (auto& car : kCars)
        drawCar (g, car.x, figureColour, skyColour);

    drawKnob (g);

    // caractere construído a partir do code point (evita mojibake de fonte/encoding do compilador com "™" literal)
    const juce::String brandTitle = juce::String ("MAIS RUA") + juce::String::charToString ((juce::juce_wchar) 0x2122);
    g.setColour (figureColour);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText (brandTitle, juce::Rectangle<float> (14.0f, 402.0f, 200.0f, 16.0f), juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::plain)));
    g.setColour (figureColour.withAlpha (0.75f));
    g.drawText ("BY FROZENSHADE", juce::Rectangle<float> (14.0f, 418.0f, 200.0f, 14.0f), juce::Justification::centredLeft);
}

void MaisRuaAudioProcessorEditor::resized()
{
    ruaKnob.setBounds (juce::Rectangle<int> ((int) kKnobCx - 90, (int) kKnobCy - 90, 180, 180));
    modoBox.setBounds (236, 411, 110, 20);
}
