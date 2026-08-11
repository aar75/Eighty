#pragma once
#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <functional>
#include <cstdint>
#include "Voice.h"
#include "LFO.h"
#include "Oversampling.h"

namespace eighty
{
// Voice management (poly / mono / legato / unison), hold latch,
// arpeggiator, global LFOs. Model-agnostic on top; the CS-80 voice card
// lives in Voice.h so a Jupiter-8 card can be added alongside it later.
class SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    enum class Mode { poly = 0, mono, legato, unison, stack };

    // Mono/legato/unison share the per-model held-note stack and a cached
    // voice (or voice group); poly and stack allocate freely per note.
    static bool isMonoish (Mode m)
    { return m == Mode::mono || m == Mode::legato || m == Mode::unison; }

    // Stack may give a single note the entire pool - that is the point of it
    static constexpr int kMaxStack = kMaxVoices;

    static constexpr int kAnyOwner = -99;   // wildcard for voice stealing

    struct EngineSettings   // refreshed per block by the processor
    {
        // engine assignment: 0 = CS-80 everywhere, 1 = JP-8 everywhere,
        // 2 = keyboard split at splitPoint (csLow picks which side is CS),
        // 3 = layer: every note plays both engines at once,
        // 4 = per-key map (keyMap: 0 = CS-80, 1 = JP-8, 2 = both)
        int engineMode = 0;
        int splitPoint = 60;
        bool csLow = true;
        std::array<uint8_t, 128> keyMap {};

        // Per-engine voice mode: CS-80 and JP-8 run independent Poly/Mono/
        // Legato/Unison settings, relevant whenever both can sound at once
        // (Split/Layer/Keys engine modes).
        struct VoiceCfg
        {
            Mode  mode = Mode::poly;
            int   polyVoices = 8;
            int   unisonCount = 4;
            float unisonDetune = 0.25f;  // 0..1 -> up to ~50 cents
        };
        VoiceCfg csVoice, jpVoice;

        int  bendRange = 2;
        bool hold = false;

        bool arpOn = false;
        int  arpMode = 0;            // up, down, updown, random, as played
        int  arpDiv = 5;
        int  arpOctaves = 1;
        float arpGate = 0.6f;

        float lfoRate = 4.5f;
        int   lfoWave = 0;
        float lfoDelay = 0.f;
        float pwmRate = 0.6f;

        // Effective tempo for the step clock: the processor resolves the
        // panel TEMPO against the host's, so everything in here counts in
        // note divisions rather than a free-running frequency.
        double bpm = 120.0;
    };

    VoiceParams vp;          // shared voice parameter snapshot
    EngineSettings es;

    // ---------------- Step sequencer --------------------------------------
    // Two tracks x 16 steps, tempo-synced to the arp's rate/div/sync/gate
    // (arp and sequencer are mutually exclusive - only one drives the step
    // clock). Tracks share the clock but have independent lengths, so they
    // can run polymetric. Live keyboard notes sound *over* a running
    // sequence rather than being swallowed by it.
    //
    // The pattern is plain fixed-size POD, deliberately: the editor grid
    // reads and writes it from the message thread while the audio thread
    // plays it. No allocation, no pointers, nothing to tear - the worst a
    // race can do is play a stale note for one step. `count` is always
    // published last when adding and cleared first when removing.
    struct SeqStep
    {
        static constexpr int kMaxNotes = 6;
        static constexpr int kMaxHold = 16;
        int8_t  note[kMaxNotes] {};
        uint8_t vel[kMaxNotes] {};
        uint8_t count = 0;                 // 0 = rest
        // How many steps this one occupies. 1 is a normal step; 4 holds the
        // notes across four steps and swallows the three it covers, which is
        // how you sustain a bass note under a busier pattern.
        uint8_t hold = 1;

        void clear() { count = 0; hold = 1; }
        bool has (int n) const
        {
            for (uint8_t i = 0; i < count; ++i) if (note[i] == (int8_t) n) return true;
            return false;
        }
        void add (int n, float v)
        {
            if (count >= kMaxNotes || has (n)) return;
            note[count] = (int8_t) std::clamp (n, 0, 127);
            vel[count]  = (uint8_t) std::clamp ((int) (v * 127.f), 1, 127);
            ++count;
        }
        void transpose (int semis)
        {
            for (uint8_t i = 0; i < count; ++i)
                note[i] = (int8_t) std::clamp ((int) note[i] + semis, 0, 127);
        }
    };

    struct SeqTrack
    {
        static constexpr int kSteps = 16;
        SeqStep steps[kSteps];
        int  length = kSteps;              // loop length, 1..16
        bool mute = false;
        int  engine = 0;                   // 0 auto (engine mode), 1 CS-80, 2 JP-8

        // playback state (audio thread only)
        int     playStep = -1;
        int     gateCounter = -1;
        int     holdRemaining = 0;         // steps the sounding step still occupies
        SeqStep sounding;                  // what this track is holding now
        int     soundModel = -1;           // forced model it was fired with
        int     owner = 0;                 // voice owner tag (= track index)
    };

    struct StepSeq
    {
        static constexpr int kTracks = 2;
        static constexpr int kSteps = SeqTrack::kSteps;
        SeqTrack tracks[kTracks];
        int  recTrack = 0;                 // armed track (also the edit cursor's track)
        int  recStep = 0;                  // record / edit cursor
        SeqStep recChord;                  // notes played for the step in progress
        int  recHeldCount = 0;             // how many are still physically held
        int  sampleCounter = 0;
        bool fireNow = false;
    } seq;
    bool seqRec = false, seqPlay = false;

