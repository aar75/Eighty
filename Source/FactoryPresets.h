#pragma once
#include <vector>
#include <utility>
#include "Params.h"

namespace eighty
{
// Factory "starting points": a bare waveform with everything else out of the
// way, for when you want to build a sound rather than play one.
//
// A factory preset is expressed as *overrides on the parameter defaults*, not
// as a full snapshot. Loading one resets every parameter to its default and
// then applies this list, so anything not named here is guaranteed to be at
// its default rather than left over from whatever was loaded before. It also
// means the list below only has to say what is interesting about the sound.
//
// Deliberately silent: velocity response, aftertouch, drift, stereo spread
// and the chorus are all turned *off*, not just left alone. These are meant
// to be a clean canvas - you should hear the oscillator and nothing else, and
// dial the life back in yourself.
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> params;   // plain (not 0-1) values
};

// Shared by every starting point. Only parameters whose default is *not*
// already what we want appear here; the rest (noise, sub, sine, ring mod,
// delay, tremolo, LFO depths, glide, hold, arp, sequencer, cross-mod, sync,
// filter envelope amounts) are off by default and stay that way.
inline std::vector<std::pair<const char*, float>> plainBase()
{
    return {
        { ID::chorusOn,      0.f },     // the only effect that defaults to on
        { ID::velToAmp,      0.f },     // no velocity or pressure response
        { ID::velToFilter,   0.f },
        { ID::velBend,       0.f },
        { ID::touchToVib,    0.f },
        { ID::touchToBright, 0.f },
        { ID::touchToLevel,  0.f },
        { ID::drift,         0.f },     // no analog tolerances
        { ID::stereoSpread,  0.f },     // voices centred, not spread
    };
}

// Filter wide open and out of circuit, and a flat gate for an envelope: hold
// a key and you hear the raw waveform at a steady level.
inline void addPlainCS (std::vector<std::pair<const char*, float>>& p)
{
    p.insert (p.end(), {
        { ID::engineMode,  0.f },       // CS-80 panel
        { ID::osc2On,      0.f },       // one oscillator, not a detuned pair
        { ID::osc1Level,   1.f },
        { ID::lpfCutoff,   18000.f },
        { ID::resonance,   0.f },
        { ID::filterDrive, 0.f },
        { ID::keyTrack,    0.f },
        { ID::aEnvA,       0.002f },
        { ID::aEnvS,       1.f },
        { ID::aEnvR,       0.06f },
    });
}

inline void addPlainJP (std::vector<std::pair<const char*, float>>& p)
{
    p.insert (p.end(), {
        { ID::engineMode, 1.f },        // JP-8 panel
        { ID::jpMix,      0.f },        // VCO 1 only
        { ID::jpLpf,      18000.f },
        { ID::jpRes,      0.f },
        { ID::jpDrive,    0.f },
        { ID::jpKeyTrk,   0.f },
        { ID::jpAEnvA,    0.002f },
        { ID::jpAEnvS,    1.f },
        { ID::jpAEnvR,    0.06f },
    });
}

// Saw and pulse come off the CS-80 card, which mixes its saw and pulse
// outputs continuously; square and triangle come off the JP-8, whose VCO has
// them as discrete waveform positions. That split is also why they open on
// different panels.
inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = []
    {
        std::vector<FactoryPreset> list;

        {
            auto p = plainBase();
            addPlainCS (p);
            p.insert (p.end(), { { ID::osc1Saw, 1.f }, { ID::osc1Pulse, 0.f } });
            list.push_back ({ "Plain Saw", std::move (p) });
        }
        {
            auto p = plainBase();
            addPlainCS (p);
            // 25% duty: narrow enough to read as a pulse rather than a square,
            // and it puts a notch on every fourth harmonic (that hollow,
            // reedy character) instead of cancelling all the even ones.
            p.insert (p.end(), { { ID::osc1Saw, 0.f }, { ID::osc1Pulse, 1.f },
                                 { ID::osc1PW, 0.25f } });
            list.push_back ({ "Plain Pulse", std::move (p) });
        }
        {
            auto p = plainBase();
            addPlainJP (p);
            p.push_back ({ ID::jpVco1Wave, 3.f });     // Tri / Saw / Pulse / Square
            list.push_back ({ "Plain Square", std::move (p) });
        }
        {
            auto p = plainBase();
            addPlainJP (p);
            p.push_back ({ ID::jpVco1Wave, 0.f });
            list.push_back ({ "Plain Triangle", std::move (p) });
        }

        return list;
    }();
    return presets;
}
} // namespace eighty
