#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiKeyboardComponent
#include "PluginProcessor.h"

/** M5 editor: product name, engine version, a mode selector across the library's
    modes, temporary dev FX controls, and an on-screen keyboard. The real
    data-driven UI (background image, frame knobs) arrives in M4.
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
    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    juce::MidiKeyboardComponent keyboard;

    // Temporary M3 dev controls for the Bass FX (effect[0]=lowpass, [1]=reverb).
    // Replaced by the data-driven UI + APVTS bindings in M4.
    juce::Label        fxLabel;
    juce::ToggleButton lowpassEnable { "Lowpass on" };
    juce::Slider       lowpassFreq;
    juce::Slider       reverbMix;
    juce::Slider       reverbGain;
    juce::Label        lowpassFreqLabel, reverbMixLabel, reverbGainLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessorEditor)
};
