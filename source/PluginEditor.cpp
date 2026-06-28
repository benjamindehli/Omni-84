#include "PluginEditor.h"

#if OMNI84_HAS_ASSETS
 #include <BinaryData.h>
#endif

namespace
{
// Load an embedded image by manifest id ("img:Knob" → the "Knob.png" resource).
juce::Image loadImageById (const juce::String& id)
{
   #if OMNI84_HAS_ASSETS
    const auto stem = id.fromLastOccurrenceOf (":", false, false);
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String fn (BinaryData::originalFilenames[i]);
        const bool isImage = fn.endsWithIgnoreCase (".png") || fn.endsWithIgnoreCase (".jpg")
                          || fn.endsWithIgnoreCase (".jpeg");
        if (isImage && fn.upToLastOccurrenceOf (".", false, false) == stem)
        {
            int size = 0;
            if (const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                return juce::ImageFileFormat::loadFrom (data, (size_t) size);
        }
    }
   #endif
    juce::ignoreUnused (id);
    return {};
}
}

Omni84AudioProcessorEditor::Omni84AudioProcessorEditor (Omni84AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    versionLabel.setText (dm::SamplerEngine::getVersion(), juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredRight);
    versionLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (versionLabel);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    addAndMakeVisible (modeLabel);

    const auto modeNames = processorRef.getModeNames();
    for (int i = 0; i < modeNames.size(); ++i)
        modeSelector.addItem (modeNames[i], i + 1);
    if (! modeNames.isEmpty())
        modeSelector.setSelectedId (processorRef.getActiveModeIndex() + 1, juce::dontSendNotification);
    modeSelector.setTextWhenNoChoicesAvailable ("(no embedded assets)");
    modeSelector.onChange = [this]
    {
        processorRef.setActiveMode (modeSelector.getSelectedId() - 1);
        rebuildUi();
    };
    addAndMakeVisible (modeSelector);

    keyboard.setAvailableRange (24, 119);
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    rebuildUi();
    setSize (812, 403); // dev top strip + the 812×375 face (keyboard overlays it)
}

Omni84AudioProcessorEditor::~Omni84AudioProcessorEditor() = default;

void Omni84AudioProcessorEditor::rebuildUi()
{
    uiComponent.reset();

    auto* ui = processorRef.getActiveModeUi();
    if (ui == nullptr)
        return;

    uiComponent = std::make_unique<dm::ManifestUiComponent> (
        *ui, [] (const juce::String& id) { return loadImageById (id); });

    uiComponent->onControlChanged = [this] (const dm::Control& c, double v) { applyControl (c, v); };
    uiComponent->onButtonChanged  = [this] (const dm::Button& b, int s)      { applyButton (b, s); };
    uiComponent->onMenuChanged    = [this] (const dm::Menu& m, int idx)
    {
        if (idx >= 0 && idx < m.options.size())
            processorRef.getEngine().setSequencerIndexOffset (m.options.getReference (idx).seqIndex);
    };
    addAndMakeVisible (*uiComponent);
    uiComponent->toBack();   // background + widgets behind the keyboard / selector

    // Sync the engine to the freshly-shown defaults (supported params + menu).
    if (! ui->tabs.isEmpty())
    {
        const auto& tab = ui->tabs.getReference (0);
        for (const auto& c : tab.controls)
            applyControl (c, c.value.value_or (c.min.value_or (0.0)));
        for (const auto& b : tab.buttons)
            applyButton (b, b.value.value_or (0));   // sync engine to default button state
        for (const auto& m : tab.menus)
        {
            const int idx = juce::jlimit (0, juce::jmax (0, m.options.size() - 1), m.value - 1);
            if (idx < m.options.size())
                processorRef.getEngine().setSequencerIndexOffset (m.options.getReference (idx).seqIndex);
        }
    }

    resized();
}

void Omni84AudioProcessorEditor::applyControl (const dm::Control& c, double value)
{
    auto& eng = processorRef.getEngine();
    for (const auto& b : c.bindings)
    {
        const double f = b.factor.value_or (1.0);
        const auto& p = b.parameter;
        if      (p == "FX_FILTER_FREQUENCY") eng.setLowpassFrequency ((float) (value * f));
        else if (p == "FX_MIX")              eng.setReverbMix ((float) (value * f));
        else if (p == "FX_OUTPUT_LEVEL")     eng.setReverbWetGainDb ((float) (value * f));
        else if (p == "SEQ_PLAYBACK_RATE")   eng.setSequencerRate (value * f);
        // ENV_*/AMP_*/etc. become automatable parameters in M4c.
    }
}

void Omni84AudioProcessorEditor::applyButton (const dm::Button& b, int stateIndex)
{
    if (stateIndex < 0 || stateIndex >= b.states.size())
        return;

    const auto* mode = processorRef.getActiveMode();
    for (const auto& bind : b.states.getReference (stateIndex).bindings)
    {
        if (bind.parameter == "ENABLED" && bind.effectIndex && mode != nullptr)
        {
            const int ei = *bind.effectIndex;
            const bool on = bind.translationValue.isBool()
                              ? (bool) bind.translationValue
                              : bind.translationValue.toString().equalsIgnoreCase ("true");
            if (ei >= 0 && ei < mode->effects.size()
                && mode->effects.getReference (ei).type == "lowpass")
                processorRef.getEngine().setLowpassEnabled (on);
        }
    }
}

void Omni84AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void Omni84AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto top = area.removeFromTop (28);
    modeLabel.setBounds (top.removeFromLeft (46));
    modeSelector.setBounds (top.removeFromLeft (220));
    versionLabel.setBounds (top.removeFromRight (190));

    // The data-driven face fills the rest; the keyboard overlays its bottom like
    // DecentSampler — 20 px from bottom/right, ~60 px on the left for the wheels.
    if (uiComponent != nullptr)
        uiComponent->setBounds (area);

    const int kbHeight = 100, margin = 10, leftGap = 40; // ~55% taller; wheels go in leftGap
    keyboard.setBounds (area.getX() + leftGap,
                        area.getBottom() - margin - kbHeight,
                        area.getWidth() - leftGap - margin,
                        kbHeight);
}
