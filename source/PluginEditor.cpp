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
}