    // Called by the processor on the rising edge of the REC/PLAY toggles.
    // REC does not wipe the track - the grid is visible and editable, so
    // clearing is an explicit action; recording overwrites from the cursor.
    void seqRecStart()
    {
        seq.recChord.clear();
        seq.recHeldCount = 0;
        seqRec = true;
    }

    void seqPlayStart()
    {
        for (auto& t : seq.tracks) { t.playStep = -1; t.gateCounter = -1; t.holdRemaining = 0; }
        seq.sampleCounter = 0;
        seq.fireNow = true;
        seqPlay = true;
    }

    void seqPlayStop()
    {
        for (auto& t : seq.tracks) stopTrack (t, echoBase);
        for (auto& t : seq.tracks) { t.playStep = -1; t.gateCounter = -1; t.holdRemaining = 0; }
        seq.fireNow = false;
        seqPlay = false;
    }

    // Message-thread edits from the grid. Safe against the audio thread for
    // the reasons above; clearing count first keeps a torn read silent
    // rather than wrong.
    void seqClearStep (int track, int step)
    {
        if (auto* s = stepAt (track, step)) s->clear();
    }
    void seqSetStep (int track, int step, const SeqStep& src)
    {
        if (auto* s = stepAt (track, step)) { s->clear(); *s = src; }
    }
    void seqClearTrack (int track)
    {
        for (int i = 0; i < SeqTrack::kSteps; ++i) seqClearStep (track, i);
    }
    void seqCopyTrack (int from, int to)
    {
        if (from == to) return;
        for (int i = 0; i < SeqTrack::kSteps; ++i)
            if (auto* s = stepAt (from, i)) seqSetStep (to, i, *s);
    }
    void seqSetHold (int track, int step, int steps)
    {
        if (auto* s = stepAt (track, step))
            s->hold = (uint8_t) std::clamp (steps, 1, SeqStep::kMaxHold);
    }
    void seqTransposeTrack (int track, int semis)
    {
        for (int i = 0; i < SeqTrack::kSteps; ++i)
            if (auto* s = stepAt (track, i)) s->transpose (semis);
    }
    SeqStep* stepAt (int track, int step)
    {
        if (track < 0 || track >= StepSeq::kTracks
            || step < 0 || step >= SeqTrack::kSteps) return nullptr;
        return &seq.tracks[track].steps[step];
    }

    // Echoes the engine's *effective* note stream (post hold-latch, post
    // arpeggiator) so a hosted synth layer can follow it. sampleOffset is
    // relative to the start of the current audio block.
    std::function<void (int note, float vel, bool on, int sampleOffset)> noteEcho;
    int echoBase = 0;        // set by the processor before midi-event calls

    // ---------------- Oversampling ----------------------------------------
    // The whole voice pool renders at osFactor * sampleRate and a cascade of
    // halfband stages brings it back down. That is what stops the filter
    // drive stages, hard sync and cross-mod from folding their own harmonics
    // back into the audible band. Everything inside render() therefore counts
    // in *oversampled* samples, including the arpeggiator and sequencer
    // clocks; only the note-echo offsets handed to a hosted synth layer are
    // converted back to base-rate samples (see echoPos).
    static constexpr int kMaxOversample = 8;

    void prepare (double sampleRate, int maxBlockSize)
    {
        baseSr = sampleRate;
        maxBlock = std::max (maxBlockSize, 32);
        sr = baseSr * osFactor;

        lfoBuf.resize ((size_t) maxBlock * kMaxOversample);
        pwmBuf.resize ((size_t) maxBlock * kMaxOversample);
        osL.resize ((size_t) maxBlock * kMaxOversample);
        osR.resize ((size_t) maxBlock * kMaxOversample);
        dsL.resize ((size_t) maxBlock);
        dsR.resize ((size_t) maxBlock);

        decim.prepare (maxBlock);
        decim.setFactor (osFactor);

        lfo.prepare (sr);
        pwmLfo.prepare (sr);
        for (int i = 0; i < kMaxVoices; ++i)
            voices[(size_t) i].prepare (sr, i, &vp);
        reset();
    }

    // 1, 2, 4 or 8. Allocation-free: the buffers are already sized for the
    // maximum, and every prepare-like call below only assigns a rate. Safe to
    // call from the audio thread when the user moves the quality selector.
    void setOversample (int factor)
    {
        const int f = factor >= 8 ? 8 : factor >= 4 ? 4 : factor >= 2 ? 2 : 1;
        if (f == osFactor) return;

        const int old = osFactor;
        osFactor = f;
        sr = baseSr * osFactor;

        decim.setFactor (osFactor);
        lfo.setSampleRate (sr);
        pwmLfo.setSampleRate (sr);
        for (auto& v : voices) v.setSampleRate (sr);

        // The step clocks count in oversampled samples, so rescale whatever
        // is mid-flight rather than letting the current step jump.
        auto rescale = [old, f] (int& v) { if (v > 0) v = (int) ((int64_t) v * f / old); };
        rescale (arpSampleCounter);
        rescale (arpGateCounter);
        rescale (seq.sampleCounter);
        for (auto& t : seq.tracks) rescale (t.gateCounter);
    }

    int oversampleFactor() const { return osFactor; }
    int latencySamples() const   { return decim.latencySamples(); }

    void reset()
    {
        decim.reset();
        for (auto& v : voices) v.kill();
        physicalHeld.clear();
        latched.clear();
        arpNotes.clear();
        monoStack[0].clear(); monoStack[1].clear();
        arpStep = 0; arpDir = 1; arpSampleCounter = 0; arpGateCounter = -1;
        arpSounding = -1;
        lfoEnvSeconds = 1000.f;

        seq.sampleCounter = 0; seq.fireNow = false;
        for (auto& t : seq.tracks)
        {
            t.playStep = -1; t.gateCounter = -1; t.holdRemaining = 0;
            t.sounding.clear(); t.soundModel = -1;
        }
    }

