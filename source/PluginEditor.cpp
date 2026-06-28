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
        // Drive the Mode param; the processor performs the actual (deferred) switch
        // and calls back onModeChanged → rebuildUi().
        setParam (omni84::params::id::mode, (float) (modeSelector.getSelectedId() - 1));
    };
    addAndMakeVisible (modeSelector);

    keyboard.setAvailableRange (24, 119);
    keyboard.setLowestVisibleKey (lowestVisibleKey);
    keyboard.setKeyPressBaseOctave (keyOctave);   // seed to match keyOctave so the first Z/X steps cleanly
    addAndMakeVisible (keyboard);

    // The processor switches the mode (on the message thread) then asks us to rebuild.
    processorRef.onModeChanged = [this]
    {
        modeSelector.setSelectedId (processorRef.getActiveModeIndex() + 1, juce::dontSendNotification);
        rebuildUi();
    };

    rebuildUi();
    setSize (812, 403); // dev top strip + the 812×375 face (keyboard overlays it)
    startTimerHz (30);  // reflect host automation / preset recall into the widgets
}

Omni84AudioProcessorEditor::~Omni84AudioProcessorEditor()
{
    stopTimer();
    processorRef.onModeChanged = nullptr;
}

void Omni84AudioProcessorEditor::rebuildUi()
{
    uiComponent.reset();

    auto* ui = processorRef.getActiveModeUi();
    if (ui == nullptr)
        return;

    uiComponent = std::make_unique<dm::ManifestUiComponent> (
        *ui, [] (const juce::String& id) { return loadImageById (id); });

    // Widgets drive the params (single source of truth). One knob may map to two
    // params (e.g. Decay+Release); each gets the same normalised position.
    uiComponent->onControlChanged = [this] (const dm::Control& c, double native)
    {
        const double mn = c.min.value_or (0.0), mx = c.max.value_or (1.0);
        const float norm = (float) (mx > mn ? juce::jlimit (0.0, 1.0, (native - mn) / (mx - mn)) : 0.0);
        for (const auto& pid : omni84::params::controlParamIds (c))
            setParam (pid.toRawUTF8(), norm);
    };
    uiComponent->onButtonChanged = [this] (const dm::Button& b, int stateIndex)
    {
        const auto pid = omni84::params::buttonParamId (b);
        if (pid.isNotEmpty())
            setParam (pid.toRawUTF8(), stateIndex >= 1 ? 1.0f : 0.0f);
    };
    uiComponent->onMenuChanged = [this] (const dm::Menu&, int idx)
    {
        setParam (omni84::params::id::chordOrder, (float) idx);
    };
    addAndMakeVisible (*uiComponent);
    uiComponent->toBack();   // background + widgets behind the keyboard / selector

    refreshWidgets();        // show the current param values
    resized();
}

void Omni84AudioProcessorEditor::setParam (const char* paramId, float nativeValue)
{
    if (auto* p = processorRef.getApvts().getParameter (paramId))
        p->setValueNotifyingHost (p->convertTo0to1 (nativeValue));
}

void Omni84AudioProcessorEditor::refreshWidgets()
{
    if (uiComponent == nullptr)
        return;

    auto& apvts = processorRef.getApvts();
    auto raw = [&apvts] (const juce::String& id) -> float
    {
        if (auto* a = apvts.getRawParameterValue (id)) return a->load();
        return 0.0f;
    };

    uiComponent->refresh (
        [&] (const dm::Control& c) -> std::optional<double>
        {
            const auto ids = omni84::params::controlParamIds (c);
            if (ids.isEmpty()) return std::nullopt;
            const double mn = c.min.value_or (0.0), mx = c.max.value_or (1.0);
            return mn + (double) raw (ids[0]) * (mx - mn);
        },
        [&] (const dm::Button& b) -> std::optional<int>
        {
            const auto pid = omni84::params::buttonParamId (b);
            if (pid.isEmpty()) return std::nullopt;
            return raw (pid) > 0.5f ? 1 : 0;
        },
        [&] (const dm::Menu&) -> std::optional<int>
        {
            return (int) raw (omni84::params::id::chordOrder);
        });
}

void Omni84AudioProcessorEditor::timerCallback()
{
    refreshWidgets();
}

bool Omni84AudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    const auto c = key.getTextCharacter();
    if (c == 'z' || c == 'Z') { shiftKeyboardOctave (-1); return true; }
    if (c == 'x' || c == 'X') { shiftKeyboardOctave (+1); return true; }
    return false;
}

void Omni84AudioProcessorEditor::shiftKeyboardOctave (int deltaOctaves)
{
    // Track play octave + visible key ourselves and shift both by the same delta.
    // (getLowestVisibleKey() returns a value clamped to what currently fits, and the
    // computer-key octave starts at JUCE's default 6 — deriving one from the other
    // desyncs the first press, so keep them as independent tracked state.)
    keyOctave = juce::jlimit (1, 9, keyOctave + deltaOctaves);
    keyboard.setKeyPressBaseOctave (keyOctave);

    lowestVisibleKey = juce::jlimit (24, 96, lowestVisibleKey + deltaOctaves * 12);
    keyboard.setLowestVisibleKey (lowestVisibleKey);
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
