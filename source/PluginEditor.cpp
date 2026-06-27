#include "PluginEditor.h"

Omni84AudioProcessorEditor::Omni84AudioProcessorEditor (Omni84AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    titleLabel.setText ("Omni-84", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    addAndMakeVisible (titleLabel);

    versionLabel.setText (dm::SamplerEngine::getVersion(), juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centred);
    versionLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (versionLabel);

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
}
