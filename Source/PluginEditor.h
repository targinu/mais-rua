#pragma once

#include "PluginProcessor.h"
#include "Achievements.h"

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
    // painel cheio de tela com a lista de conquistas; toque em qualquer lugar fecha.
    struct AchievementsPanel : public juce::Component
    {
        explicit AchievementsPanel (Achievements::Manager& mgr) : achievements (mgr) {}
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override { setVisible (false); }

        Achievements::Manager& achievements;
    };

    // painel cheio de tela com informações do plugin; tem um botão que revela
    // a AchievementsPanel por cima. Toque fora dos botões fecha.
    struct AboutPanel : public juce::Component
    {
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

        std::function<void()> onShowAchievements;
    };

    void timerCallback() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void drawKnob (juce::Graphics&);
    void drawAchievementsIcon (juce::Graphics&, juce::Colour);
    void checkAchievements (float rawRua, float rawModo);
    void drawAchievementToast (juce::Graphics&);

    MaisRuaAudioProcessor& processor;
    InvisibleKnobLNF invisibleLnf;

    juce::Slider ruaKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ruaAttachment;

    juce::ComboBox modoBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modoAttachment;

    float rua { 0.0f };   // valor suavizado (0..1) usado pro desenho

    // --- about (botão no nome do plugin no rodapé) ---
    bool brandHovered { false };
    AboutPanel aboutPanel;

    // --- conquistas ---
    Achievements::Manager achievements;   // precisa vir antes de achievementsPanel (que guarda referência)
    AchievementsPanel achievementsPanel { achievements };
    std::unique_ptr<juce::Drawable> achievementsIconSvg;
    juce::Colour achievementsIconColour { juce::Colours::black };   // cor atualmente aplicada ao SVG (pra chamar replaceColour)
    const Achievements::Info* toastInfo { nullptr };
    double toastShownMs { 0.0 };

    float  prevRawRua { 0.0f };
    double prevRawRuaTimeMs { 0.0 };
    double zeroSinceMs { -1.0 };
    double travelAccum { 0.0 };
    double nightSinceMs { -1.0 };
    bool   potholeHit[3] { false, false, false };
    float  prevRawModo { 0.0f };
    juce::Array<double> modoSwitchTimes;
    double editorOpenedMs { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaisRuaAudioProcessorEditor)
};
