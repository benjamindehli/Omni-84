#pragma once

// Omni-84 APVTS parameter system (M4c).
//
// Design: a FIXED, named union of every distinct control across all 7 modes, so
// DAW automation lanes never move and show real names. A param is simply inactive
// in a mode that has no matching control. Float params are stored NORMALISED
// (0..1); each mode maps them through its own control's min..max range (e.g.
// ENV_RELEASE is 0.03..0.5 in Bass but 0.25..6.0 in SonicStrings).
//
// The params are the single source of truth: the processor applies them to the
// engine every block (so automation works with the editor closed), and the editor
// only drives/reflects them. The binding->engine vocabulary lives here.

#include <juce_audio_processors/juce_audio_processors.h>
#include <model/Manifest.h>
#include <SamplerEngine.h>
#include <optional>

namespace omni84::params
{

// Stable parameter ids — these strings are the automation-lane identity, so never
// rename them once shipped.
namespace id
{
    inline constexpr const char* mode       = "mode";
    inline constexpr const char* filter     = "filter";
    inline constexpr const char* reverbMix  = "reverbMix";
    inline constexpr const char* release    = "release";
    inline constexpr const char* decay      = "decay";
    inline constexpr const char* voice1     = "voice1";
    inline constexpr const char* voice2     = "voice2";
    inline constexpr const char* strumSpeed = "strumSpeed";
    inline constexpr const char* fxEnable   = "fxEnable";
    inline constexpr const char* monoPoly   = "monoPoly";
    inline constexpr const char* velTrack   = "velTrack";
    inline constexpr const char* chordOrder = "chordOrder";
}

/** Build the fixed APVTS layout. Float defaults come from the manifest (the
    default value of the first control that drives each target, normalised). */
juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const dm::PresetLibrary& library);

/** Apply the current parameter values to the engine for `mode`. Idempotent and
    cheap — call it every block so a mode switch (which resets engine overrides)
    is re-honoured. Does NOT switch modes (that's the `mode` param, handled on the
    message thread by the processor). */
void applyToEngine (dm::SamplerEngine& engine,
                    const dm::Mode& mode,
                    juce::AudioProcessorValueTreeState& apvts);

/** The float param ids a control's bindings drive (for the editor to write/reflect).
    Usually one; a knob bound to ENV_DECAY+ENV_RELEASE returns both. */
juce::StringArray controlParamIds (const dm::Control& c);

/** The bool param id a button drives (decided by its state bindings), or empty. */
juce::String buttonParamId (const dm::Button& b);

} // namespace omni84::params
