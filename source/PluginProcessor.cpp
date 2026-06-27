#include "PluginProcessor.h"
#include "PluginEditor.h"

Omni84AudioProcessor::Omni84AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

Omni84AudioProcessor::~Omni84AudioProcessor() = default;

void Omni84AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
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
