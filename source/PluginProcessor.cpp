#include "PluginProcessor.h"
#include "PluginEditor.h"

#if OMNI84_HAS_ASSETS
 #include <BinaryData.h>
#endif

Omni84AudioProcessor::Omni84AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    loadEmbeddedLibrary();

    // Build the parameter tree from the loaded manifest (defaults come from it),
    // then watch the Mode choice so host/state changes trigger a deferred switch.
    apvts = std::make_unique<juce::AudioProcessorValueTreeState> (
        *this, nullptr, "PARAMS", dm::params::createLayout (library));
    apvts->addParameterListener (dm::params::id::mode, this);
}

Omni84AudioProcessor::~Omni84AudioProcessor()
{
    if (apvts != nullptr)
        apvts->removeParameterListener (dm::params::id::mode, this);
}

void Omni84AudioProcessor::parameterChanged (const juce::String& paramID, float)
{
    // Only the Mode param needs deferral; the rest are polled in processBlock.
    if (paramID == dm::params::id::mode)
        triggerAsyncUpdate();
}

void Omni84AudioProcessor::handleAsyncUpdate()
{
    if (apvts == nullptr)
        return;

    const int requested = (int) apvts->getRawParameterValue (dm::params::id::mode)->load();
    if (requested != engine.getActiveModeIndex())
        engine.setActiveMode (requested);   // builds + lock-free swaps (message thread)

    if (onModeChanged)
        onModeChanged();                     // let the editor rebuild its face
}

void Omni84AudioProcessor::loadEmbeddedLibrary()
{
   #if OMNI84_HAS_ASSETS
    auto json = juce::String::fromUTF8 (BinaryData::manifest_json, BinaryData::manifest_jsonSize);
    auto parsed = dm::loadManifestFromJson (json);
    if (! parsed.ok)
    {
        DBG ("Omni84: manifest load failed: " << parsed.errors.joinIntoString ("; "));
        return;
    }
    library = parsed.library;

    sampleSource = std::make_unique<dm::EmbeddedFlacSource>();

    // The manifest is the source of truth for asset ids. Resolve every id it
    // references (sample sources + effect IRs) to its embedded FLAC by original
    // file name — "flac:Bass_0C" and "ir:Space" both map to "<stem>.flac".
    auto findResource = [] (const juce::String& filename, int& sizeOut) -> const char*
    {
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            if (filename == BinaryData::originalFilenames[i])
                return BinaryData::getNamedResource (BinaryData::namedResourceList[i], sizeOut);
        return nullptr;
    };

    juce::StringArray ids;
    for (const auto& m : library.modes)
    {
        for (const auto& g : m.groups)
            for (const auto& s : g.samples)
                ids.addIfNotAlreadyThere (s.source);
        for (const auto& e : m.effects)
            if (e.ir.isNotEmpty())
                ids.addIfNotAlreadyThere (e.ir);
    }

    for (const auto& id : ids)
    {
        const auto filename = id.fromLastOccurrenceOf (":", false, false) + ".flac";
        int size = 0;
        if (const char* data = findResource (filename, size))
            sampleSource->addFlac (id, data, (size_t) size);
        else
            DBG ("Omni84: no embedded asset for id " << id << " (" << filename << ")");
    }

    loaded = ! library.modes.isEmpty() && sampleSource->size() > 0;

    // Hand the library to the engine (stores pointers; mode is built on prepare()).
    if (loaded)
        engine.setLibrary (library, *sampleSource);
   #endif
}

void Omni84AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Builds the active mode's render unit for these audio settings.
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void Omni84AudioProcessor::releaseResources()
{
    engine.releaseResources();
}

bool Omni84AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Instrument: mono or stereo output, no input bus.
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo()
        || out == juce::AudioChannelSet::mono();
}

void Omni84AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that don't have corresponding input.
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Merge on-screen keyboard events into the incoming MIDI (Standalone play).
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // Inject the on-screen pitch/mod wheels as MIDI (same path as a hardware controller).
    if (const int pw = uiPitchWheel.exchange (-1); pw >= 0)
        midi.addEvent (juce::MidiMessage::pitchWheel (1, pw), 0);
    if (const int mw = uiModWheel.exchange (-1); mw >= 0)
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, mw), 0);

    // Push the current parameter values into the engine for the active mode. Done
    // every block (idempotent) so it survives a mode switch's override reset, and
    // so host automation reaches the engine with the editor closed.
    if (loaded && apvts != nullptr)
    {
        if (auto* br = apvts->getRawParameterValue (dm::params::id::pitchBendRange))
            engine.setPitchBendRange (br->load());
        if (const auto* m = getActiveMode())
        {
            dm::params::applyCcToParams (midi, *m, *apvts);     // mod wheel / CC → params
            dm::params::applyNoteSwitches (midi, *m, *apvts);   // key-switches → chord order
            dm::params::applyToEngine (engine, *m, *apvts);
        }
    }

    engine.processBlock (buffer, midi, getPlayHead());

    // Output peak for the editor's level meter (max-since-read; meter resets it).
    const float blockPeak = buffer.getMagnitude (0, buffer.getNumSamples());
    if (blockPeak > outputPeak.load (std::memory_order_relaxed))
        outputPeak.store (blockPeak, std::memory_order_relaxed);
}

juce::Image Omni84AudioProcessor::loadImage (const juce::String& id)
{
   #if OMNI84_HAS_ASSETS
    // Resolve a manifest image id ("img:Knob" → the "Knob.png" embedded resource).
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

juce::AudioProcessorEditor* Omni84AudioProcessor::createEditor()
{
    return new Omni84AudioProcessorEditor (*this);
}

void Omni84AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (apvts == nullptr)
        return;
    if (auto xml = apvts->copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Omni84AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (apvts == nullptr)
        return;
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts->state.getType()))
        {
            apvts->replaceState (juce::ValueTree::fromXml (*xml));
            triggerAsyncUpdate();   // restore active mode to match the Mode param (message thread)
        }
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Omni84AudioProcessor();
}
