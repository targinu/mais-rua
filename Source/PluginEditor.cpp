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

    constexpr float kAchIconX = kW - 34.0f, kAchIconY = 14.0f, kAchIconSize = 20.0f;
    const juce::Rectangle<float> kAchievementsIconBounds (kAchIconX, kAchIconY, kAchIconSize, kAchIconSize);

    // ícone de conquistas: SVG "trophy" do Heroicons (MIT), cor trocada em
    // runtime via Drawable::replaceColour pra acompanhar o dia/noite da UI.
    const char* const kTrophySvgSource = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#000000">
  <path fill-rule="evenodd" d="M5.166 2.621v.858c-1.035.148-2.059.33-3.071.543a.75.75 0 0 0-.584.859 6.753 6.753 0 0 0 6.138 5.6 6.73 6.73 0 0 0 2.743 1.346A6.707 6.707 0 0 1 9.279 15H8.54c-1.036 0-1.875.84-1.875 1.875V19.5h-.75a2.25 2.25 0 0 0-2.25 2.25c0 .414.336.75.75.75h15a.75.75 0 0 0 .75-.75 2.25 2.25 0 0 0-2.25-2.25h-.75v-2.625c0-1.036-.84-1.875-1.875-1.875h-.739a6.706 6.706 0 0 1-1.112-3.173 6.73 6.73 0 0 0 2.743-1.347 6.753 6.753 0 0 0 6.139-5.6.75.75 0 0 0-.585-.858 47.077 47.077 0 0 0-3.07-.543V2.62a.75.75 0 0 0-.658-.744 49.22 49.22 0 0 0-6.093-.377c-2.063 0-4.096.128-6.093.377a.75.75 0 0 0-.657.744Zm0 2.629c0 1.196.312 2.32.857 3.294A5.266 5.266 0 0 1 3.16 5.337a45.6 45.6 0 0 1 2.006-.343v.256Zm13.5 0v-.256c.674.1 1.343.214 2.006.343a5.265 5.265 0 0 1-2.863 3.207 6.72 6.72 0 0 0 .857-3.294Z" clip-rule="evenodd"/>
</svg>)svg";

    // área do rodapé "MAIS RUA BY FROZENSHADE" — texto, hover e botão usam o mesmo retângulo.
    const juce::Rectangle<float> kBrandBounds (14.0f, 411.0f, 210.0f, 20.0f);

    // layout da AboutPanel.
    const juce::Rectangle<float> kAboutUpdateButtonBounds       ((kW - 240.0f) * 0.5f, 168.0f, 240.0f, 32.0f);
    const juce::Rectangle<float> kAboutAchievementsButtonBounds ((kW - 240.0f) * 0.5f, 212.0f, 240.0f, 32.0f);

    // layout da AchievementsPanel. Cada linha tem duas alturas: nome +
    // requisito pra desbloquear (embaixo, sempre visível, trancada ou não).
    const juce::Rectangle<float> kAchListCloseBounds (kW - 34.0f, 12.0f, 22.0f, 22.0f);
    constexpr float kAchRowsStartY = 62.0f, kAchRowH = 29.0f;

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

    addChildComponent (aboutPanel);
    aboutPanel.setVisible (false);
    aboutPanel.onShowAchievements = [this]
    {
        aboutPanel.setVisible (false);
        achievementsPanel.setVisible (true);
        achievementsPanel.toFront (false);
    };

    addChildComponent (achievementsPanel);
    achievementsPanel.setVisible (false);

    if (auto xml = juce::XmlDocument::parse (kTrophySvgSource))
        achievementsIconSvg = juce::Drawable::createFromSVG (*xml);

    setSize ((int) kW, (int) kH);
    startTimerHz (30);

    editorOpenedMs   = juce::Time::getMillisecondCounterHiRes();
    prevRawRua       = processor.getRuaValue();
    prevRawRuaTimeMs = editorOpenedMs;
    prevRawModo      = processor.getModoValue();
    achievements.unlock (Achievements::Id::bemVindoRua);
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
    checkAchievements (target, processor.getModoValue());
    repaint();
}

