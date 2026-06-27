#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/** M0 placeholder editor: shows the product name and engine version so you can
    confirm the plugin loads and the engine is wired in. The real data-driven UI
    (background image, frame knobs, mode switcher) arrives in M4/M5.
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessorEditor)
};