    // ---------------- MIDI-facing API (called between render segments) ----
    void noteOn (int note, float vel)
    {
        if (es.hold && physicalHeld.empty() && ! latched.empty())
            clearLatched();                      // new phrase replaces latch

        addUnique (physicalHeld, note);
        addUnique (latched, note);
        lastVelocity = vel;
        if (physicalHeld.size() == 1)
            lfoEnvSeconds = 0.f;                 // restart LFO fade-in

        if (seqRec)
            seqRecNoteOn (note, vel);

        if (es.arpOn)
        {
            addArpNote (note, vel);
            return;
        }
        // Live notes sound on top of a running sequence - playing over the
        // pattern is the point of the sequencer.
        echo (note, vel, true, echoBase);
        triggerNote (note, vel);
    }

    void noteOff (int note)
    {
        removeVal (physicalHeld, note);
        if (seqRec)
            seqRecNoteOff (note);
        if (es.hold)
            return;                              // latched until hold released
        removeVal (latched, note);

        if (es.arpOn) { removeArpNote (note); return; }
        echo (note, 0.f, false, echoBase);
        releaseNote (note);
    }

    void setHold (bool h)
    {
        if (es.hold == h) return;
        es.hold = h;
        if (! h)
        {
            // release everything not physically held
            for (int i = (int) latched.size() - 1; i >= 0; --i)
            {
                int n = latched[(size_t) i];
                if (! contains (physicalHeld, n))
                {
                    removeVal (latched, n);
                    if (es.arpOn) removeArpNote (n);
                    else          { echo (n, 0.f, false, echoBase); releaseNote (n); }
                }
            }
        }
    }

    void setSustainPedal (bool down)
    {
        sustainPedal = down;
        for (auto& v : voices)
            if (v.wasActive()) v.setSustained (down);
    }

    void setPitchBend (int raw14) { bendTarget = ((float) raw14 - 8192.f) / 8192.f; }
    void setBendNorm (float b)    { bendTarget = b; }   // -1..1 (UI wheel / arrow keys)
    void setModWheel (float m)    { vp.modWheel = m; }
    void setChannelPressure (float p) { vp.channelPressure = p; }
    void setPolyPressure (int note, float p)
    {
        for (auto& v : voices)
            if (v.currentNote() == note && v.wasActive()) v.setPolyPressure (p);
    }

    void allNotesOff()
    {
        for (int n : latched) echo (n, 0.f, false, echoBase);
        physicalHeld.clear(); latched.clear(); arpNotes.clear();
        monoStack[0].clear(); monoStack[1].clear();
        stopArpNote (echoBase);
        for (auto& t : seq.tracks) { stopTrack (t, echoBase); t.gateCounter = -1; t.holdRemaining = 0; }
        seq.fireNow = false;
        for (auto& v : voices) v.release();
    }

    float currentBend() const { return bendCurrent; }
    int activeVoiceCount() const
    {
        int n = 0;
        for (auto& v : voices) if (v.wasActive()) ++n;
        return n;
    }

    // Voices currently held by one owner (Voice::kOwnerLive, or a sequencer
    // track index) - how you tell live playing apart from the pattern.
    int heldVoiceCount (int owner) const
    {
        int n = 0;
        for (auto& v : voices)
            if (v.isHeld() && v.currentOwner() == owner) ++n;
        return n;
    }

    // ---------------- Render ---------------------------------------------
    void render (float* left, float* right, int numSamples, int blockOffset = 0)
    {
        if (numSamples <= 0) return;

        if (osFactor == 1)
        {
            renderSegment (left, right, numSamples, blockOffset);
            return;
        }

        // Generate at the oversampled rate into scratch, then decimate. The
        // scratch is sized for maxBlock base samples, so long segments are
        // taken in chunks; the decimator's state carries across them.
        int done = 0;
        while (done < numSamples)
        {
            const int chunk = std::min (numSamples - done, maxBlock);
            const int n = chunk * osFactor;
            std::fill (osL.begin(), osL.begin() + n, 0.f);
            std::fill (osR.begin(), osR.begin() + n, 0.f);

            renderSegment (osL.data(), osR.data(), n, blockOffset + done);

            decim.process (osL.data(), dsL.data(), chunk, 0);
            decim.process (osR.data(), dsR.data(), chunk, 1);
            for (int i = 0; i < chunk; ++i)
            {
                left[done + i]  += dsL[(size_t) i];
                right[done + i] += dsR[(size_t) i];
            }
            done += chunk;
        }
    }

    // Called when arp is switched off mid-flight
    void arpModeChanged (bool nowOn)
    {
        if (nowOn)
        {
            // move currently latched notes into the arp
            for (int n : latched) echo (n, 0.f, false, echoBase);
            for (auto& v : voices) v.release();
            arpNotes.clear();
            for (int n : latched) addArpNote (n, lastVelocity);
        }
        else
        {
            stopArpNote (echoBase);
            arpNotes.clear();
            // retrigger latched notes as normal voices
            for (int n : latched)
            {
                echo (n, lastVelocity, true, echoBase);
                triggerNote (n, lastVelocity);
            }
        }
    }

private:
    EngineSettings::VoiceCfg& cfgFor (int model)
    { return model == modelJP8 ? es.jpVoice : es.csVoice; }
    const EngineSettings::VoiceCfg& cfgFor (int model) const
    { return model == modelJP8 ? es.jpVoice : es.csVoice; }

