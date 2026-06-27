#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiKeyboardComponent
#include "PluginProcessor.h"

/** M2 editor: product name, engine version, the loaded mode, and an on-screen
    keyboard so you can play the embedded samples in the Standalone. The real
    data-driven UI (background image, frame knobs, mode switcher) arrives in M4/M5.
*/
class Omni84AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Omni84AudioProcessorEditor (Omni84AudioProcessor&);
    ~Omni84AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Omni84AudioProcessor& processorRef;
    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessorEditor)
};
