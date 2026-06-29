#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <ui/ManifestEditor.h>   // reusable editor (engine)
#include "PluginProcessor.h"

/** Thin wrapper: hosts the engine's reusable dm::ManifestEditor (the processor is
    its ManifestEditorHost). All editor logic lives in the engine. */
class Omni84AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Omni84AudioProcessorEditor (Omni84AudioProcessor& p)
        : AudioProcessorEditor (&p), editor (p)
    {
        addAndMakeVisible (editor);
        setSize (editor.preferredWidth(), editor.preferredHeight());
    }

    void resized() override { editor.setBounds (getLocalBounds()); }
    bool keyPressed (const juce::KeyPress& key) override { return editor.handleKey (key); }

private:
    dm::ManifestEditor editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessorEditor)
};