    // Which engine(s) a note plays. Fills models[0..1], returns the count.
    int modelsForNote (int note, int* models) const
    {
        switch (es.engineMode)
        {
            case 0: models[0] = modelCS80; return 1;
            case 1: models[0] = modelJP8;  return 1;
            case 3: models[0] = modelCS80; models[1] = modelJP8; return 2;
            case 4:
            {
                const int z = es.keyMap[(size_t) std::clamp (note, 0, 127)];
                if (z >= 2) { models[0] = modelCS80; models[1] = modelJP8; return 2; }
                models[0] = z == 1 ? modelJP8 : modelCS80;
                return 1;
            }
            default:   // split
            {
                const bool low = note < es.splitPoint;
                models[0] = (low == es.csLow) ? modelCS80 : modelJP8;
                return 1;
            }
        }
    }

    // ---------------- Voice triggering ------------------------------------
    // A note may sound on one or both engines (modelsForNote); each engine
    // dispatches through its own independent Poly/Mono/Legato/Unison/Stack
    // mode. forceModel pins the note to one engine regardless of the engine
    // mode - the sequencer uses it to give each track its own voice card.
    int layersFor (int note, int* models, int forceModel) const
    {
        if (forceModel >= 0) { models[0] = forceModel; return 1; }
        return modelsForNote (note, models);
    }

    void triggerNote (int note, float vel, int forceModel = -1,
                      int owner = Voice::kOwnerLive)
    {
        int models[2];
        const int layers = layersFor (note, models, forceModel);
        const float gain = layers == 2 ? 0.72f : 1.f;
        for (int li = 0; li < layers; ++li)
            triggerForModel (note, vel, models[li], gain, owner);
    }

    // Mono/legato/unison keep a single shared note stack per engine, so a
    // sequencer track and the keyboard genuinely contend for it there -
    // ownership only separates them in the poly and stack modes, which is
    // where playing over a pattern makes sense anyway.
    void triggerForModel (int note, float vel, int model, float gain, int owner)
    {
        auto& cfg = cfgFor (model);
        switch (cfg.mode)
        {
            case Mode::poly:   triggerPolyModel   (note, vel, model, gain, owner); break;
            case Mode::mono:   triggerMonoModel   (note, vel, model, gain, true, owner); break;
            case Mode::legato: triggerMonoModel   (note, vel, model, gain, monoStack[model].empty(), owner); break;
            case Mode::unison: triggerUnisonModel (note, vel, model, gain, true, owner); break;
            case Mode::stack:  triggerStackModel  (note, vel, model, gain, owner); break;
        }
        if (isMonoish (cfg.mode))
            addUnique (monoStack[model], note);
        lastTriggeredNote[model] = note;
    }

    void releaseNote (int note, int forceModel = -1, int owner = Voice::kOwnerLive)
    {
        int models[2];
        const int layers = layersFor (note, models, forceModel);
        bool done[2] = { false, false };
        for (int li = 0; li < layers; ++li)
        {
            releaseForModel (note, models[li], owner);
            done[models[li]] = true;
        }

        // The engine mode can change while a key is down (CS -> JP, Layer ->
        // one card, a repainted key zone), so the card this note-off routes
        // to is not always the one the note-on triggered. Anything still
        // sounding on the other card gets its own release pass - otherwise
        // that voice hangs until you happen to press the key again.
        for (int m = 0; m < 2; ++m)
            if (! done[m] && stillSounding (note, m, owner))
                releaseForModel (note, m, owner);
    }

    bool stillSounding (int note, int model, int owner) const
    {
        if (contains (monoStack[model], note)) return true;
        for (auto& v : voices)
            if (v.isHeld() && v.currentNote() == note
                && v.currentModel() == model && v.currentOwner() == owner)
                return true;
        return false;
    }

    void releaseForModel (int note, int model, int owner = Voice::kOwnerLive)
    {
        auto& cfg = cfgFor (model);
        if (isMonoish (cfg.mode))
            releaseMonoish (note, model, owner);

        // A note entered in a mono-ish mode leaves an entry in that card's
        // note stack. The poly branch would never take it out, and a stale
        // entry re-triggers a phantom note the next time the mode comes back.
        removeVal (monoStack[model], note);

        // Poly and stack release every voice on the note - and so do the
        // mono-ish modes once they have run their own note-stack fallback,
        // because the voice mode can change while a key is down too. A note
        // played in Poly and released in Mono left voices sounding that
        // nothing above would ever have looked for.
        for (auto& v : voices)
            if (v.isHeld() && v.currentNote() == note && v.currentModel() == model
                && v.currentOwner() == owner)
            {
                if (sustainPedal) v.setSustained (true);
                v.noteOff();
            }
    }

    void releaseMonoish (int note, int model, int owner)
    {
        auto& cfg = cfgFor (model);
        auto& stack = monoStack[model];
        removeVal (stack, note);
        if (! stack.empty())
        {
            const int back = stack.back();
            if (back != soundingMonoNote (model))
            {
                int mdls[2];
                const int lyr = modelsForNote (back, mdls);
                const float gain = lyr == 2 ? 0.72f : 1.f;
                if (cfg.mode == Mode::mono)        triggerMonoModel   (back, lastVelocity, model, gain, true, owner);
                else if (cfg.mode == Mode::legato) triggerMonoModel   (back, lastVelocity, model, gain, false, owner);
                else                                triggerUnisonModel (back, lastVelocity, model, gain, false, owner);
            }
        }
        else if (cfg.mode == Mode::unison)
        {
            for (auto* v : unisonVoices[model])
                if (v != nullptr && v->isHeld())
                {
                    if (sustainPedal) v->setSustained (true);
                    v->noteOff();
                }
        }
        else if (Voice* v = monoVoice[model])
        {
            if (v->isHeld())
            {
                if (sustainPedal) v->setSustained (true);
                v->noteOff();
            }
        }
    }

