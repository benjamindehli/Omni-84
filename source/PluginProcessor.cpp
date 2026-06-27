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
}

Omni84AudioProcessor::~Omni84AudioProcessor() = default;

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
   #endif
}

juce::String Omni84AudioProcessor::getLoadedModeName() const
{
    return loaded ? library.modes.getReference (0).name : juce::String();
}

void Omni84AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Configure the engine for the current mode (M2: the single embedded mode).
    if (loaded)
        engine.setMode (library.modes.getReference (0), *sampleSource);
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

    engine.processBlock (buffer, midi, getPlayHead());
}

juce::AudioProcessorEditor* Omni84AudioProcessor::createEditor()
{
    return new Omni84AudioProcessorEditor (*this);
}

void Omni84AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // M4: persist Mode + parameter slots via APVTS.
    juce::ignoreUnused (destData);
}

void Omni84AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // M4: restore state.
    juce::ignoreUnused (data, sizeInBytes);
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Omni84AudioProcessor();
}