namespace
{
    constexpr double kToastDurationMs = 3200.0;
    constexpr double kToastSlideMs    = 300.0;
    constexpr double kToastFadeMs     = 400.0;
}

void MaisRuaAudioProcessorEditor::checkAchievements (float rawRua, float rawModo)
{
    using namespace Achievements;

    const double now = juce::Time::getMillisecondCounterHiRes();
    const float  valuePct = rawRua * 100.0f;

    if (rawRua >= 0.999f)
        achievements.unlock (Id::semFreio);

    if (valuePct >= 87.0f)
        achievements.unlock (Id::cidadeFantasma);

    if (valuePct >= 80.0f)
        achievements.unlock (Id::modoNoturno);

    if (valuePct >= 90.0f)
    {
        if (nightSinceMs < 0.0)
            nightSinceMs = now;
        else if (now - nightSinceMs >= 5000.0)
            achievements.unlock (Id::pichacao);
    }
    else
    {
        nightSinceMs = -1.0;
    }

    if (valuePct >= 35.0f) potholeHit[0] = true;
    if (valuePct >= 60.0f) potholeHit[1] = true;
    if (valuePct >= 85.0f) potholeHit[2] = true;
    if (potholeHit[0] && potholeHit[1] && potholeHit[2])
        achievements.unlock (Id::buracoNaPista);

    const double dtJump = now - prevRawRuaTimeMs;
    if (std::abs (rawRua - prevRawRua) >= 0.9f && dtJump <= 1000.0)
        achievements.unlock (Id::voltouProChao);

    travelAccum += std::abs (rawRua - prevRawRua);
    if (travelAccum >= 12.0)
        achievements.unlock (Id::rodouQuarteirao);

    prevRawRua = rawRua;
    prevRawRuaTimeMs = now;

    if (rawRua <= 0.0005f)
    {
        if (zeroSinceMs < 0.0)
            zeroSinceMs = now;
        else if (now - zeroSinceMs >= 180000.0)
            achievements.unlock (Id::vizinhancaTranquila);
    }
    else
    {
        zeroSinceMs = -1.0;
    }

    if (rawModo != prevRawModo)
    {
        if (rawModo >= 0.5f)
            achievements.unlock (Id::graoFino);

        modoSwitchTimes.add (now);
        while (modoSwitchTimes.size() > 0 && now - modoSwitchTimes.getFirst() > 10000.0)
            modoSwitchTimes.remove (0);
        if (modoSwitchTimes.size() >= 5)
            achievements.unlock (Id::indeciso);

        prevRawModo = rawModo;
    }

    if (now - editorOpenedMs >= 600000.0)
        achievements.unlock (Id::sessaoLonga);

    if (toastInfo == nullptr)
    {
        if (auto* next = achievements.popNextNotification())
        {
            toastInfo = next;
            toastShownMs = now;
        }
    }
    else if (now - toastShownMs >= kToastDurationMs)
    {
        toastInfo = nullptr;
    }
}

void MaisRuaAudioProcessorEditor::drawAchievementToast (juce::Graphics& g)
{
    if (toastInfo == nullptr)
        return;

    const double elapsed = juce::Time::getMillisecondCounterHiRes() - toastShownMs;
    const float  slideIn = (float) juce::jlimit (0.0, 1.0, elapsed / kToastSlideMs);
    float fadeOut = 1.0f;
    if (elapsed > kToastDurationMs - kToastFadeMs)
        fadeOut = (float) juce::jlimit (0.0, 1.0, (kToastDurationMs - elapsed) / kToastFadeMs);
    const float alpha = juce::jmin (slideIn, fadeOut);

    const float barW = kW - 32.0f, barH = 54.0f;
    const float y = juce::jmap (slideIn, 0.0f, 1.0f, -barH, 14.0f);
    const juce::Rectangle<float> bar (16.0f, y, barW, barH);

    g.setColour (kColInk.withAlpha (0.92f * alpha));
    g.fillRoundedRectangle (bar, 10.0f);

    auto textArea = bar.reduced (14.0f, 6.0f);
    g.setColour (kColSignal.withAlpha (alpha));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("CONQUISTA DESBLOQUEADA", textArea.removeFromTop (16.0f), juce::Justification::centredLeft);

    g.setColour (kColCream.withAlpha (alpha));
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawText (toastInfo->title, textArea.removeFromTop (20.0f), juce::Justification::centredLeft);

    g.setColour (kColCream.withAlpha (alpha * 0.75f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::plain)));
    g.drawText (toastInfo->description, textArea, juce::Justification::centredLeft);
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