    int soundingMonoNote (int model) const
    {
        if (cfgFor (model).mode == Mode::unison)
        {
            for (auto* v : unisonVoices[model])
                if (v != nullptr && v->isHeld()) return v->currentNote();
            return -1;
        }
        const Voice* v = monoVoice[model];
        return (v != nullptr && v->isHeld()) ? v->currentNote() : -1;
    }

    void triggerPolyModel (int note, float vel, int model, float gain, int owner)
    {
        float glideFrom = glideSource (model, false);
        Voice* v = allocateVoice (model, owner);
        v->noteOn (note, vel, 0.f, 0.f, glideFrom, true, gain, model, owner);
        if (sustainPedal) v->setSustained (true);
    }

    void triggerMonoModel (int note, float vel, int model, float gain, bool retrig, int owner)
    {
        Voice*& v = monoVoice[model];
        if (v == nullptr) v = allocateVoice (model, owner);
        float glideFrom = glideSource (model, v->wasActive());
        if (! retrig && v->wasActive())
            v->changeNote (note);
        else
            v->noteOn (note, vel, 0.f, 0.f, glideFrom, retrig || ! v->wasActive(), gain, model, owner);
        if (sustainPedal) v->setSustained (true);
    }

    void triggerUnisonModel (int note, float vel, int model, float noteGain, bool retrig, int owner)
    {
        auto& cfg = cfgFor (model);
        const int count = std::min (cfg.unisonCount, kMaxVoices);
        const float maxDet = cfg.unisonDetune * 50.f;
        const float gain = noteGain / std::sqrt ((float) count);
        auto& stack = unisonVoices[model];

        float glideFrom = glideSource (model, ! stack.empty() && stack[0] != nullptr && stack[0]->wasActive());

        // Resize the cached voice stack to the current unison count,
        // stopping/dropping any extras (e.g. the count changed mid-hold).
        for (size_t i = (size_t) count; i < stack.size(); ++i)
            if (stack[i] != nullptr && stack[i]->isHeld()) stack[i]->noteOff();
        stack.resize ((size_t) count, nullptr);

        for (int i = 0; i < count; ++i)
        {
            float pos = count > 1 ? ((float) i / (float) (count - 1) - 0.5f) * 2.f : 0.f;
            Voice*& v = stack[(size_t) i];
            if (v == nullptr) v = allocateVoice (model, owner);
            if (! retrig && v->wasActive())
                v->changeNote (note);
            else
                v->noteOn (note, vel, pos * maxDet, pos * 0.8f, glideFrom,
                           retrig || ! v->wasActive(), gain, model, owner);
            if (sustainPedal) v->setSustained (true);
        }
    }

    // ---- Stack: spread the whole voice pool over the notes being played.
    // One note gets all 16 cards, two notes get 8 each, and so on - the
    // Jupiter-8 unison trick, but polyphonic. Each card is detuned and
    // panned across the stack, so a single note becomes a wall instead of
    // one thin oscillator pair. As notes are added the existing stacks are
    // *released* down to size rather than stolen, so thinning is a fade,
    // not a click.
    void triggerStackModel (int note, float vel, int model, float noteGain, int owner)
    {
        auto& cfg = cfgFor (model);
        const int cap = std::min (cfg.polyVoices, kMaxVoices);
        // The pool is shared, so the split is over *every* note sounding on
        // this engine, whoever triggered it: a sequencer track must not grab
        // all 16 cards and evict what you are playing live. Only note-off
        // respects ownership - thinning here is a release, not a steal.
        const int distinct = distinctHeldNotes (model, note) + 1;
        const int per = std::clamp (cap / std::max (1, distinct), 1, kMaxStack);

        shrinkStacks (model, per, note);

        const float maxDet = cfg.unisonDetune * 50.f;
        const float gain = noteGain / std::sqrt ((float) per);
        const float glideFrom = glideSource (model, false);
        for (int i = 0; i < per; ++i)
        {
            const float pos = per > 1 ? ((float) i / (float) (per - 1) - 0.5f) * 2.f : 0.f;
            Voice* v = allocateVoice (model, owner);
            v->noteOn (note, vel, pos * maxDet, pos * 0.8f, glideFrom, true, gain, model, owner);
            if (sustainPedal) v->setSustained (true);
        }
    }

    bool inStackGroup (const Voice& v, int model) const
    { return v.isHeld() && v.currentModel() == model; }

