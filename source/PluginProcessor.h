#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <SamplerEngine.h>   // from dehli_musikk_sampler_engine

/** Omni-84 plugin processor.

    M0 skeleton: a valid VST3/AU/Standalone instrument that loads and runs but
    produces silence. It owns a dm::SamplerEngine; real behaviour (modes, voices,
    FX, parameters) is added in later milestones — see ../../PLAN.md.
*/
class Omni84AudioProcessor : public juce::AudioProcessor
{
public:
    Omni84AudioProcessor();
    ~Omni84AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override            { return 1; }
    int getCurrentProgram() override         { return 0; }
    void setCurrentProgram (int) override    {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    dm::SamplerEngine& getEngine() noexcept { return engine; }

private:
    dm::SamplerEngine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Omni84AudioProcessor)
};
