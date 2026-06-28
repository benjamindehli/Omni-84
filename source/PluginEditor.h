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
class Omni84AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit Omni84AudioProcessorEditor (Omni84AudioProcessor&);
    ~Omni84AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;       // Z / X shift the keyboard octave

private:
    void shiftKeyboardOctave (int deltaOctaves);
    void rebuildUi();                                       // build renderer for active mode
    void refreshWidgets();                                  // push current param values into widgets
    void setParam (const char* paramId, float nativeValue); // widget → param (notifying host)
    void timerCallback() override;

    Omni84AudioProcessor& processorRef;

    juce::Label    versionLabel, modeLabel;
    juce::ComboBox modeSelector;
    juce::MidiKeyboardComponent keyboard;
    int keyOctave { 6 };          // computer-key play octave (JUCE's default keyMappingOctave)
    int lowestVisibleKey { 36 };  // leftmost visible key; tracked (getLowestVisibleKey clamps)
    std::unique_ptr<dm::ManifestUiComponent> uiComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessorEditor)
};