    int distinctHeldNotes (int model, int exceptNote) const
    {
        int n = 0;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            const Voice& v = voices[(size_t) i];
            if (! inStackGroup (v, model)) continue;
            const int note = v.currentNote();
            if (note == exceptNote) continue;
            bool seen = false;
            for (int j = 0; j < i && ! seen; ++j)
                seen = inStackGroup (voices[(size_t) j], model)
                    && voices[(size_t) j].currentNote() == note;
            if (! seen) ++n;
        }
        return n;
    }

    void shrinkStacks (int model, int per, int exceptNote)
    {
        for (int i = 0; i < kMaxVoices; ++i)
        {
            Voice& v = voices[(size_t) i];
            if (! inStackGroup (v, model)) continue;
            const int note = v.currentNote();
            if (note == exceptNote) continue;
            int seen = 0;
            for (int j = 0; j < i; ++j)
                if (inStackGroup (voices[(size_t) j], model)
                    && voices[(size_t) j].currentNote() == note) ++seen;
            if (seen >= per) v.releaseNow();
        }
    }

    float glideSource (int model, bool voiceActive) const
    {
        const int last = lastTriggeredNote[model];
        const int glideMode = vp.glide[model == modelJP8 ? 1 : 0].mode;
        if (glideMode == 2 && last >= 0)   // always
            return (float) last;
        if (glideMode == 1)                // legato only
        {
            bool overlap = voiceActive || ! physicalHeld.empty() || ! monoStack[model].empty();
            if (overlap && last >= 0)
                return (float) last;
        }
        return -1.f;
    }

    // Draws from the shared 16-voice pool but never lets one engine steal
    // a voice actively serving the other engine while under its own cap -
    // otherwise two independent per-engine polyphony limits would just
    // fight over the same low index range.
    Voice* allocateVoice (int model, int owner)
    {
        const int cap = std::min (cfgFor (model).polyVoices, kMaxVoices);
        int activeForModel = 0;
        for (auto& v : voices)
            if (v.wasActive() && v.currentModel() == model) ++activeForModel;

        if (activeForModel < cap)
            for (auto& v : voices)
                if (! v.wasActive()) return &v;

        // At/over budget, or no free voice at all. Steal from the same owner
        // before anyone else, so a busy sequencer track recycles its own
        // cards instead of chewing through notes you are playing live.
        auto quietestReleasing = [this, model] (int wantOwner) -> Voice*
        {
            Voice* best = nullptr; float bestLevel = 1e9f;
            for (auto& v : voices)
                if (v.currentModel() == model && v.isReleasing() && v.envLevel() < bestLevel
                    && (wantOwner == kAnyOwner || v.currentOwner() == wantOwner))
                    { best = &v; bestLevel = v.envLevel(); }
            return best;
        };
        auto oldestOf = [this, model] (int wantOwner) -> Voice*
        {
            uint32_t oldest = 0xFFFFFFFFu; Voice* pick = nullptr;
            for (auto& v : voices)
                if (v.currentModel() == model && v.age() < oldest
                    && (wantOwner == kAnyOwner || v.currentOwner() == wantOwner))
                    { oldest = v.age(); pick = &v; }
            return pick;
        };

        if (Voice* v = quietestReleasing (owner))    return v;
        if (Voice* v = quietestReleasing (kAnyOwner)) return v;
        if (Voice* v = oldestOf (owner))             return v;
        if (Voice* v = oldestOf (kAnyOwner))         return v;

        // This model has no voices of its own to steal from (the other
        // engine currently holds all of them) - fall back to the globally
        // oldest voice so we always return something playable.
        uint32_t oldest = 0xFFFFFFFFu;
        Voice* pick = &voices[0];
        for (auto& v : voices)
            if (v.age() < oldest) { oldest = v.age(); pick = &v; }
        return pick;
    }

    // numSamples is in oversampled samples; blockOffset stays in base-rate
    // samples because it only feeds the note-echo timestamps.
    void renderSegment (float* left, float* right, int numSamples, int blockOffset)
    {
        renderBase = blockOffset;

        // Smooth pitch bend toward target (fast but click-free)
        const float bCoef = 1.f - std::exp (-(float) numSamples / ((float) sr * 0.005f));
        bendCurrent += (bendTarget - bendCurrent) * bCoef;
        vp.bendSemis = bendCurrent * (float) es.bendRange;

        // Global LFOs, one value per sample
        lfo.setRate (es.lfoRate);
        lfo.setWave (es.lfoWave);
        pwmLfo.setRate (es.pwmRate);
        pwmLfo.setWave (LFO::triangle);
        const float dt = 1.f / (float) sr;
        for (int i = 0; i < numSamples; ++i)
        {
            lfoEnvSeconds += dt;
            float fade = es.lfoDelay <= 0.01f ? 1.f
                       : std::fmin (1.f, lfoEnvSeconds / es.lfoDelay);
            lfoBuf[(size_t) i] = lfo.tick() * fade;
            pwmBuf[(size_t) i] = pwmLfo.tick();
        }

        if (es.arpOn)
            renderWithArp (left, right, numSamples);
        else if (seqPlay)
            renderWithSeq (left, right, numSamples);
        else
            renderVoices (left, right, 0, numSamples);
    }

    // Note-echo timestamps are consumed by the host's MIDI stream, so they
    // have to come back out of the oversampled clock.
    int echoPos (int osOffset) const { return renderBase + osOffset / osFactor; }

    void renderVoices (float* left, float* right, int offset, int num)
    {
        for (auto& v : voices)
            v.render (left + offset, right + offset, num,
                      lfoBuf.data() + offset, pwmBuf.data() + offset);
    }

    // ---------------- Arpeggiator -----------------------------------------
    struct ArpNote { int note; float vel; };

    void addArpNote (int note, float vel)
    {
        for (auto& a : arpNotes) if (a.note == note) return;
        if (arpNotes.empty())
        {
            arpStep = 0;
            arpFireNow = true;    // first note of a phrase sounds immediately
        }
        arpNotes.push_back ({ note, vel });
    }

    void removeArpNote (int note)
    {
        for (size_t i = 0; i < arpNotes.size(); ++i)
            if (arpNotes[i].note == note) { arpNotes.erase (arpNotes.begin() + (long) i); break; }
        if (arpNotes.empty())
        {
            arpStep = 0; arpDir = 1;
        }
    }

    // Step length for both the arpeggiator and the sequencer. Always a note
    // division of the effective tempo - there is no free-running Hz mode any
    // more, because "1/16 at 124 BPM" is the thing you actually want to dial.
    int arpStepLengthSamples() const
    {
        static const double beats[] = { 4.0, 2.0, 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0, 0.125 };
        const double b = beats[std::max (0, std::min (7, es.arpDiv))];
        return std::max (32, (int) (b * 60.0 / std::fmax (1.0, es.bpm) * sr));
    }

    void renderWithArp (float* left, float* right, int numSamples)
    {
        const int stepLen = arpStepLengthSamples();
        int pos = 0;
        while (pos < numSamples)
        {
            // Fire any events due right now
            if (arpFireNow && ! arpNotes.empty())
            {
                arpFireNow = false;
                arpSampleCounter = 0;
                advanceArpStep (stepLen, echoPos (pos));
            }
            if (arpGateCounter == 0) { stopArpNote (echoPos (pos)); arpGateCounter = -1; }
            if (! arpNotes.empty() && arpSampleCounter >= stepLen)
            {
                arpSampleCounter = 0;
                advanceArpStep (stepLen, echoPos (pos));
            }

            // Render up to the next event boundary
            int chunk = numSamples - pos;
            if (! arpNotes.empty())  chunk = std::min (chunk, std::max (1, stepLen - arpSampleCounter));
            if (arpGateCounter > 0)  chunk = std::min (chunk, arpGateCounter);

            renderVoices (left, right, pos, chunk);
            if (! arpNotes.empty()) arpSampleCounter += chunk;
            if (arpGateCounter > 0) arpGateCounter -= chunk;
            pos += chunk;
        }
    }

    void advanceArpStep (int stepLen, int sampleOffset = 0)
    {
        stopArpNote (sampleOffset);
        if (arpNotes.empty()) return;

        // Build the pattern: held notes expanded across octaves
        pattern.clear();
        for (int oct = 0; oct < es.arpOctaves; ++oct)
            for (auto& a : arpNotes)
                pattern.push_back ({ a.note + 12 * oct, a.vel });

        const int n = (int) pattern.size();
        int idx = 0;
        switch (es.arpMode)
        {
            case 0: // up
                std::sort (pattern.begin(), pattern.end(), byPitch);
                idx = arpStep % n;
                ++arpStep;
                break;
            case 1: // down
                std::sort (pattern.begin(), pattern.end(), byPitch);
                idx = (n - 1) - (arpStep % n);
                ++arpStep;
                break;
            case 2: // up-down
            {
                std::sort (pattern.begin(), pattern.end(), byPitch);
                if (n == 1) { idx = 0; break; }
                int cycle = 2 * n - 2;
                int s = arpStep % cycle;
                idx = s < n ? s : cycle - s;
                ++arpStep;
                break;
            }
            case 3: // random
                idx = (int) (rng() % (uint32_t) n);
                break;
            case 4: // as played (sequencer)
            default:
                idx = arpStep % n;
                ++arpStep;
                break;
        }

        auto& sel = pattern[(size_t) idx];
        int note = std::min (127, sel.note);
        echo (note, sel.vel, true, sampleOffset);
        triggerNote (note, sel.vel);
        arpSounding = note;
        arpGateCounter = std::max (16, (int) (es.arpGate * (float) stepLen));
    }

    void stopArpNote (int sampleOffset = 0)
    {
        if (arpSounding >= 0)
        {
            echo (arpSounding, 0.f, false, sampleOffset);
            int models[2];
            const int layers = modelsForNote (arpSounding, models);
            for (int li = 0; li < layers; ++li)
            {
                const int model = models[li];
                if (! isMonoish (cfgFor (model).mode))
                    releaseForModel (arpSounding, model);
                else
                {
                    // Blanket stop rather than the usual mono/legato/unison
                    // stack fallback, so the arp always goes fully silent.
                    monoStack[model].clear();
                    if (Voice* v = monoVoice[model]) if (v->isHeld()) v->noteOff();
                    for (auto* v : unisonVoices[model]) if (v != nullptr && v->isHeld()) v->noteOff();
                }
            }
            arpSounding = -1;
        }
    }

    // ---------------- Step sequencer: recording ----------------------------
    // Step record: notes played land on the cursor step of the armed track;
    // the cursor advances once every key of the chord is released, so you
    // enter a pattern at your own pace. Recording stops at the track's loop
    // length. The cursor is also the grid's edit position, so clicking a
    // step in the UI moves where the next note lands.
    void seqRecNoteOn (int note, float vel)
    {
        if (seq.recChord.has (note)) return;
        seq.recChord.add (note, vel);
        ++seq.recHeldCount;
    }

    void seqRecNoteOff (int note)
    {
        if (! seq.recChord.has (note)) return;
        --seq.recHeldCount;
        if (seq.recHeldCount > 0 || seq.recChord.count == 0) return;

        auto& track = seq.tracks[(size_t) std::clamp (seq.recTrack, 0, StepSeq::kTracks - 1)];
        const int len = std::clamp (track.length, 1, SeqTrack::kSteps);
        const int cursor = std::clamp (seq.recStep, 0, len - 1);
        const uint8_t keepHold = track.steps[(size_t) cursor].hold;   // step property
        track.steps[(size_t) cursor].clear();
        track.steps[(size_t) cursor] = seq.recChord;
        track.steps[(size_t) cursor].hold = keepHold;
        seq.recChord.clear();
        seq.recHeldCount = 0;
        if (++seq.recStep >= len)
        {
            seq.recStep = 0;
            seqRec = false;         // one pass round the loop, then stop
        }
    }

    // ---------------- Step sequencer: playback ------------------------------
    // Mirrors renderWithArp, but every track advances one step per tick (an
    // empty step is a rest, not a pause), fires its whole chord at once, and
    // holds its own gate - tracks with different lengths drift against each
    // other on purpose.
    void renderWithSeq (float* left, float* right, int numSamples)
    {
        const int stepLen = arpStepLengthSamples();   // shares the arp's rate/div/sync
        int pos = 0;
        while (pos < numSamples)
        {
            if (seq.fireNow)
            {
                seq.fireNow = false;
                seq.sampleCounter = 0;
                advanceSeqStep (stepLen, echoPos (pos));
            }
            for (auto& t : seq.tracks)
                if (t.gateCounter == 0) { stopTrack (t, echoPos (pos)); t.gateCounter = -1; }
            if (seq.sampleCounter >= stepLen)
            {
                seq.sampleCounter = 0;
                advanceSeqStep (stepLen, echoPos (pos));
            }

            int chunk = std::min (numSamples - pos, std::max (1, stepLen - seq.sampleCounter));
            for (auto& t : seq.tracks)
                if (t.gateCounter > 0) chunk = std::min (chunk, t.gateCounter);

            renderVoices (left, right, pos, chunk);
            seq.sampleCounter += chunk;
            for (auto& t : seq.tracks)
                if (t.gateCounter > 0) t.gateCounter -= chunk;
            pos += chunk;
        }
    }

    void advanceSeqStep (int stepLen, int sampleOffset)
    {
        for (int ti = 0; ti < StepSeq::kTracks; ++ti)
        {
            auto& t = seq.tracks[(size_t) ti];
            const int len = std::clamp (t.length, 1, SeqTrack::kSteps);
            t.playStep = (t.playStep + 1) % len;

            // Still inside a held step: the playhead moves on but the notes
            // keep sounding and this step's own contents are skipped.
            if (t.holdRemaining > 1) { --t.holdRemaining; continue; }

            stopTrack (t, sampleOffset);
            t.gateCounter = -1;

            const auto& st = t.steps[(size_t) t.playStep];
            const uint8_t n = std::min (st.count, (uint8_t) SeqStep::kMaxNotes);
            const int hold = n > 0 ? std::clamp ((int) st.hold, 1, SeqStep::kMaxHold) : 1;
            t.holdRemaining = hold;        // counted even when muted, so the
            if (t.mute) continue;          // pattern keeps its shape

            const int forced = t.engine == 1 ? modelCS80 : t.engine == 2 ? modelJP8 : -1;
            for (uint8_t i = 0; i < n; ++i)
            {
                const float vel = (float) st.vel[i] / 127.f;
                echo (st.note[i], vel, true, sampleOffset);
                triggerNote (st.note[i], vel, forced, ti);
            }
            t.sounding = st;
            t.soundModel = forced;
            t.owner = ti;
            // Whole steps held, plus the gate fraction of the last one, so
            // Gate still shortens a held note without cutting it to one step.
            if (n > 0)
                t.gateCounter = std::max (16, (int) (((float) (hold - 1) + es.arpGate)
                                                     * (float) stepLen));
        }
    }

    void stopTrack (SeqTrack& t, int sampleOffset)
    {
        const uint8_t n = std::min (t.sounding.count, (uint8_t) SeqStep::kMaxNotes);
        for (uint8_t i = 0; i < n; ++i)
        {
            echo (t.sounding.note[i], 0.f, false, sampleOffset);
            releaseNote (t.sounding.note[i], t.soundModel, t.owner);
        }
        t.sounding.clear();
    }

    void echo (int note, float vel, bool on, int offset)
    {
        if (noteEcho) noteEcho (note, vel, on, offset);
    }

    static bool byPitch (const ArpNote& a, const ArpNote& b) { return a.note < b.note; }

    // ---------------- helpers ---------------------------------------------
    static void addUnique (std::vector<int>& v, int n)
    { if (! contains (v, n)) v.push_back (n); }
    static void removeVal (std::vector<int>& v, int n)
    { v.erase (std::remove (v.begin(), v.end(), n), v.end()); }
    static bool contains (const std::vector<int>& v, int n)
    { return std::find (v.begin(), v.end(), n) != v.end(); }

    void clearLatched()
    {
        for (int n : latched)
        {
            if (es.arpOn) removeArpNote (n);
            else { echo (n, 0.f, false, echoBase); releaseNote (n); }
        }
        latched.clear();
        monoStack[0].clear(); monoStack[1].clear();
    }

    // sr is the *rendering* rate: baseSr * osFactor. Everything inside
    // render() - LFO increments, arp and sequencer step clocks, voice
    // control-rate smoothing - counts against it.
    double sr = 44100.0, baseSr = 44100.0;
    int maxBlock = 512, osFactor = 1;
    Decimator decim;
    std::vector<float> osL, osR, dsL, dsR;

    std::array<Voice, kMaxVoices> voices;
    LFO lfo, pwmLfo;
    std::vector<float> lfoBuf, pwmBuf;
    float lfoEnvSeconds = 1000.f;

    std::vector<int> physicalHeld, latched;
    std::vector<int> monoStack[2];             // per-model held-note stack (mono/legato/unison)
    Voice* monoVoice[2] = { nullptr, nullptr }; // per-model sounding voice (mono/legato)
    std::vector<Voice*> unisonVoices[2];        // per-model unison voice stack
    bool sustainPedal = false;
    float lastVelocity = 0.8f;
    int lastTriggeredNote[2] = { -1, -1 };

    float bendTarget = 0.f, bendCurrent = 0.f;

    int renderBase = 0;

    // arp state
    std::vector<ArpNote> arpNotes, pattern;
    int arpStep = 0, arpDir = 1;
    int arpSampleCounter = 0, arpGateCounter = -1, arpSounding = -1;
    bool arpFireNow = false;
    std::mt19937 rng { 0x8080 };
};
} // namespace eighty
