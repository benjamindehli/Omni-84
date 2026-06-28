#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>   // MidiKeyboardComponent
#include <ui/ManifestUiComponent.h>               // data-driven renderer (engine)
#include "PluginProcessor.h"
#include <memory>

/** M4 editor: renders the active mode's data-driven UI (background, filmstrip
    knobs, image buttons, lights) from the manifest, plus a mode selector, the
    chord-ordering selector (AutoStrum), and an on-screen keyboard.
*/
class Omni84AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Omni84AudioProcessorEditor (Omni84AudioProcessor&);
    ~Omni84AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void rebuildUi();                                       // build renderer for active mode
    void applyControl (const dm::Control&, double value);   // binding → engine
    void applyButton (const dm::Button&, int stateIndex);

    Omni84AudioProcessor& processorRef;

    juce::Label    versionLabel, modeLabel;
    juce::ComboBox modeSelector;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<dm::ManifestUiComponent> uiComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessorEditor)
};
