#include "Parameters.h"

namespace omni84::params
{

// ---------------------------------------------------------------------------
// Float-param targets: each maps to an engine binding parameter (+ optional
// group index, e.g. Voice 1/2). Order here = order in the DAW's lane list.
// ---------------------------------------------------------------------------
struct FloatTarget
{
    const char* id;
    const char* name;          // DAW display name
    const char* engineParam;   // binding.parameter to match
    int         matchGroup;    // groupIndex to match, or -1 = any
};

static const FloatTarget kFloatTargets[] =
{
    { id::filter,     "Filter",        "FX_FILTER_FREQUENCY", -1 },
    { id::reverbMix,  "Reverb Mix",    "FX_MIX",              -1 },
    { id::release,    "Release",       "ENV_RELEASE",         -1 },
    { id::decay,      "Decay",         "ENV_DECAY",           -1 },
    { id::voice1,     "Voice 1 Level", "AMP_VOLUME",           0 },
    { id::voice2,     "Voice 2 Level", "AMP_VOLUME",           1 },
    { id::strumSpeed, "Strum Speed",   "SEQ_PLAYBACK_RATE",   -1 },
};

// The binding in control `c` that drives target `t`, or nullptr.
static const dm::Binding* matchBinding (const dm::Control& c, const FloatTarget& t)
{
    for (const auto& b : c.bindings)
        if (b.parameter == t.engineParam
            && (t.matchGroup < 0 || (b.groupIndex && *b.groupIndex == t.matchGroup)))
            return &b;
    return nullptr;
}

static double normOf (const dm::Control& c)
{
    const double mn = c.min.value_or (0.0), mx = c.max.value_or (1.0);
    const double v  = c.value.value_or (mn);
    return mx > mn ? juce::jlimit (0.0, 1.0, (v - mn) / (mx - mn)) : 0.0;
}

// First control across all modes (in order) that drives `t` — used for the
// param's static default. Returns 0.5 if no mode uses this target.
static float defaultNorm (const dm::PresetLibrary& lib, const FloatTarget& t)
{
    for (const auto& m : lib.modes)
        if (! m.ui.tabs.isEmpty())
            for (const auto& c : m.ui.tabs.getReference (0).controls)
                if (matchBinding (c, t) != nullptr)
                    return (float) normOf (c);
    return 0.5f;
}

static juce::StringArray firstMenuOptions (const dm::PresetLibrary& lib)
{
    juce::StringArray names;
    for (const auto& m : lib.modes)
        if (! m.ui.tabs.isEmpty())
            for (const auto& menu : m.ui.tabs.getReference (0).menus)
                if (! menu.options.isEmpty())
                {
                    for (const auto& o : menu.options)
                        names.add (o.name);
                    return names;
                }
    return names;
}

// ---------------------------------------------------------------------------

juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const dm::PresetLibrary& lib)
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    StringArray modeNames;
    for (const auto& m : lib.modes) modeNames.add (m.name);
    if (modeNames.isEmpty()) modeNames.add ("Default");
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::mode, 1 }, "Mode", modeNames, 0));

    for (const auto& t : kFloatTargets)
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { t.id, 1 }, t.name, NormalisableRange<float> (0.0f, 1.0f), defaultNorm (lib, t)));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id::fxEnable, 1 }, "FX Enable",      false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id::monoPoly, 1 }, "Mono / Poly",    false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id::velTrack, 1 }, "Velocity Track", false));

    StringArray chordOpts = firstMenuOptions (lib);
    if (chordOpts.isEmpty()) chordOpts.add ("Default");
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::chordOrder, 1 }, "Chord Order", chordOpts, 0));

    return { p.begin(), p.end() };
}