void MaisRuaAudioProcessorEditor::drawAchievementsIcon (juce::Graphics& g, juce::Colour colour)
{
    if (achievementsIconSvg == nullptr)
        return;

    if (colour != achievementsIconColour)
    {
        achievementsIconSvg->replaceColour (achievementsIconColour, colour);
        achievementsIconColour = colour;
    }

    achievementsIconSvg->drawWithin (g, kAchievementsIconBounds, juce::RectanglePlacement::centred, 1.0f);
}

void MaisRuaAudioProcessorEditor::AboutPanel::paint (juce::Graphics& g)
{
    g.fillAll (kColInk.withAlpha (0.96f));

    g.setColour (kColCream);
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("SOBRE", juce::Rectangle<float> (0.0f, 18.0f, kW, 28.0f), juce::Justification::centred);

    g.setColour (kColCream.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
    g.drawText ("MAIS RUA \xC2\xB7 v" MAISRUA_VERSION, juce::Rectangle<float> (0.0f, 46.0f, kW, 16.0f), juce::Justification::centred);

    g.setColour (kColCream.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::plain)));
    g.drawFittedText ("Plugin de efeito de um knob so: drive e bitcrush oversampled 2x, com "
                       "modo de saturacao selecionavel. Sem frescura, so mais rua.",
                       juce::Rectangle<int> (32, 74, (int) kW - 64, 60),
                       juce::Justification::centredTop, 4);

    g.setColour (kColSignal);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText ("desenvolvido por targinu", juce::Rectangle<float> (0.0f, 138.0f, kW, 20.0f), juce::Justification::centred);

    g.setColour (kColSignal);
    g.fillRoundedRectangle (kAboutUpdateButtonBounds, 6.0f);
    g.setColour (kColInk);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("CHECK FOR UPDATES", kAboutUpdateButtonBounds, juce::Justification::centred);

    g.setColour (kColCream.withAlpha (0.15f));
    g.fillRoundedRectangle (kAboutAchievementsButtonBounds, 6.0f);
    g.setColour (kColCream.withAlpha (0.6f));
    g.drawRoundedRectangle (kAboutAchievementsButtonBounds, 6.0f, 1.2f);
    g.setColour (kColCream);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("VER CONQUISTAS", kAboutAchievementsButtonBounds, juce::Justification::centred);

    g.setColour (kColCream.withAlpha (0.45f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::plain)));
    g.drawText ("toque em qualquer lugar pra fechar", juce::Rectangle<float> (0.0f, kH - 22.0f, kW, 16.0f), juce::Justification::centred);
}

