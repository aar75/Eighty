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

        // Strum articulation. Hold leaves each string sounding until it is
        // damped; Pluck strikes it and lets go as the finger passes, so the
        // patch's release is what shapes the decay. Pluck is what makes a run
        // work in both directions - nothing is left holding the notes you
        // already went past.
        bool strumPluck = false;

        // The arpeggiator is per engine, indexed by VoiceModel: the CS-80 can
        // arpeggiate a chord while the JP-8 holds it underneath, or the other
        // way round. Everything else about the arp - mode, division, octaves,
        // rate, gate - is shared, so the two run the same figure on their own
        // cards rather than becoming two unrelated sequencers.
        bool arpOn[2] = { false, false };
        bool anyArp() const { return arpOn[0] || arpOn[1]; }

        int  arpMode = 0;            // up, down, updown, random, as played
        int  arpDiv = 5;
        int  arpOctaves = 1;
        float arpGate = 0.6f;
        // How many arp notes fit in one DIV step. At x1 the arp is exactly
        // the step clock, as it always was; above that it subdivides, which
        // is what lets an arpeggio run inside a sequenced chord rather than
        // only changing when the chord does.
        int  arpMult = 0;            // index into kArpMults

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

        // Arpeggiated playback. With the arp on, a step's chord is the figure
        // this track walks rather than a block to strike - which is what lets
        // you sequence chords and turn them into arpeggios with one switch.
        // Each track walks its own, so track A can stay a plain bass line
        // while track B arpeggiates, on its own engine.
        // A step splits the same way a live note does: the engines whose arp
        // is on walk the chord, the others hold it as a block. Both halves
        // can be live at once, so each has its own mask and its own gate.
        bool    soundArped = false;
        int     arpModelMask = 0;          // engines walking the figure
        int     blockMask = 0;             // engines holding the chord
        int     arpGateCounter = -1;
        int     arpPos = 0;                // position in the figure
        int     arpCounter = 0;            // samples until the next note
        int     arpNote = -1;              // note it is sounding right now
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
        for (auto& t : seq.tracks)
        {
            t.playStep = -1; t.gateCounter = -1; t.holdRemaining = 0;
            t.soundArped = false; t.arpNote = -1; t.arpPos = 0; t.arpCounter = 0;
            t.arpGateCounter = -1; t.arpModelMask = 0; t.blockMask = 0;
        }
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
        for (auto& a : arp) { rescale (a.sampleCounter); rescale (a.gateCounter); }
        rescale (seq.sampleCounter);
        for (auto& t : seq.tracks)
            { rescale (t.gateCounter); rescale (t.arpGateCounter); rescale (t.arpCounter); }
        for (int i = 0; i < numPlucks; ++i) rescale (plucks[(size_t) i].samples);
    }

    int oversampleFactor() const { return osFactor; }
    int latencySamples() const   { return decim.latencySamples(); }

    void reset()
    {
        decim.reset();
        for (auto& v : voices) v.kill();
        physicalHeld.clear();
        latched.clear();
        publishLatched();
        arpNotes.clear();
        monoStack[0].clear(); monoStack[1].clear();
        for (auto& a : arp) a = ArpState();
        lfoEnvSeconds = 1000.f;

        for (auto& h : strumHeld) h.clear();
        numPlucks = 0;
        for (auto& r : echoRef) r = 0;
        for (int i = 0; i < kStrumLanes; ++i) publishRing (i);
        gestureArmed = false; patternManual = false;
        ribbonTarget[0] = ribbonTarget[1] = 0.f;
        ribbonCurrent[0] = ribbonCurrent[1] = 0.f;
        vp.ribbonSemis[0] = vp.ribbonSemis[1] = 0.f;

        seq.sampleCounter = 0; seq.fireNow = false;
        for (auto& t : seq.tracks)
        {
            t.playStep = -1; t.gateCounter = -1; t.holdRemaining = 0;
            t.sounding.clear(); t.soundModel = -1;
            t.soundArped = false; t.arpNote = -1; t.arpPos = 0; t.arpCounter = 0;
            t.arpGateCounter = -1; t.arpModelMask = 0; t.blockMask = 0;
        }
    }

    // ---------------- MIDI-facing API (called between render segments) ----
    void noteOn (int note, float vel)
    {
        if (es.hold && physicalHeld.empty() && ! latched.empty())
            clearLatched();                      // new phrase replaces latch

        addUnique (physicalHeld, note);
        addUnique (latched, note);
        publishLatched();
        lastVelocity = vel;
        if (physicalHeld.size() == 1)
            lfoEnvSeconds = 0.f;                 // restart LFO fade-in

        if (seqRec)
            seqRecNoteOn (note, vel);

        if (es.anyArp())
            addArpNote (note, vel);

        // While the trackpad is armed the strum is the only thing that
        // sounds: the keyboard's job is to say *what* the chord is, and the
        // hand on the pad decides when any of it is heard.
        if (gestureArmed) { strumPruneLanes(); return; }

        // Straight to the voice pool on the engines whose arp is off; the
        // arpeggiated ones will sound it in their own time. With one arp on
        // in Layer mode that is exactly the point: one card holds the chord
        // while the other walks it.
        const int direct = directMask();
        if (direct == 0) return;
        if (triggerNote (note, vel, -1, Voice::kOwnerLive, direct) > 0)
            echo (note, vel, true, echoBase);
    }

    void noteOff (int note)
    {
        removeVal (physicalHeld, note);
        if (seqRec)
            seqRecNoteOff (note);
        if (es.hold)
            return;                              // latched until hold released
        removeVal (latched, note);
        publishLatched();

        if (es.anyArp()) removeArpNote (note);
        if (gestureArmed) { strumPruneLanes(); return; }

        const int direct = directMask();
        if (direct == 0) return;
        echo (note, 0.f, false, echoBase);
        releaseNote (note, -1, Voice::kOwnerLive, direct);
    }

    // The engines that play a live note straight, i.e. the ones not
    // arpeggiating it.
    int directMask() const
    { return (es.arpOn[0] ? 0 : 1) | (es.arpOn[1] ? 0 : 2); }

    // The engines that arpeggiate rather than sound a note straight.
    int arpMask() const
    { return (es.arpOn[0] ? 1 : 0) | (es.arpOn[1] ? 2 : 0); }

    void setHold (bool h)
    {
        if (es.hold == h) return;
        es.hold = h;
        if (! h)
        {
            // Anything the strum left ringing was being held by the latch on
            // its behalf, so it goes with it.
            strumReleaseAll();

            // release everything not physically held
            for (int i = (int) latched.size() - 1; i >= 0; --i)
            {
                int n = latched[(size_t) i];
                if (! contains (physicalHeld, n))
                {
                    removeVal (latched, n);
                    if (es.anyArp()) removeArpNote (n);
                    else          { echo (n, 0.f, false, echoBase); releaseNote (n); }
                }
            }
            publishLatched();
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
        strumReleaseAll();
        numPlucks = 0;
        echoFlush (echoBase);
        physicalHeld.clear(); latched.clear(); arpNotes.clear();
        publishLatched();
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
    // Walks every voice currently held down, as (note, model, owner). The
    // per-engine arpeggiator is defined by which card is sounding what, so
    // that is what has to be inspectable to check it.
    template <typename Fn>
    void forEachHeldVoice (Fn&& fn) const
    {
        for (auto& v : voices)
            if (v.isHeld())
                fn (v.currentNote(), v.currentModel(), v.currentOwner());
    }

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

    // ---------------- Trackpad gestures -----------------------------------
    // A strum is one hand crossing the notes you are holding. Everything it
    // plays goes through the same triggerNote/releaseNote path as the
    // keyboard and the sequencer, tagged with its own voice owner, so it can
    // hold strings ringing without a keyboard note-off - or a sequencer step
    // ending - cutting them off underneath it.
    static constexpr int kStrumLanes = 2;      // one per finger
    static constexpr int kOwnerStrum = 10;     // + lane; sequencer tracks are 0/1
    static constexpr int kMaxRing = 8;         // strings one lane may leave ringing

    static int countTrailingZeros (uint32_t v)
    {
        int n = 0;
        while ((v & 1u) == 0u) { v >>= 1; ++n; }
        return n;
    }

    // Publishes `latched` for the message thread. Called from every place
    // that edits it, so the strum never lays its strings out against a set
    // the engine has already moved on from.
    void publishRing (int lane)
    {
        uint32_t bits[4] = { 0, 0, 0, 0 };
        for (auto& h : strumHeld[(size_t) lane])
            if (h.note >= 0 && h.note < 128) bits[h.note >> 5] |= (1u << (h.note & 31));
        for (int i = 0; i < 4; ++i)
            ringBits[lane][(size_t) i].store (bits[i], std::memory_order_relaxed);
    }

    void publishLatched()
    {
        uint32_t bits[4] = { 0, 0, 0, 0 };
        for (int n : latched)
            if (n >= 0 && n < 128) bits[n >> 5] |= (1u << (n & 31));
        for (int i = 0; i < 4; ++i)
            latchedBits[(size_t) i].store (bits[i], std::memory_order_relaxed);
    }

    // The latched set as a bitmask, republished whenever it changes. The
    // strum is laid out on the message thread as fingers move, and `latched`
    // is a vector the audio thread grows and erases - so the UI side reads
    // this instead of walking a container out from under its owner.
    std::array<std::atomic<uint32_t>, 4> latchedBits {};

    // What each lane is leaving ringing, published for the same reason: the
    // touch monitor draws it from the message thread.
    std::array<std::atomic<uint32_t>, 4> ringBits[kStrumLanes] {};

    int strumRingCount (int lane) const
    {
        if (lane < 0 || lane >= kStrumLanes) return 0;
        int n = 0;
        for (int w = 0; w < 4; ++w)
        {
            uint32_t bits = ringBits[lane][(size_t) w].load (std::memory_order_relaxed);
            for (; bits != 0u; bits &= bits - 1u) ++n;
        }
        return n;
    }

    bool strumRinging (int lane, int note) const
    {
        if (lane < 0 || lane >= kStrumLanes || note < 0 || note > 127) return false;
        return (ringBits[lane][(size_t) (note >> 5)].load (std::memory_order_relaxed)
                & (1u << (note & 31))) != 0u;
    }

    // The set a strum plays: the *latched* notes, sorted low to high and
    // repeated up the octaves. Reading the latched set rather than the
    // physically-held one is what makes this work with Hold and with the
    // arpeggiator for free - both of them already run off the same list.
    int strumChord (int* dest, int maxDest, int octaves) const
    {
        int base[32];
        int n = 0;
        for (int word = 0; word < 4 && n < 32; ++word)
        {
            uint32_t bits = latchedBits[(size_t) word].load (std::memory_order_relaxed);
            while (bits != 0 && n < 32)
            {
                const int bit = countTrailingZeros (bits);
                base[n++] = word * 32 + bit;
                bits &= ~(1u << bit);
            }
        }
        if (n == 0) return 0;     // already sorted: bits come out low to high

        int total = 0;
        for (int oct = 0; oct < std::max (1, octaves) && total < maxDest; ++oct)
            for (int i = 0; i < n && total < maxDest; ++i)
            {
                const int note = base[i] + 12 * oct;
                if (note <= 127) dest[total++] = note;
            }
        return total;
    }

    // patternTarget: the strokes are meant to clock the arp/sequencer rather
    // than play the chord themselves.
    void strumArm (bool on, bool patternTarget)
    {
        // The arpeggiator and a chord strum are both "play what is held", so
        // they cannot both run; the sequencer is an independent pattern, and
        // playing over it is the whole point of it, so it keeps its clock
        // unless the strum is explicitly there to drive it.
        const bool manual = on && (es.anyArp() || (patternTarget && seqPlay));
        if (gestureArmed == on && patternManual == manual) return;

        patternManual = manual;
        if (gestureArmed == on) return;
        gestureArmed = on;

        if (on)
        {
            // Palm on the strings. Whatever is ringing from the keyboard, the
            // arp or a latched chord is damped so the strum is the only voice
            // - the note *list* is left exactly as it was, because that list
            // is what the strum plays.
            stopAllArpNotes (echoBase);
            for (int n : latched) echo (n, 0.f, false, echoBase);
            for (auto& v : voices)
                if (v.currentOwner() == Voice::kOwnerLive && v.isHeld())
                    v.noteOff();
            monoStack[0].clear(); monoStack[1].clear();
        }
        else
        {
            // Coming off the key, the lanes let go - unless Hold is latching,
            // in which case what you strummed stays up exactly as the Hold
            // button would have kept a chord you played on the keyboard.
            if (! es.hold)
                for (int i = 0; i < kStrumLanes; ++i) strumLaneOff (i, true);
            ribbonTarget[0] = ribbonTarget[1] = 0.f;
        }
    }

    bool strumArmed() const { return gestureArmed; }

    // One string struck. model: 0 = CS-80, 1 = JP-8, -1 = follow engine mode.
    void strumNote (int lane, int note, float vel, int model)
    {
        if (lane < 0 || lane >= kStrumLanes) return;
        auto& held = strumHeld[(size_t) lane];
        const int owner = kOwnerStrum + lane;

        // Plucked: strike it and hand it straight to a timed release. It is
        // deliberately not entered in the lane's held list - the hand has
        // already moved on, and nothing about this note is still being held.
        if (es.strumPluck)
        {
            echo (note, vel, true, echoBase);
            triggerNote (note, vel, model, owner);
            addPluck (note, model, owner);
            lastVelocity = vel;
            return;
        }

        // Re-striking a string that is still ringing lets go of it first, so
        // strumming back and forth re-articulates instead of stacking a new
        // voice on the same note every pass.
        for (size_t i = 0; i < held.size(); ++i)
            if (held[i].note == note && held[i].model == model)
            {
                releaseNote (note, model, owner);
                held.erase (held.begin() + (long) i);
                break;
            }

        // Cap what one lane leaves ringing, oldest string first: a long
        // sweep across four octaves must not eat the whole voice pool, and a
        // hand damping behind itself is what actually happens anyway.
        while ((int) held.size() >= kMaxRing)
        {
            echo (held.front().note, 0.f, false, echoBase);
            releaseNote (held.front().note, held.front().model, owner);
            held.erase (held.begin());
        }

        echo (note, vel, true, echoBase);
        triggerNote (note, vel, model, owner);
        held.push_back ({ note, model });
        lastVelocity = vel;
        publishRing (lane);
    }

    // Finger lifted. damp = let go of this lane's strings; otherwise they
    // keep sounding until they are struck again or the chord changes.
    void strumLaneOff (int lane, bool damp)
    {
        if (lane < 0 || lane >= kStrumLanes || ! damp) return;
        auto& held = strumHeld[(size_t) lane];
        for (auto& h : held)
        {
            echo (h.note, 0.f, false, echoBase);
            releaseNote (h.note, h.model, kOwnerStrum + lane);
        }
        held.clear();
        publishRing (lane);
        if (patternManual) stopAllArpNotes (echoBase);
    }

    // One stroke of a hand-clocked pattern: the trackpad replaces the step
    // clock, so the arp or the sequencer advances exactly as far as the
    // finger moved, in the direction it moved.
    void strumPatternStep (int, int dir)
    {
        const int stepLen = arpStepLengthSamples();
        if (es.anyArp())
        {
            for (int m = 0; m < 2; ++m)
            {
                if (! es.arpOn[m]) continue;
                if (dir < 0) arp[m].step -= 2;      // walk the figure backwards
                advanceArpStep (m, stepLen, echoBase);
                arp[m].gateCounter = -1;            // holds until the next stroke
            }
        }
        else if (seqPlay)
        {
            if (dir < 0)
                for (auto& t : seq.tracks)
                {
                    t.playStep = wrap (t.playStep - 2,
                                       std::clamp (t.length, 1, SeqTrack::kSteps));
                    t.holdRemaining = 0;
                }
            advanceSeqStep (stepLen, echoBase);
            for (auto& t : seq.tracks) t.gateCounter = -1;
        }
    }

    // model < 0 bends both engines together.
    void setRibbon (int model, float semis)
    {
        if (model < 0) ribbonTarget[0] = ribbonTarget[1] = semis;
        else           ribbonTarget[model == modelJP8 ? 1 : 0] = semis;
    }

    // Called when arp is switched off mid-flight
    // One engine's arp was switched on or off. The other engine carries on
    // untouched - that separation is the whole feature.
    void arpModeChanged (int model, bool nowOn)
    {
        // The tracks start their next step fresh on whichever side of the
        // switch they now belong: a chord half-walked cannot carry on as a
        // block, and a block cannot become a figure mid-step.
        for (auto& t : seq.tracks)
        {
            stopTrack (t, echoBase);
            t.gateCounter = -1;
            t.arpPos = 0;
        }

        const int mask = 1 << model;
        arp[model] = ArpState();

        if (nowOn)
        {
            // This engine's share of what the keyboard is holding hands over
            // to the arpeggiator; the other engine keeps sounding it.
            for (int n : latched)
            {
                echo (n, 0.f, false, echoBase);
                releaseNote (n, -1, Voice::kOwnerLive, mask);
            }
            arpNotes.clear();
            if (! gestureArmed)
                for (int n : latched) addArpNote (n, lastVelocity);
        }
        else
        {
            stopArpNote (model, echoBase);
            if (! es.anyArp()) arpNotes.clear();

            // Hand the held notes back to this engine as ordinary voices -
            // unless the trackpad is armed, in which case nothing sounds
            // until it is strummed.
            if (gestureArmed) return;
            for (int n : latched)
                if (triggerNote (n, lastVelocity, -1, Voice::kOwnerLive, mask) > 0)
                    echo (n, lastVelocity, true, echoBase);
        }
    }

private:
    // The chord changed under a strum: anything a lane is ringing that is no
    // longer in the set gets let go, so lifting a key actually takes that
    // note out of the strum instead of leaving it hanging.
    void strumPruneLanes()
    {
        int set[128];
        const int n = strumChord (set, 128, 4);      // widest span on offer
        for (int lane = 0; lane < kStrumLanes; ++lane)
        {
            auto& held = strumHeld[(size_t) lane];
            for (int i = (int) held.size() - 1; i >= 0; --i)
            {
                if (std::find (set, set + n, held[(size_t) i].note) != set + n) continue;
                echo (held[(size_t) i].note, 0.f, false, echoBase);
                releaseNote (held[(size_t) i].note, held[(size_t) i].model,
                             kOwnerStrum + lane);
                held.erase (held.begin() + i);
            }
            publishRing (lane);
        }
    }

    void strumReleaseAll()
    {
        for (int i = 0; i < kStrumLanes; ++i) strumLaneOff (i, true);
    }

    // ---- plucked strings ------------------------------------------------
    // A note-off cannot follow a note-on immediately: the release segment
    // falls from wherever the envelope has got to, and at that instant it has
    // got nowhere - the note would be silent. So a plucked string is gated
    // just long enough for its amp envelope to reach the top, and released
    // there. Taking the gate from the patch's own attack rather than fixing
    // it means a slower-attacking sound still gets to open before it is let
    // go, and a fast one is let go almost at once.
    float pluckGateSeconds (int model) const
    {
        const float atk = model == modelJP8 ? vp.jp.aA : vp.aA;
        return std::clamp (atk * 1.15f + 0.008f, 0.01f, 0.6f);
    }

    void addPluck (int note, int model, int owner)
    {
        const int samples = std::max (1, (int) (pluckGateSeconds (model) * sr));

        // Struck again before its gate ran out: push the deadline back rather
        // than queue a second release. releaseNote lets go of every voice on
        // this note, so two pending releases would have the first one cut the
        // second string off a moment after it sounded.
        for (int i = 0; i < numPlucks; ++i)
            if (plucks[i].note == note && plucks[i].model == model
                && plucks[i].owner == owner)
                { plucks[i].samples = samples; return; }

        if (numPlucks >= kMaxPlucks)
        {
            // Full. Let one go now rather than drop a note-off on the floor
            // and leave a voice sounding for ever.
            echo (plucks[0].note, 0.f, false, echoBase);
            releaseNote (plucks[0].note, plucks[0].model, plucks[0].owner);
            plucks[0] = plucks[--numPlucks];
        }
        plucks[numPlucks++] = { note, model, owner, samples };
    }

    // osOffset counts oversampled samples into the current segment, as every
    // other clock in here does.
    void advancePlucks (int samples, int osOffset)
    {
        for (int i = numPlucks - 1; i >= 0; --i)
        {
            plucks[(size_t) i].samples -= samples;
            if (plucks[(size_t) i].samples > 0) continue;
            echo (plucks[(size_t) i].note, 0.f, false, echoPos (osOffset));
            releaseNote (plucks[(size_t) i].note, plucks[(size_t) i].model,
                         plucks[(size_t) i].owner);
            plucks[(size_t) i] = plucks[(size_t) --numPlucks];
        }
    }

    int nextPluckDeadline (int limit) const
    {
        for (int i = 0; i < numPlucks; ++i)
            limit = std::min (limit, std::max (1, plucks[(size_t) i].samples));
        return limit;
    }

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

    // modelMask picks which of the note's engines actually fire: bit 0 is the
    // CS-80 card, bit 1 the JP-8. The layer gain still counts *every* engine
    // the note would sound on, so arpeggiating one layer of a Layer patch
    // does not make the other one jump in level.
    // Returns how many engines actually sounded the note. A mask can select
    // engines this note does not route to - the JP-8's arp over a CS-80-only
    // patch, say - and then nothing sounds. Callers echo on the strength of
    // this rather than assuming, because a note-on echoed for a note that
    // never played would take the hosted layer's reference count with it.
    int triggerNote (int note, float vel, int forceModel = -1,
                     int owner = Voice::kOwnerLive, int modelMask = 3)
    {
        int models[2];
        const int layers = layersFor (note, models, forceModel);
        const float gain = layers == 2 ? 0.72f : 1.f;
        int sounded = 0;
        for (int li = 0; li < layers; ++li)
            if ((modelMask & (1 << models[li])) != 0)
            {
                triggerForModel (note, vel, models[li], gain, owner);
                ++sounded;
            }
        return sounded;
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

    void releaseNote (int note, int forceModel = -1, int owner = Voice::kOwnerLive,
                      int modelMask = 3)
    {
        int models[2];
        const int layers = layersFor (note, models, forceModel);
        bool done[2] = { false, false };
        for (int li = 0; li < layers; ++li)
        {
            if ((modelMask & (1 << models[li])) == 0) continue;
            releaseForModel (note, models[li], owner);
            done[models[li]] = true;
        }

        // The engine mode can change while a key is down (CS -> JP, Layer ->
        // one card, a repainted key zone), so the card this note-off routes
        // to is not always the one the note-on triggered. Anything still
        // sounding on the other card gets its own release pass - otherwise
        // that voice hangs until you happen to press the key again.
        for (int m = 0; m < 2; ++m)
            if ((modelMask & (1 << m)) != 0 && ! done[m] && stillSounding (note, m, owner))
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

        for (int i = 0; i < 2; ++i)
        {
            ribbonCurrent[i] += (ribbonTarget[i] - ribbonCurrent[i]) * bCoef;
            vp.ribbonSemis[i] = ribbonCurrent[i];
        }

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

        if (es.anyArp() || seqPlay)
            renderClocked (left, right, numSamples);
        else
            renderVoices (left, right, 0, numSamples);
    }

    // Note-echo timestamps are consumed by the host's MIDI stream, so they
    // have to come back out of the oversampled clock.
    int echoPos (int osOffset) const { return renderBase + osOffset / osFactor; }

    // Every render path goes through here, which is why the pluck clock lives
    // here too: the gate is a few milliseconds, so letting it round to a
    // block boundary would be most of its length.
    void renderVoices (float* left, float* right, int offset, int num)
    {
        int pos = 0;
        while (pos < num)
        {
            const int chunk = numPlucks > 0 ? nextPluckDeadline (num - pos) : num - pos;
            for (auto& v : voices)
                v.render (left + offset + pos, right + offset + pos, chunk,
                          lfoBuf.data() + offset + pos, pwmBuf.data() + offset + pos);
            if (numPlucks > 0) advancePlucks (chunk, offset + pos);
            pos += chunk;
        }
    }

    // ---------------- Arpeggiator -----------------------------------------
    struct ArpNote { int note; float vel; };

    void addArpNote (int note, float vel)
    {
        for (auto& a : arpNotes) if (a.note == note) return;
        if (arpNotes.empty())
            for (auto& a : arp)
            {
                a.step = 0;
                a.fireNow = true;   // first note of a phrase sounds immediately
            }
        arpNotes.push_back ({ note, vel });
    }

    void removeArpNote (int note)
    {
        for (size_t i = 0; i < arpNotes.size(); ++i)
            if (arpNotes[i].note == note) { arpNotes.erase (arpNotes.begin() + (long) i); break; }
        if (arpNotes.empty())
            for (auto& a : arp) { a.step = 0; a.fireNow = false; }
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

    // The arpeggiator's own step, which is the sequencer's step divided by
    // ARP x. At x1 - the default - this *is* the step clock, so nothing about
    // the arp on its own changes.
    int arpSubStepSamples() const
    {
        static const int mults[] = { 1, 2, 3, 4, 6, 8 };
        const int m = mults[std::clamp (es.arpMult, 0, 5)];
        return std::max (16, arpStepLengthSamples() / m);
    }

    // One event loop for both clocks, because they are no longer alternatives:
    // the sequencer can be laying down chords while the arpeggiator walks
    // them and your own playing arpeggiates over the top. Whichever clocks are
    // running contribute their next boundary, and the render is cut there.
    void renderClocked (float* left, float* right, int numSamples)
    {
        const int stepLen = arpStepLengthSamples();
        const int subLen  = arpSubStepSamples();
        const bool liveArp = ! patternManual;
        int pos = 0;

        while (pos < numSamples)
        {
            const int at = echoPos (pos);

            // ---- arpeggiator over what you are holding. Under a hand-clocked
            // strum it stands down: the pad is the clock, and strumPatternStep
            // is the only thing that advances a step.
            for (int m = 0; m < 2; ++m)
            {
                if (! es.arpOn[m]) continue;
                auto& A = arp[m];
                if (A.fireNow && ! arpNotes.empty() && ! patternManual)
                {
                    A.fireNow = false;
                    A.sampleCounter = 0;
                    advanceArpStep (m, subLen, at);
                }
                if (A.gateCounter == 0) { stopArpNote (m, at); A.gateCounter = -1; }
                if (liveArp && ! arpNotes.empty() && A.sampleCounter >= subLen)
                {
                    A.sampleCounter = 0;
                    advanceArpStep (m, subLen, at);
                }
            }

            // ---- sequencer
            if (seqPlay)
            {
                if (seq.fireNow && ! patternManual)
                {
                    seq.fireNow = false;
                    seq.sampleCounter = 0;
                    advanceSeqStep (stepLen, at);
                }
                for (auto& t : seq.tracks)
                {
                    if (t.gateCounter == 0)    { stopTrackBlock (t, at);   t.gateCounter = -1; }
                    if (t.arpGateCounter == 0) { stopTrackArpNote (t, at); t.arpGateCounter = -1; }
                }
                if (! patternManual && seq.sampleCounter >= stepLen)
                {
                    seq.sampleCounter = 0;
                    advanceSeqStep (stepLen, at);
                }
                // A track walking its chord fires on the arp's subdivision,
                // inside whatever step it is currently holding.
                for (auto& t : seq.tracks)
                    if (t.soundArped && t.arpCounter <= 0)
                        fireTrackArp (t, subLen, at);
            }

            // ---- render as far as the nearest upcoming boundary
            int chunk = numSamples - pos;
            for (int m = 0; m < 2; ++m)
            {
                if (! es.arpOn[m]) continue;
                if (liveArp && ! arpNotes.empty())
                    chunk = std::min (chunk, std::max (1, subLen - arp[m].sampleCounter));
                if (arp[m].gateCounter > 0) chunk = std::min (chunk, arp[m].gateCounter);
            }
            if (seqPlay)
            {
                if (! patternManual)
                    chunk = std::min (chunk, std::max (1, stepLen - seq.sampleCounter));
                for (auto& t : seq.tracks)
                {
                    if (t.gateCounter > 0)    chunk = std::min (chunk, t.gateCounter);
                    if (t.arpGateCounter > 0) chunk = std::min (chunk, t.arpGateCounter);
                    if (t.soundArped && t.arpCounter > 0)
                        chunk = std::min (chunk, t.arpCounter);
                }
            }

            renderVoices (left, right, pos, chunk);

            for (int m = 0; m < 2; ++m)
            {
                if (! es.arpOn[m]) continue;
                if (! arpNotes.empty()) arp[m].sampleCounter += chunk;
                if (arp[m].gateCounter > 0) arp[m].gateCounter -= chunk;
            }
            if (seqPlay)
            {
                seq.sampleCounter += chunk;
                for (auto& t : seq.tracks)
                {
                    if (t.gateCounter > 0)    t.gateCounter -= chunk;
                    if (t.arpGateCounter > 0) t.arpGateCounter -= chunk;
                    if (t.soundArped)         t.arpCounter -= chunk;
                }
            }
            pos += chunk;
        }
    }

    // One step of one engine's arpeggiator. `model` says whose - the figure
    // and the held notes are shared, the walk position and the sounding note
    // are not.
    void advanceArpStep (int model, int stepLen, int sampleOffset = 0)
    {
        auto& A = arp[model];
        stopArpNote (model, sampleOffset);
        if (arpNotes.empty()) return;

        // Build the pattern: held notes expanded across octaves
        pattern.clear();
        for (int oct = 0; oct < es.arpOctaves; ++oct)
            for (auto& a : arpNotes)
                pattern.push_back ({ a.note + 12 * oct, a.vel });

        const int n = (int) pattern.size();
        int idx = 0;
        // wrap(), not %, because a hand-clocked strum walks the step counter
        // *backwards* and C++ leaves a negative remainder negative.
        switch (es.arpMode)
        {
            case 0: // up
                std::sort (pattern.begin(), pattern.end(), byPitch);
                idx = wrap (A.step, n);
                ++A.step;
                break;
            case 1: // down
                std::sort (pattern.begin(), pattern.end(), byPitch);
                idx = (n - 1) - wrap (A.step, n);
                ++A.step;
                break;
            case 2: // up-down
            {
                std::sort (pattern.begin(), pattern.end(), byPitch);
                if (n == 1) { idx = 0; break; }
                int cycle = 2 * n - 2;
                int s = wrap (A.step, cycle);
                idx = s < n ? s : cycle - s;
                ++A.step;
                break;
            }
            case 3: // random
                idx = (int) (rng() % (uint32_t) n);
                break;
            case 4: // as played (sequencer)
            default:
                idx = wrap (A.step, n);
                ++A.step;
                break;
        }

        auto& sel = pattern[(size_t) idx];
        int note = std::min (127, sel.note);
        if (triggerNote (note, sel.vel, -1, Voice::kOwnerLive, 1 << model) > 0)
            echo (note, sel.vel, true, sampleOffset);
        A.sounding = note;
        A.gateCounter = std::max (16, (int) (es.arpGate * (float) stepLen));
    }

    void stopArpNote (int model, int sampleOffset = 0)
    {
        auto& A = arp[model];
        if (A.sounding < 0) return;

        echo (A.sounding, 0.f, false, sampleOffset);
        // Only this engine's card: the other one may be holding the same
        // note perfectly legitimately, either as a live layer or as its own
        // arpeggio a step behind.
        if (! isMonoish (cfgFor (model).mode))
            releaseForModel (A.sounding, model);
        else
        {
            // Blanket stop rather than the usual mono/legato/unison stack
            // fallback, so the arp always goes fully silent.
            monoStack[model].clear();
            if (Voice* v = monoVoice[model]) if (v->isHeld()) v->noteOff();
            for (auto* v : unisonVoices[model]) if (v != nullptr && v->isHeld()) v->noteOff();
        }
        A.sounding = -1;
    }

    void stopAllArpNotes (int sampleOffset = 0)
    { for (int m = 0; m < 2; ++m) stopArpNote (m, sampleOffset); }

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
    // Every track advances one step per tick (an empty step is a rest, not a
    // pause) and holds its own gate - tracks with different lengths drift
    // against each other on purpose. A step's chord is either struck as a
    // block or, with the arp on, walked note by note.
    //
    // The order a track's chord is walked in: the same modes the keyboard
    // arpeggiator offers, over the step's own notes and octaves. "As played"
    // means the order they were recorded into the step.
    struct ArpPick { int note = -1; float vel = 0.f; };

    ArpPick seqArpPick (const SeqStep& st, int pos)
    {
        const int n = std::min ((int) st.count, (int) SeqStep::kMaxNotes);
        if (n <= 0) return {};

        ArpPick items[SeqStep::kMaxNotes * 4];
        int total = 0;
        const int octs = std::clamp (es.arpOctaves, 1, 4);
        for (int o = 0; o < octs; ++o)
            for (int i = 0; i < n; ++i)
                items[total++] = { std::min (127, (int) st.note[i] + 12 * o),
                                   (float) st.vel[i] / 127.f };

        auto byNote = [] (const ArpPick& a, const ArpPick& b) { return a.note < b.note; };
        int idx = 0;
        switch (es.arpMode)
        {
            case 0: std::sort (items, items + total, byNote); idx = wrap (pos, total); break;
            case 1: std::sort (items, items + total, byNote);
                    idx = (total - 1) - wrap (pos, total); break;
            case 2:
            {
                std::sort (items, items + total, byNote);
                if (total == 1) { idx = 0; break; }
                const int cycle = 2 * total - 2;
                const int s = wrap (pos, cycle);
                idx = s < total ? s : cycle - s;
                break;
            }
            case 3: idx = (int) (rng() % (uint32_t) total); break;
            default: idx = wrap (pos, total); break;      // as played
        }
        return items[idx];
    }

    void fireTrackArp (SeqTrack& t, int subLen, int sampleOffset)
    {
        stopTrackArpNote (t, sampleOffset);
        const auto pick = seqArpPick (t.sounding, t.arpPos++);
        t.arpCounter = subLen;
        if (pick.note < 0) return;

        if (triggerNote (pick.note, pick.vel, t.soundModel, t.owner, t.arpModelMask) > 0)
            echo (pick.note, pick.vel, true, sampleOffset);
        t.arpNote = pick.note;
        t.arpGateCounter = std::max (16, (int) (es.arpGate * (float) subLen));
    }

    void stopTrackArpNote (SeqTrack& t, int sampleOffset)
    {
        if (t.arpNote < 0) return;
        echo (t.arpNote, 0.f, false, sampleOffset);
        releaseNote (t.arpNote, t.soundModel, t.owner, t.arpModelMask);
        t.arpNote = -1;
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
            t.arpGateCounter = -1;

            const auto& st = t.steps[(size_t) t.playStep];
            const uint8_t n = std::min (st.count, (uint8_t) SeqStep::kMaxNotes);
            const int hold = n > 0 ? std::clamp ((int) st.hold, 1, SeqStep::kMaxHold) : 1;
            t.holdRemaining = hold;        // counted even when muted, so the
            if (t.mute) continue;          // pattern keeps its shape

            const int forced = t.engine == 1 ? modelCS80 : t.engine == 2 ? modelJP8 : -1;
            t.sounding = st;
            t.soundModel = forced;
            t.owner = ti;
            // A figure needs more than one note in it. A single-note step -
            // a bass line, a kick pattern - is left exactly as it plays with
            // the arp off, held steps and all, so switching the arp on to
            // break up the chords does not also start chopping the bass.
            // Octaves count, so OCT > 1 does give a one-note step a figure.
            // A figure needs more than one note in it. A single-note step -
            // a bass line, a kick pattern - is left exactly as it plays with
            // the arp off, held steps and all, so switching the arp on to
            // break up the chords does not also start chopping the bass.
            // Octaves count, so OCT > 1 does give a one-note step a figure.
            const int figure = n * std::clamp (es.arpOctaves, 1, 4);

            // Split by engine, exactly as a live note is: the cards whose arp
            // is on walk the chord, the rest hold it. A forced track can only
            // reach its own card, so mask the arp down to it - otherwise the
            // JP-8's arp switch would arpeggiate a CS-80 track.
            int arpM = figure > 1 ? arpMask() : 0;
            // A forced track only reaches its own card, so both masks are
            // narrowed to it. Leaving the block mask at "everything else"
            // would have the track echo notes it never actually plays.
            const int reach = forced >= 0 ? (1 << forced) : 3;
            arpM &= reach;
            const int blockM = reach & ~arpM;

            t.soundArped = arpM != 0;
            t.arpModelMask = arpM;
            t.blockMask = blockM;

            if (t.soundArped)
            {
                // Hand the chord to this track's arpeggiator. The position
                // deliberately carries over from the last step rather than
                // resetting: at x1 there is only one note per step, so
                // restarting would play the same note of the chord every time
                // and never actually arpeggiate anything.
                t.arpCounter = 0;      // fires on this pass through the loop
            }

            if (blockM != 0)
                for (uint8_t i = 0; i < n; ++i)
                {
                    const float vel = (float) st.vel[i] / 127.f;
                    if (triggerNote (st.note[i], vel, forced, ti, blockM) > 0)
                        echo (st.note[i], vel, true, sampleOffset);
                }

            // Whole steps held, plus the gate fraction of the last one, so
            // Gate still shortens a held note without cutting it to one step.
            if (n > 0 && blockM != 0)
                t.gateCounter = std::max (16, (int) (((float) (hold - 1) + es.arpGate)
                                                     * (float) stepLen));
        }
    }

    // The block chord's gate ran out. Its notes are let go on the engines
    // that struck them, but `sounding` stays: a track arpeggiating the same
    // chord on the other card is still walking through it.
    void stopTrackBlock (SeqTrack& t, int sampleOffset)
    {
        if (t.blockMask == 0) return;
        const uint8_t n = std::min (t.sounding.count, (uint8_t) SeqStep::kMaxNotes);
        for (uint8_t i = 0; i < n; ++i)
        {
            echo (t.sounding.note[i], 0.f, false, sampleOffset);
            releaseNote (t.sounding.note[i], t.soundModel, t.owner, t.blockMask);
        }
        t.blockMask = 0;
    }

    void stopTrack (SeqTrack& t, int sampleOffset)
    {
        stopTrackBlock (t, sampleOffset);
        stopTrackArpNote (t, sampleOffset);
        t.sounding.clear();
        t.soundArped = false;
        t.arpModelMask = 0;
    }

    // The hosted synth layer is a third instrument, not a third engine: it
    // has no notion of CS versus JP. So it gets one note-on for a pitch
    // however many of our cards are sounding it, and its note-off only when
    // the last of them lets go - otherwise one engine's arpeggio would cut
    // short a note the other engine is still holding.
    void echo (int note, float vel, bool on, int offset)
    {
        if (note < 0 || note > 127) return;
        auto& refs = echoRef[(size_t) note];
        if (on)  { if (refs++ > 0) return; }
        else     { if (refs == 0) return; if (--refs > 0) return; }
        if (noteEcho) noteEcho (note, vel, on, offset);
    }

    // Everything the layer thinks is down, released. Used by panic, where
    // the reference counts are about to become meaningless.
    void echoFlush (int offset)
    {
        for (int n = 0; n < 128; ++n)
            if (echoRef[(size_t) n] > 0)
            {
                echoRef[(size_t) n] = 0;
                if (noteEcho) noteEcho (n, 0.f, false, offset);
            }
    }

    static bool byPitch (const ArpNote& a, const ArpNote& b) { return a.note < b.note; }

    static int wrap (int v, int n) { return n <= 0 ? 0 : ((v % n) + n) % n; }

    // ---------------- helpers ---------------------------------------------
    static void addUnique (std::vector<int>& v, int n)
    { if (! contains (v, n)) v.push_back (n); }
    static void removeVal (std::vector<int>& v, int n)
    { v.erase (std::remove (v.begin(), v.end(), n), v.end()); }
    static bool contains (const std::vector<int>& v, int n)
    { return std::find (v.begin(), v.end(), n) != v.end(); }

    void clearLatched()
    {
        strumReleaseAll();
        for (int n : latched)
        {
            if (es.anyArp()) removeArpNote (n);
            else { echo (n, 0.f, false, echoBase); releaseNote (n); }
        }
        latched.clear();
        publishLatched();
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

    // trackpad state. `patternManual` hands the arp/sequencer step clock to
    // the hand on the pad; the ribbon is smoothed alongside the pitch wheel
    // in renderSegment, because a finger reports at screen rates and a pitch
    // that moves in steps that size is audible as zipper.
    struct StrumHeld { int note; int model; };
    std::vector<StrumHeld> strumHeld[kStrumLanes];

    // Strings struck in Pluck mode, waiting out their gate. Fixed size, so
    // nothing here allocates on the audio thread.
    struct Pluck { int note, model, owner, samples; };
    static constexpr int kMaxPlucks = 32;
    Pluck plucks[kMaxPlucks] {};
    int numPlucks = 0;

    bool gestureArmed = false, patternManual = false;
    float ribbonTarget[2] = { 0.f, 0.f }, ribbonCurrent[2] = { 0.f, 0.f };

    int renderBase = 0;

    // Arp state, one per engine. The held-note set (arpNotes) is shared -
    // it is just what you are playing - but each engine walks its own
    // position through it and sounds its own note, which is what lets one
    // arpeggiate while the other holds.
    std::vector<ArpNote> arpNotes, pattern;
    uint8_t echoRef[128] {};        // note-echo reference counts, see echo()
    struct ArpState
    {
        int  step = 0;
        int  sampleCounter = 0;
        int  gateCounter = -1;
        int  sounding = -1;
        bool fireNow = false;
    };
    ArpState arp[2];
    std::mt19937 rng { 0x8080 };
};
} // namespace eighty