// ---------------------------------------------------------------------------
// binding -> engine vocabulary (the M4c home of what used to live in the editor).
// `value` is the final engine value (native units, factor already applied).
// ---------------------------------------------------------------------------
static void applyBinding (dm::SamplerEngine& eng, const dm::Binding& b, double value)
{
    const auto& p = b.parameter;
    if      (p == "FX_FILTER_FREQUENCY") eng.setLowpassFrequency ((float) value);
    else if (p == "FX_MIX")              eng.setReverbMix ((float) value);
    else if (p == "FX_OUTPUT_LEVEL")     eng.setReverbWetGainDb ((float) value);
    else if (p == "SEQ_PLAYBACK_RATE")   eng.setSequencerRate (value);
    else if (p == "ENV_ATTACK")          eng.setAmpAttack ((float) value);
    else if (p == "ENV_DECAY")           eng.setAmpDecay ((float) value);
    else if (p == "ENV_SUSTAIN")         eng.setAmpSustain ((float) value);
    else if (p == "ENV_RELEASE")         eng.setAmpRelease ((float) value);
    else if (p == "AMP_VOLUME")          eng.setGroupVolume (b.groupIndex.value_or (0), (float) value);
    else if (p == "ENABLED")
    {
        const bool on = value > 0.5;
        if (b.level == "group" && b.groupIndex) eng.setGroupEnabled (*b.groupIndex, on);
        else if (b.effectIndex)                 eng.setEffectEnabled (*b.effectIndex, on);
    }
    // AMP_VEL_TRACK: no engine runtime param yet (velocity response is deferred).
}

static bool bindingIsTrue (const dm::Binding& b)
{
    return b.translationValue.isBool() ? (bool) b.translationValue
                                       : b.translationValue.toString().equalsIgnoreCase ("true");
}

// Apply a whole button state (each binding carries its own fixed translationValue).
static void applyButtonState (dm::SamplerEngine& eng, const dm::ButtonState& st)
{
    for (const auto& b : st.bindings)
    {
        if (b.parameter == "PATH") continue;
        if (b.parameter == "ENABLED")
        {
            const bool on = bindingIsTrue (b);
            if (b.level == "group" && b.groupIndex) eng.setGroupEnabled (*b.groupIndex, on);
            else if (b.effectIndex)                 eng.setEffectEnabled (*b.effectIndex, on);
        }
        // AMP_VEL_TRACK etc.: deferred.
    }
}

// Which bool param (if any) a button drives, by its state bindings.
static juce::String boolIdForButton (const dm::Button& b)
{
    for (const auto& st : b.states)
        for (const auto& bd : st.bindings)
        {
            if (bd.parameter == "ENABLED" && bd.level == "group") return id::monoPoly;
            if (bd.parameter == "ENABLED" && bd.effectIndex)      return id::fxEnable;
            if (bd.parameter == "AMP_VEL_TRACK")                  return id::velTrack;
        }
    return {};
}

// ---------------------------------------------------------------------------

void applyToEngine (dm::SamplerEngine& engine, const dm::Mode& mode,
                    juce::AudioProcessorValueTreeState& apvts)
{
    if (mode.ui.tabs.isEmpty())
        return;
    const auto& tab = mode.ui.tabs.getReference (0);

    auto raw = [&apvts] (const char* pid) -> float
    {
        if (auto* a = apvts.getRawParameterValue (pid)) return a->load();
        return 0.0f;
    };

    // Floats: per target, find the driving control in THIS mode and map norm->native.
    for (const auto& t : kFloatTargets)
        for (const auto& c : tab.controls)
            if (const auto* b = matchBinding (c, t))
            {
                const double mn = c.min.value_or (0.0), mx = c.max.value_or (1.0);
                const double native = mn + (double) raw (t.id) * (mx - mn);
                applyBinding (engine, *b, native * b->factor.value_or (1.0));
                break;   // one control per target per mode
            }

    // Bools: pick the state the param selects and apply its bindings.
    for (const auto& btn : tab.buttons)
    {
        const auto bid = boolIdForButton (btn);
        if (bid.isEmpty()) continue;
        const int stateIndex = raw (bid.toRawUTF8()) > 0.5f ? 1 : 0;
        if (stateIndex >= 0 && stateIndex < btn.states.size())
            applyButtonState (engine, btn.states.getReference (stateIndex));
    }

    // Chord-order menu -> sequencer index offset.
    for (const auto& menu : tab.menus)
        if (! menu.options.isEmpty())
        {
            const int sel = juce::jlimit (0, menu.options.size() - 1, (int) raw (id::chordOrder));
            engine.setSequencerIndexOffset (menu.options.getReference (sel).seqIndex);
            break;
        }
}

juce::StringArray controlParamIds (const dm::Control& c)
{
    juce::StringArray ids;
    for (const auto& t : kFloatTargets)
        if (matchBinding (c, t) != nullptr)
            ids.add (t.id);
    return ids;
}

juce::String buttonParamId (const dm::Button& b)
{
    return boolIdForButton (b);
}

} // namespace omni84::params
