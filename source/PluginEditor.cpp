#include "PluginEditor.h"

Omni84AudioProcessorEditor::Omni84AudioProcessorEditor (Omni84AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    titleLabel.setText ("Omni-84", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    addAndMakeVisible (titleLabel);

    const auto mode = processorRef.getLoadedModeName();
    versionLabel.setText (dm::SamplerEngine::getVersion()
                              + (mode.isNotEmpty() ? "  —  mode: " + mode
                                                   : juce::String ("  —  no embedded assets")),
                          juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centred);
    versionLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (versionLabel);

    // Bass maps notes 24..42 — start the visible range there.
    keyboard.setAvailableRange (24, 60);
    keyboard.setLowestVisibleKey (24);
    addAndMakeVisible (keyboard);

    // --- Temporary M3 dev FX controls (effect[0]=lowpass, effect[1]=reverb) ---
    fxLabel.setText ("FX (dev controls — M4 replaces these)", juce::dontSendNotification);
    fxLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::italic)));
    fxLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (fxLabel);

    lowpassEnable.onClick = [this]
    {
        processorRef.getEngine().setEffectEnabled (0, lowpassEnable.getToggleState());
    };
    addAndMakeVisible (lowpassEnable);

    lowpassFreq.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lowpassFreq.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    lowpassFreq.setRange (150.0, 15000.0);
    lowpassFreq.setSkewFactorFromMidPoint (2000.0);
    lowpassFreq.setValue (15000.0, juce::dontSendNotification);
    lowpassFreq.onValueChange = [this]
    {
        processorRef.getEngine().setEffectParam (0, "FX_FILTER_FREQUENCY", (float) lowpassFreq.getValue());
    };
    addAndMakeVisible (lowpassFreq);
    lowpassFreqLabel.setText ("Lowpass Hz", juce::dontSendNotification);
    lowpassFreqLabel.setJustificationType (juce::Justification::centred);
    lowpassFreqLabel.attachToComponent (&lowpassFreq, false);

    reverbMix.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    reverbMix.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    reverbMix.setRange (0.0, 1.0);
    reverbMix.setValue (0.0, juce::dontSendNotification);
    reverbMix.onValueChange = [this]
    {
        processorRef.getEngine().setEffectParam (1, "FX_MIX", (float) reverbMix.getValue());
    };
    addAndMakeVisible (reverbMix);
    reverbMixLabel.setText ("Reverb mix", juce::dontSendNotification);
    reverbMixLabel.setJustificationType (juce::Justification::centred);
    reverbMixLabel.attachToComponent (&reverbMix, false);

    reverbGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    reverbGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    reverbGain.setRange (-24.0, 24.0, 0.1);
    reverbGain.setValue (0.0, juce::dontSendNotification);
    reverbGain.onValueChange = [this]
    {
        processorRef.getEngine().setEffectParam (1, "FX_OUTPUT_LEVEL", (float) reverbGain.getValue());
    };
    addAndMakeVisible (reverbGain);
    reverbGainLabel.setText ("Reverb dB", juce::dontSendNotification);
    reverbGainLabel.setJustificationType (juce::Justification::centred);
    reverbGainLabel.attachToComponent (&reverbGain, false);

    setSize (812, 375); // matches the DecentSampler UI size; real skin comes in M4.
}

Omni84AudioProcessorEditor::~Omni84AudioProcessorEditor() = default;

void Omni84AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff222222));
}

void Omni84AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (44));
    versionLabel.setBounds (area.removeFromTop (24));
    keyboard.setBounds (area.removeFromBottom (90));

    area.removeFromTop (8);
    fxLabel.setBounds (area.removeFromTop (18));
    area.removeFromTop (18); // room for the attached knob labels
    auto row = area.removeFromTop (100);
    lowpassEnable.setBounds (row.removeFromLeft (120).withSizeKeepingCentre (110, 28));
    lowpassFreq.setBounds (row.removeFromLeft (100));
    reverbMix.setBounds (row.removeFromLeft (100));
    reverbGain.setBounds (row.removeFromLeft (100));
}
