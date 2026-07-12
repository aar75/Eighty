#pragma once
#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <functional>
#include <cstdint>
#include "Voice.h"
#include "LFO.h"

namespace eighty
{
// Voice management (poly / mono / legato / unison), hold latch,
// arpeggiator, global LFOs. Model-agnostic on top; the CS-80 voice card
// lives in Voice.h so a Jupiter-8 card can be added alongside it later.
class SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    enum class Mode { poly = 0, mono, legato, unison };

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
        bool arpSync = true;
        float arpRateHz = 5.f;
        int  arpDiv = 5;
        int  arpOctaves = 1;
        float arpGate = 0.6f;

        float lfoRate = 4.5f;
        int   lfoWave = 0;
        float lfoDelay = 0.f;
        float pwmRate = 0.6f;

        double bpm = 120.0;
    };

    VoiceParams vp;          // shared voice parameter snapshot
    EngineSettings es;

    // ---------------- Step sequencer --------------------------------------
    // 16 fixed steps, tempo-synced to the arp's rate/div/sync/gate (the two
    // are mutually exclusive - only one drives the engine at a time). REC
    // captures notes as they're played, committing a step (as a chord) when
    // all its notes are released; empty steps are rests. Owned here (rather
    // than in EngineSettings) because the engine mutates it directly -
    // recording auto-stops after the last step.
    struct SeqStep { std::vector<std::pair<int, float>> notes; };  // note, vel; empty = rest
    struct StepSeq
    {
        static constexpr int kSteps = 16;
        std::array<SeqStep, kSteps> steps;
        int recStep = 0;
        std::vector<std::pair<int, float>> recChord;   // notes played for the step in progress
        int recHeldCount = 0;                           // how many are still physically held
        int playStep = -1;
        std::vector<std::pair<int, float>> sounding;    // notes currently sounding during playback
        int sampleCounter = 0, gateCounter = -1;
        bool fireNow = false;
    } seq;
    bool seqRec = false, seqPlay = false;

    // Called by the processor on the rising edge of the REC/PLAY toggles.
    void seqRecStart()
    {
        seq.steps.fill (SeqStep {});
        seq.recStep = 0;
        seq.recChord.clear();
        seq.recHeldCount = 0;
        seqRec = true;
    }

    void seqPlayStart()
    {
        seq.playStep = -1;
        seq.sampleCounter = 0;
        seq.gateCounter = -1;
        seq.fireNow = true;
        seqPlay = true;
    }

    void seqPlayStop()
    {
        stopSeqStep (echoBase);
        seq.playStep = -1;
        seq.gateCounter = -1;
        seq.fireNow = false;
        seqPlay = false;
    }

    // Echoes the engine's *effective* note stream (post hold-latch, post
    // arpeggiator) so a hosted synth layer can follow it. sampleOffset is
    // relative to the start of the current audio block.
    std::function<void (int note, float vel, bool on, int sampleOffset)> noteEcho;
    int echoBase = 0;        // set by the processor before midi-event calls

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        lfoBuf.resize ((size_t) maxBlockSize);
        pwmBuf.resize ((size_t) maxBlockSize);
        lfo.prepare (sampleRate);
        pwmLfo.prepare (sampleRate);
        for (int i = 0; i < kMaxVoices; ++i)
            voices[i].prepare (sampleRate, i, &vp);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v.kill();
        physicalHeld.clear();
        latched.clear();
        arpNotes.clear();
        monoStack[0].clear(); monoStack[1].clear();
        arpStep = 0; arpDir = 1; arpSampleCounter = 0; arpGateCounter = -1;
        arpSounding = -1;
        lfoEnvSeconds = 1000.f;

        seq.playStep = -1; seq.sampleCounter = 0; seq.gateCounter = -1; seq.fireNow = false;
        seq.sounding.clear();
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
        if (seqPlay)
            return;                              // sequencer playback owns the engine
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
        if (seqPlay) return;
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
        stopSeqStep (echoBase);
        seq.playStep = -1; seq.gateCounter = -1; seq.fireNow = false;
        for (auto& v : voices) v.release();
    }

    float currentBend() const { return bendCurrent; }
    int activeVoiceCount() const
    {
        int n = 0;
        for (auto& v : voices) if (v.wasActive()) ++n;
        return n;
    }

    // ---------------- Render ---------------------------------------------
    void render (float* left, float* right, int numSamples, int blockOffset = 0)
    {
        if (numSamples <= 0) return;
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
    // dispatches through its own independent Poly/Mono/Legato/Unison mode.
    void triggerNote (int note, float vel)
    {
        int models[2];
        const int layers = modelsForNote (note, models);
        const float gain = layers == 2 ? 0.72f : 1.f;
        for (int li = 0; li < layers; ++li)
            triggerForModel (note, vel, models[li], gain);
    }

    void triggerForModel (int note, float vel, int model, float gain)
    {
        auto& cfg = cfgFor (model);
        switch (cfg.mode)
        {
            case Mode::poly:   triggerPolyModel   (note, vel, model, gain); break;
            case Mode::mono:   triggerMonoModel   (note, vel, model, gain, true); break;
            case Mode::legato: triggerMonoModel   (note, vel, model, gain, monoStack[model].empty()); break;
            case Mode::unison: triggerUnisonModel (note, vel, model, gain, true); break;
        }
        if (cfg.mode != Mode::poly)
            addUnique (monoStack[model], note);
        lastTriggeredNote[model] = note;
    }

    void releaseNote (int note)
    {
        int models[2];
        const int layers = modelsForNote (note, models);
        for (int li = 0; li < layers; ++li)
            releaseForModel (note, models[li]);
    }

    void releaseForModel (int note, int model)
    {
        auto& cfg = cfgFor (model);
        if (cfg.mode == Mode::poly)
        {
            for (auto& v : voices)
                if (v.currentNote() == note && v.currentModel() == model && v.isHeld())
                {
                    if (sustainPedal) v.setSustained (true);
                    v.noteOff();
                }
            return;
        }

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
                if (cfg.mode == Mode::mono)        triggerMonoModel   (back, lastVelocity, model, gain, true);
                else if (cfg.mode == Mode::legato) triggerMonoModel   (back, lastVelocity, model, gain, false);
                else                                triggerUnisonModel (back, lastVelocity, model, gain, false);
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

    void triggerPolyModel (int note, float vel, int model, float gain)
    {
        float glideFrom = glideSource (model, false);
        Voice* v = allocateVoice (model);
        v->noteOn (note, vel, 0.f, 0.f, glideFrom, true, gain, model);
        if (sustainPedal) v->setSustained (true);
    }

    void triggerMonoModel (int note, float vel, int model, float gain, bool retrig)
    {
        Voice*& v = monoVoice[model];
        if (v == nullptr) v = allocateVoice (model);
        float glideFrom = glideSource (model, v->wasActive());
        if (! retrig && v->wasActive())
            v->changeNote (note);
        else
            v->noteOn (note, vel, 0.f, 0.f, glideFrom, retrig || ! v->wasActive(), gain, model);
        if (sustainPedal) v->setSustained (true);
    }

    void triggerUnisonModel (int note, float vel, int model, float noteGain, bool retrig)
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
            if (v == nullptr) v = allocateVoice (model);
            if (! retrig && v->wasActive())
                v->changeNote (note);
            else
                v->noteOn (note, vel, pos * maxDet, pos * 0.8f, glideFrom,
                           retrig || ! v->wasActive(), gain, model);
            if (sustainPedal) v->setSustained (true);
        }
    }

    float glideSource (int model, bool voiceActive) const
    {
        const int last = lastTriggeredNote[model];
        if (vp.glideMode == 2 && last >= 0)   // always
            return (float) last;
        if (vp.glideMode == 1)                // legato only
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
    Voice* allocateVoice (int model)
    {
        const int cap = std::min (cfgFor (model).polyVoices, kMaxVoices);
        int activeForModel = 0;
        for (auto& v : voices)
            if (v.wasActive() && v.currentModel() == model) ++activeForModel;

        if (activeForModel < cap)
            for (auto& v : voices)
                if (! v.wasActive()) return &v;

        // At/over budget, or no free voice at all: steal this model's own
        // quietest-releasing voice first, then its oldest.
        Voice* best = nullptr; float bestLevel = 1e9f;
        for (auto& v : voices)
            if (v.currentModel() == model && v.isReleasing() && v.envLevel() < bestLevel)
                { best = &v; bestLevel = v.envLevel(); }
        if (best) return best;

        uint32_t oldest = 0xFFFFFFFFu; Voice* pick = nullptr;
        for (auto& v : voices)
            if (v.currentModel() == model && v.age() < oldest) { oldest = v.age(); pick = &v; }
        if (pick) return pick;

        // This model has no voices of its own to steal from (the other
        // engine currently holds all of them) - fall back to the globally
        // oldest voice so we always return something playable.
        oldest = 0xFFFFFFFFu; pick = &voices[0];
        for (auto& v : voices)
            if (v.age() < oldest) { oldest = v.age(); pick = &v; }
        return pick;
    }

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

    int arpStepLengthSamples() const
    {
        if (es.arpSync && es.bpm > 1.0)
        {
            static const double beats[] = { 4.0, 2.0, 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0, 0.125 };
            double b = beats[std::max (0, std::min (7, es.arpDiv))];
            return std::max (32, (int) (b * 60.0 / es.bpm * sr));
        }
        return std::max (32, (int) (sr / std::fmax (0.1f, es.arpRateHz)));
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
                advanceArpStep (stepLen, renderBase + pos);
            }
            if (arpGateCounter == 0) { stopArpNote (renderBase + pos); arpGateCounter = -1; }
            if (! arpNotes.empty() && arpSampleCounter >= stepLen)
            {
                arpSampleCounter = 0;
                advanceArpStep (stepLen, renderBase + pos);
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
                if (cfgFor (model).mode == Mode::poly)
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
    void seqRecNoteOn (int note, float vel)
    {
        for (auto& p : seq.recChord) if (p.first == note) return;
        seq.recChord.push_back ({ note, vel });
        ++seq.recHeldCount;
    }

    void seqRecNoteOff (int note)
    {
        bool wasHeld = false;
        for (auto& p : seq.recChord) if (p.first == note) { wasHeld = true; break; }
        if (! wasHeld) return;
        --seq.recHeldCount;
        if (seq.recHeldCount <= 0 && ! seq.recChord.empty())
        {
            seq.steps[(size_t) seq.recStep].notes = seq.recChord;
            seq.recChord.clear();
            seq.recHeldCount = 0;
            if (++seq.recStep >= StepSeq::kSteps)
            {
                seq.recStep = 0;
                seqRec = false;
            }
        }
    }

    // ---------------- Step sequencer: playback ------------------------------
    // Mirrors renderWithArp/advanceArpStep/stopArpNote, but always advances
    // one step per tick (an empty step is a rest, not a pause) and fires
    // every note in a step's chord simultaneously.
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
                advanceSeqStep (stepLen, renderBase + pos);
            }
            if (seq.gateCounter == 0) { stopSeqStep (renderBase + pos); seq.gateCounter = -1; }
            if (seq.sampleCounter >= stepLen)
            {
                seq.sampleCounter = 0;
                advanceSeqStep (stepLen, renderBase + pos);
            }

            int chunk = std::min (numSamples - pos, std::max (1, stepLen - seq.sampleCounter));
            if (seq.gateCounter > 0) chunk = std::min (chunk, seq.gateCounter);

            renderVoices (left, right, pos, chunk);
            seq.sampleCounter += chunk;
            if (seq.gateCounter > 0) seq.gateCounter -= chunk;
            pos += chunk;
        }
    }

    void advanceSeqStep (int stepLen, int sampleOffset)
    {
        stopSeqStep (sampleOffset);
        seq.playStep = (seq.playStep + 1) % StepSeq::kSteps;
        auto& st = seq.steps[(size_t) seq.playStep];
        for (auto& n : st.notes)
        {
            echo (n.first, n.second, true, sampleOffset);
            triggerNote (n.first, n.second);
        }
        seq.sounding = st.notes;
        seq.gateCounter = st.notes.empty() ? -1 : std::max (16, (int) (es.arpGate * (float) stepLen));
    }

    void stopSeqStep (int sampleOffset)
    {
        for (auto& n : seq.sounding)
        {
            echo (n.first, 0.f, false, sampleOffset);
            releaseNote (n.first);
        }
        seq.sounding.clear();
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

    double sr = 44100.0;
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