void MaisRuaAudioProcessorEditor::AchievementsPanel::paint (juce::Graphics& g)
{
    g.fillAll (kColInk.withAlpha (0.96f));

    g.setColour (kColCream);
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("CONQUISTAS", juce::Rectangle<float> (0.0f, 18.0f, kW, 28.0f), juce::Justification::centred);

    int unlockedCount = 0;
    for (int i = 0; i < (int) Achievements::Id::count; ++i)
        if (achievements.isUnlocked ((Achievements::Id) i))
            ++unlockedCount;

    g.setColour (kColCream.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
    g.drawText (juce::String (unlockedCount) + " / " + juce::String ((int) Achievements::Id::count),
                juce::Rectangle<float> (0.0f, 44.0f, kW, 14.0f), juce::Justification::centred);

    g.setColour (kColCream.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::plain)));
    g.drawText ("x", kAchListCloseBounds, juce::Justification::centred);

    float y = kAchRowsStartY;
    for (int i = 0; i < (int) Achievements::Id::count; ++i)
    {
        const auto& info = Achievements::getInfo ((Achievements::Id) i);
        const bool unlocked = achievements.isUnlocked ((Achievements::Id) i);

        g.setColour (unlocked ? kColSignal : kColCream.withAlpha (0.25f));
        g.fillEllipse (24.0f, y + 4.0f, 7.0f, 7.0f);

        g.setColour (unlocked ? kColCream : kColCream.withAlpha (0.4f));
        g.setFont (juce::Font (juce::FontOptions (12.5f, unlocked ? juce::Font::bold : juce::Font::plain)));
        g.drawText (unlocked ? juce::String (info.title) : juce::String ("???"),
                    juce::Rectangle<float> (40.0f, y, kW - 56.0f, 15.0f), juce::Justification::centredLeft);

        // requisito pra desbloquear: sempre visível, trancada ou não.
        g.setColour (kColCream.withAlpha (unlocked ? 0.55f : 0.4f));
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::plain)));
        g.drawFittedText (info.description,
                           juce::Rectangle<int> (40, (int) (y + 15.0f), (int) (kW - 56.0f), 13),
                           juce::Justification::centredLeft, 1, 0.75f);

        y += kAchRowH;
    }

    g.setColour (kColCream.withAlpha (0.45f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::plain)));
    g.drawText ("toque em qualquer lugar pra fechar", juce::Rectangle<float> (0.0f, kH - 20.0f, kW, 16.0f), juce::Justification::centred);
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

    // botão "about": mesma faixa vertical do dropdown de modo (y=411, h=20),
    // pra ficar alinhado com ele; horizontalmente fica à esquerda (centredLeft).
    // acende cor de destaque + sublinhado no hover, como qualquer botão.
    const auto brandFont = juce::Font (juce::FontOptions (12.0f, juce::Font::bold));
    g.setColour (brandHovered ? kColSignal : figureColour);
    g.setFont (brandFont);
    g.drawText ("MAIS RUA BY FROZENSHADE", kBrandBounds, juce::Justification::centredLeft);
    if (brandHovered)
    {
        const float textW = juce::GlyphArrangement::getStringWidth (brandFont, "MAIS RUA BY FROZENSHADE");
        g.drawLine (kBrandBounds.getX(), kBrandBounds.getBottom() - 2.0f,
                    kBrandBounds.getX() + textW, kBrandBounds.getBottom() - 2.0f, 1.4f);
    }

    drawAchievementsIcon (g, figureColour);
    drawAchievementToast (g);
}

void MaisRuaAudioProcessorEditor::resized()
{
    ruaKnob.setBounds (juce::Rectangle<int> ((int) kKnobCx - 90, (int) kKnobCy - 90, 180, 180));
    modoBox.setBounds (236, 411, 110, 20);
    aboutPanel.setBounds (getLocalBounds());
    achievementsPanel.setBounds (getLocalBounds());
}

void MaisRuaAudioProcessorEditor::mouseUp (const juce::MouseEvent& e)
{
    if (kAchievementsIconBounds.contains (e.position))
    {
        achievementsPanel.setVisible (true);
        achievementsPanel.toFront (false);
        return;
    }

    if (kBrandBounds.contains (e.position))
    {
        aboutPanel.setVisible (true);
        aboutPanel.toFront (false);
    }
}

void MaisRuaAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    const bool overBrand = kBrandBounds.contains (e.position);
    const bool overIcon  = kAchievementsIconBounds.contains (e.position);

    setMouseCursor ((overBrand || overIcon) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);

    if (overBrand != brandHovered)
    {
        brandHovered = overBrand;
        repaint();
    }
}

void MaisRuaAudioProcessorEditor::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);

    if (brandHovered)
    {
        brandHovered = false;
        repaint();
    }
}

void MaisRuaAudioProcessorEditor::AboutPanel::mouseUp (const juce::MouseEvent& e)
{
    if (kAboutUpdateButtonBounds.contains (e.position))
    {
        juce::URL ("https://github.com/targinu/mais-rua").launchInDefaultBrowser();
        return;
    }

    if (kAboutAchievementsButtonBounds.contains (e.position))
    {
        if (onShowAchievements != nullptr)
            onShowAchievements();
        return;
    }

    setVisible (false);
}
