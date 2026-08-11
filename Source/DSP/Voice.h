#pragma once
#include <cmath>
#include <cstdint>
#include "Oscillator.h"
#include "CS80Filter.h"
#include "JP8Filter.h"
#include "Envelope.h"

namespace eighty
{
// Parameter snapshot the engine refreshes once per block from the APVTS.
// Voices read from this; no atomics needed on the audio thread.
struct VoiceParams
{
    struct OscChannel
    {
        bool  on = true;
        float footSemis = 0.f;   // -24 / -12 / 0 / +12
        float semi = 0.f;        // osc II only
        float fine = 0.f;        // cents
        float saw = 0.8f, pulse = 0.f, pw = 0.5f, pwmDepth = 0.f;
        float level = 0.9f;
    };
    OscChannel osc[2];
    float noiseLevel = 0.f;

    // Sub-oscillator, one config per engine (indexed by VoiceModel). Tracks
    // the engine's first oscillator, one or two octaves down.
    struct SubCfg
    {
        float level = 0.f;
        int   octaves = 1;         // 1 or 2 octaves below
        int   wave = 0;            // 0 square, 1 triangle
    };
    SubCfg sub[2];

    float hpfCutoff = 20.f, lpfCutoff = 9000.f, resonance = 0.15f;
    float hpfRes = 0.f;            // CS-80 high-pass resonance (the real one has it)
    float filterEnvAmt = 0.f, keyTrack = 0.5f, filterDrive = 0.2f;
    float sineLevel = 0.f;         // CS-80 sine, mixed *after* the filters

    float fA = 0.005f, fD = 0.35f, fS = 0.4f, fR = 0.3f;
    float fIL = 0.f, fAL = 1.f;    // CS-80 filter EG initial / attack level
    float aA = 0.004f, aD = 0.4f,  aS = 0.8f, aR = 0.35f;
    float velToAmp = 0.5f, velToFilter = 0.3f;

    // CS-80 channel II. The real machine is two complete 8-voice synths
    // sharing a chassis - each channel has its own filter, VCA and envelope
    // generators, and the two layers moving independently is what makes its
    // brass and strings work. Rather than duplicate the whole panel, channel
    // II runs the same settings *offset*: same controls, its own filter and
    // its own pair of envelope generators running at its own speed.
    struct Ch2Cfg
    {
        float cutoffOct = 0.f;     // octaves, -3 .. +3
        float res = 0.f;           // -1 .. +1 added to resonance
        float envAmt = 0.f;        // -1 .. +1 added to filter env amount
        float timeScale = 1.f;     // 0.25 .. 4, scales both of its envelopes
    } ch2;

    // Ring modulator (CS-80). A sine carrier with its own attack/decay
    // envelope, where the envelope also drives the carrier's *speed* - the
    // detail that makes it sound like the original rather than a multiplier.
    // Deliberately not keyboard-tracked: that is where the inharmonic,
    // metallic character comes from.
    struct RingCfg
    {
        bool  on = false;
        float depth = 0.5f;
        float rateHz = 220.f;
        float envAmt = 0.f;        // -1 .. +1, envelope -> carrier speed
        float attack = 0.01f, decay = 0.5f;
    } ring;

    float lfoToPitch = 0.f, lfoToFilter = 0.f, lfoToAmp = 0.f;

    float touchRise = 0.6f, touchToVib = 0.25f, touchToBright = 0.2f, touchToLevel = 0.15f;

    float drift = 0.35f;
    float stereoSpread = 0.6f;
    float csGain = 1.f, jpGain = 1.f;   // CS/JP mix (Split & Layer modes)

    // Global performance macros, the CS-80's two most-used front-panel
    // sliders: one offset applied to every filter cutoff in the instrument,
    // one to every resonance. They sit on top of whatever the patch says.
    float brilliance = 0.f;        // -1 .. +1, scaled to +/- 3 octaves
    float resOffset = 0.f;         // -1 .. +1

    // Touch Response initial pitch bend: striking a key hard slides up into
    // the note from just below it, the way a brass player's embouchure does.
    float velBendSemis = 0.f;      // 0 .. 2 semitones at full velocity

    // Portamento is per-engine: the two voice cards glide independently,
    // so a JP-8 lead can slide while the CS-80 pad underneath does not.
    struct GlideCfg
    {
        float time = 0.05f;
        int   mode = 0;            // 0 off, 1 legato, 2 always
    };
    GlideCfg glide[2];             // indexed by VoiceModel

    float masterTuneCents = 0.f;

    // Per-block modulation state computed by the engine:
    float bendSemis = 0.f;         // smoothed pitch bend, in semitones
    float modWheel = 0.f;          // 0..1, adds vibrato
    float channelPressure = 0.f;   // 0..1

    // ---- Jupiter-8 voice card parameters ----
    struct JP
    {
        // waves: 0 tri, 1 saw, 2 pulse, 3 square (VCO1) / 3 noise (VCO2)
        int   vco1Wave = 1;
        float vco1FootSemis = 0.f;     // 16'/8'/4'/2' -> -12/0/+12/+24
        float pw = 0.5f, pwmDepth = 0.f;
        float mix = 0.5f;              // 0 = VCO1 only, 1 = VCO2 only
        int   vco2Wave = 1;
        float vco2FootSemis = 0.f;
        float semi = 0.f, fine = 5.f;
        bool  vco2Low = false;         // VCO2 drops out of key tracking and runs at LF
        bool  sync = false;            // VCO2 hard-synced to VCO1
        float xmod = 0.f;              // VCO2 -> VCO1 frequency cross-mod
        float hpf = 20.f, lpf = 9000.f, res = 0.15f, drive = 0.15f;
        bool  slope24 = true;
        float envAmt = 0.f, keyTrack = 0.5f;
        bool  envInv = false;          // ENV-1 polarity switch
        float fA = 0.005f, fD = 0.35f, fS = 0.4f, fR = 0.3f;
        float aA = 0.004f, aD = 0.4f,  aS = 0.8f, aR = 0.35f;
    } jp;
};

enum VoiceModel { modelCS80 = 0, modelJP8 = 1 };

// One voice card. In CS-80 mode it is two complete channels - two VCO
// channels, each into its own HPF->LPF pair and its own filter + amp
// envelope generators - plus noise, a sub-oscillator and a sine that
// bypasses the filters, then a shared ring modulator. In JP-8 mode it is a
// two-VCO card into the ladder filter.
//
// Each card gets fixed component-tolerance offsets seeded by its index,
// scaled at runtime by the Drift parameter - like eight slightly different
// boards in one chassis. The two oscillators in a card drift *independently*:
// making them share one offset kept them perfectly beat-locked, which is the
// one thing real analog never does.
class Voice
{
public:
    static constexpr int kCtrl = 16; // control-rate interval in samples

    void prepare (double sampleRate, int voiceIndex, const VoiceParams* sharedParams)
    {
        sr = (float) sampleRate;
        params = sharedParams;
        filter[0].prepare (sampleRate);
        filter[1].prepare (sampleRate);
        jpFilter.prepare (sampleRate);
        fEnv.prepare (sampleRate);
        aEnv.prepare (sampleRate);
        fEnv2.prepare (sampleRate);
        aEnv2.prepare (sampleRate);
        ringEnv.prepare (sampleRate);
        noise.seed (0xBADA55u + (uint32_t) voiceIndex * 7919u);

        // Fixed per-card tolerances (stable across runs)
        uint32_t s = 0x1234ABCDu + (uint32_t) voiceIndex * 2654435761u;
        auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5;
                            return (float) (int32_t) s * 4.6566129e-10f; };
        tolPitchCents[0] = rnd() * 6.f;
        tolPitchCents[1] = rnd() * 6.f;
        tolCutoffOct  = rnd() * 0.25f;
        tolEnvScale   = rnd() * 0.12f;
        tolPwOffset   = rnd() * 0.03f;
        tolLevel      = rnd() * 0.08f;
        tolPan        = rnd() * 0.2f;
        cardSide      = (voiceIndex % 2 == 0) ? -1.f : 1.f;
    }

    // Only the sample rate changes when the oversampling factor changes -
    // no allocation, so this is safe to call from the audio thread.
    void setSampleRate (double sampleRate)
    {
        sr = (float) sampleRate;
        filter[0].prepare (sampleRate);
        filter[1].prepare (sampleRate);
        jpFilter.prepare (sampleRate);
        fEnv.prepare (sampleRate);
        aEnv.prepare (sampleRate);
        fEnv2.prepare (sampleRate);
        aEnv2.prepare (sampleRate);
        ringEnv.prepare (sampleRate);
    }

    // owner: who triggered this voice - kOwnerLive for the keyboard, or a
    // sequencer track index. The engine only ever releases voices belonging
    // to the same owner, so a sequencer step ending cannot cut off a live
    // note you happen to be holding at the same pitch.
    static constexpr int kOwnerLive = -1;

    void noteOn (int midiNote, float vel, float detuneCents, float panOffset,
                 float glideFromNote, bool retrigger, float gainScale = 1.f,
                 int voiceModel = modelCS80, int voiceOwner = kOwnerLive)
    {
        owner = voiceOwner;
        note = midiNote;
        velocity = vel;
        unisonDetune = detuneCents;
        unisonPan = panOffset;
        noteGain = gainScale;
        model = voiceModel;
        held = true;
        sustained = false;
        heldSeconds = 0.f;

        targetNoteF = (float) midiNote;
        if (glideFromNote >= 0.f)
            currentNoteF = glideFromNote;
        else if (! wasActive())
            currentNoteF = targetNoteF;

        // Touch Response initial pitch bend. Kept separate from portamento
        // so it resolves at its own fast rate whatever Glide is set to.
        velBend = -params->velBendSemis * velocity;

        if (! wasActive())
            smoothInit = true;      // snap the control smoothers, don't sweep in

        if (retrigger || ! aEnv.isActive())
        {
            const float ts = 1.f + tolEnvScale * params->drift;
            if (model == modelJP8)
            {
                const auto& J = params->jp;
                fEnv.setShape (0.f, 1.f);
                fEnv.setParams (J.fA, J.fD, J.fS, J.fR, ts);
                aEnv.setParams (J.aA, J.aD, J.aS, J.aR, ts);
                fEnv2.kill();
                aEnv2.kill();
            }
            else
            {
                const auto& P = *params;
                fEnv.setShape (P.fIL, P.fAL);
                fEnv.setParams (P.fA, P.fD, P.fS, P.fR, ts);
                aEnv.setParams (P.aA, P.aD, P.aS, P.aR, ts);
                // Channel II: same settings, its own generators, its own speed
                const float ts2 = ts * P.ch2.timeScale;
                fEnv2.setShape (P.fIL, P.fAL);
                fEnv2.setParams (P.fA, P.fD, P.fS, P.fR, ts2);
                aEnv2.setParams (P.aA, P.aD, P.aS, P.aR, ts2);
                fEnv2.noteOn();
                aEnv2.noteOn();
            }
            fEnv.noteOn();
            aEnv.noteOn();

            if (params->ring.on)
            {
                ringEnv.setParams (params->ring.attack, params->ring.decay);
                ringEnv.noteOn();
            }
            else
                ringEnv.kill();

            if (! wasActive())
            {
                vco[0].reset (0.f);
                vco[1].reset (0.37f); // free-running feel: channels never phase-locked
                subVco.reset (0.11f);
                sineOsc.reset (0.f);
                filter[0].reset();
                filter[1].reset();
                jpFilter.reset();
            }
        }
        steal = false;
        ++serial;
    }

    // Re-target pitch without retriggering (legato)
    void changeNote (int midiNote)
    {
        note = midiNote;
        targetNoteF = (float) midiNote;
        held = true;
    }

    void noteOff()               { held = false; if (! sustained) release(); }
    // Unconditional: used when the engine retires a surplus stack voice,
    // which must fade out even with the sustain pedal down.
    void releaseNow()            { held = false; sustained = false; release(); }
    void setSustained (bool s)   { sustained = s; if (! s && ! held) release(); }
    void release()               { fEnv.noteOff(); aEnv.noteOff();
                                   fEnv2.noteOff(); aEnv2.noteOff(); }
    void kill()                  { fEnv.kill(); aEnv.kill(); fEnv2.kill(); aEnv2.kill();
                                   ringEnv.kill(); held = false; sustained = false; }

    // Channel II can outlast channel I when its time scale is stretched, so
    // liveness is the union of the two amp envelopes.
    bool wasActive() const       { return aEnv.isActive() || aEnv2.isActive(); }
    bool isHeld() const          { return held; }
    bool isReleasing() const     { return aEnv.isReleasing(); }
    int  currentNote() const     { return note; }
    int  currentModel() const    { return model; }
    int  currentOwner() const    { return owner; }
    float envLevel() const       { return std::fmax (aEnv.value(), aEnv2.value()); }
    uint32_t age() const         { return serial; }
    void setPolyPressure (float p) { polyPressure = p; }

    // lfoBuf / pwmBuf: one value per sample for this block segment
    void render (float* left, float* right, int numSamples,
                 const float* lfoBuf, const float* pwmBuf)
    {
        if (! wasActive())
            return;

        const VoiceParams& P = *params;
        int i = 0;
        while (i < numSamples)
        {
            const int n = (numSamples - i) < kCtrl ? (numSamples - i) : kCtrl;
            updateControl (P, lfoBuf[i]);

            for (int k = 0; k < n; ++k)
            {
                const int idx = i + k;
                float x = 0.f;
                float amp;

                if (model == modelCS80)
                {
                    // PWM from the dedicated PWM LFO (per channel depth)
                    float s0 = 0.f, p0 = 0.f, s1 = 0.f, p1 = 0.f;
                    float pwA = clampf (pwEff[0] + pwmDepth[0] * 0.45f * pwmBuf[idx], 0.03f, 0.97f);
                    float pwB = clampf (pwEff[1] + pwmDepth[1] * 0.45f * pwmBuf[idx], 0.03f, 0.97f);
                    vco[0].tick (pwA, s0, p0);
                    vco[1].tick (pwB, s1, p1);

                    // Channel I carries the noise and the sub; channel II is
                    // just its own oscillator. Each runs its own filter.
                    float chA = s0 * sawG[0] + p0 * pulseG[0];
                    chA += noiseG > 0.0001f ? noise.tick() * noiseG * 0.7f : 0.f;
                    chA += tickSub();
                    float chB = s1 * sawG[1] + p1 * pulseG[1];

                    chA = filter[0].process (chA);
                    chB = filter[1].process (chB);

                    fEnv.tick();
                    fEnv2.tick();
                    const float a1 = aEnv.tick();
                    const float a2 = aEnv2.tick();

                    // The sine bypasses the filters entirely and joins at the
                    // VCA mixer, as on the original.
                    const float sine = sineG > 0.0001f ? sineOsc.tick() * sineG : 0.f;

                    x = (chA + sine) * a1 + chB * a2;
                    amp = ampScale;
                }
                else
                {
                    const auto& J = P.jp;
                    const float pw = clampf (pwEff[0] + pwmDepth[0] * 0.45f * pwmBuf[idx], 0.03f, 0.97f);

                    // Cross-mod in the exponential domain: a linear multiplier
                    // goes negative at depth and runs the oscillator backwards.
                    if (J.xmod > 0.001f)
                        vco[0].setFrequency (vcoHz[0] * std::exp2 (J.xmod * 3.f * lastVco2), sr);

                    float s0, p0, s1, p1;
                    vco[0].tick (J.vco1Wave == 2 ? pw : 0.5f, s0, p0);
                    if (J.sync && vco[0].justWrapped())
                        vco[1].syncTo (vco[0].wrapFraction(), J.vco2Wave == 2 ? pw : 0.5f);
                    vco[1].tick (J.vco2Wave == 2 ? pw : 0.5f, s1, p1);

                    const float o1 = J.vco1Wave == 0 ? vco[0].triangle()
                                   : J.vco1Wave == 1 ? s0 : p0;   // pulse & square via p0
                    const float o2 = J.vco2Wave == 0 ? vco[1].triangle()
                                   : J.vco2Wave == 1 ? s1
                                   : J.vco2Wave == 2 ? p1 : noise.tick();
                    lastVco2 = o2;

                    x = (o1 * jpMixA + o2 * jpMixB) * 1.2f;
                    x += tickSub();
                    x = jpFilter.process (x);

                    fEnv.tick();
                    amp = aEnv.tick() * ampScale;
                }

                // Ring modulator: a sine carrier whose speed is pushed by its
                // own AD envelope. Sits after the filters, before the output
                // stage, and applies to whichever card this is.
                if (ringOn)
                {
                    const float re = ringEnv.tick();
                    ringOsc.setFrequency (ringHz * std::exp2 (ringEnvAmt * 2.f * re), sr);
                    const float c = ringOsc.tick();
                    x *= (1.f - ringDepth) + ringDepth * c;
                }

                x *= amp;

                // global LFO tremolo routing (bipolar -> unipolar dip)
                x *= 1.f - P.lfoToAmp * 0.5f * (1.f + lfoBuf[idx]) * 0.5f;

                const float o = x * 0.8f; // headroom
                left[idx]  += o * panL;
                right[idx] += o * panR;
            }
            i += n;
        }
    }

private:
    static float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

    // Sub-oscillator: square (from the pulse output at 50%) or triangle,
    // an octave or two under the engine's first oscillator. Skipped entirely
    // when the level is down so it costs nothing in patches that don't use it.
    float tickSub()
    {
        if (subG <= 0.0001f) return 0.f;
        float saw, pulse;
        subVco.tick (0.5f, saw, pulse);
        const float w = subWave == 1 ? subVco.triangle() : pulse;
        return w * subG * 0.8f;
    }

    // One-pole toward a target, run once per control block (every kCtrl
    // samples). This is what keeps a parameter *change* - dragging a fader
    // or arrow-keying it - from arriving as an audible step: the block-rate
    // snapshot in VoiceParams is the target, never the value we use.
    void smoothTo (float& cur, float target, float coef) const
    {
        if (smoothInit) cur = target;
        else            cur += (target - cur) * coef;
    }

    // oscSemis: the pitch of the engine's first oscillator, so the sub
    // follows the patch's octave rather than the raw note.
    void updateSub (const VoiceParams& P, float oscSemis, float smCoef)
    {
        const auto& S = P.sub[model == modelJP8 ? 1 : 0];
        subWave = S.wave;
        smoothTo (subG, S.level, smCoef);
        const float semis = oscSemis - 12.f * (float) (S.octaves < 2 ? 1 : 2);
        subVco.setFrequency (440.f * std::exp2 ((semis - 69.f) / 12.f), sr);
    }

    void updateControl (const VoiceParams& P, float lfoVal)
    {
        const float ctrlDt = (float) kCtrl / sr;
        const float smCoef = 1.f - std::exp (-ctrlDt / 0.02f);   // ~20 ms
        heldSeconds += held ? ctrlDt : 0.f;

        // --- Touch pressure: simulated ramp while held, or real AT if higher
        float simP = held ? (1.f - std::exp (-heldSeconds / std::fmax (0.05f, P.touchRise))) : pressure;
        float extP = std::fmax (P.channelPressure, polyPressure);
        pressure = std::fmax (simP * (held ? 1.f : 0.f), extP);

        // --- Slow per-oscillator pitch wander. Two independent random walks,
        // so the pair beats against itself the way two real VCOs do.
        for (int c = 0; c < 2; ++c)
        {
            driftWalk[c] += (noise.tick() - driftWalk[c] * 0.02f) * 0.01f;
            driftCents[c] = (tolPitchCents[c] + driftWalk[c] * 40.f) * P.drift;
        }

        // --- Glide (per-engine setting; this card follows its own model)
        const auto& G = P.glide[model == modelJP8 ? 1 : 0];
        if (G.mode == 0 || G.time <= 0.001f)
            currentNoteF = targetNoteF;
        else
        {
            const float coef = 1.f - std::exp (-ctrlDt * 6.f / std::fmax (0.01f, G.time));
            currentNoteF += (targetNoteF - currentNoteF) * coef;
        }

        // --- Touch Response initial pitch bend, resolving at a fixed fast
        // rate independent of Glide (~45 ms).
        velBend -= velBend * (1.f - std::exp (-ctrlDt / 0.045f));

        // --- Vibrato: global LFO routing + touch vibrato + mod wheel.
        // Touch/mod-wheel vibrato rides the same LFO, so it must be gated
        // by lfoToPitch too - otherwise LFO>Pitch=0 doesn't mean silent.
        const float vibCents = lfoVal * P.lfoToPitch * (P.lfoToPitch * 200.f
                                         + pressure * P.touchToVib * 60.f
                                         + P.modWheel * 60.f);

        const float baseSemis = currentNoteF + P.bendSemis + velBend
                              + (P.masterTuneCents + unisonDetune + vibCents) * 0.01f;

        if (model == modelCS80)
        {
            for (int c = 0; c < 2; ++c)
            {
                const auto& O = P.osc[c];
                const float semis = baseSemis + O.footSemis + O.semi + O.fine * 0.01f
                                  + driftCents[c] * 0.01f;
                const float hz = 440.f * std::exp2 ((semis - 69.f) / 12.f);
                vco[c].setFrequency (hz, sr);
                vcoHz[c] = hz;
                smoothTo (pwEff[c], O.pw + tolPwOffset * P.drift, smCoef);
                smoothTo (pwmDepth[c], O.pwmDepth, smCoef);
                // level folded into the mix gains so a fader move can't step
                smoothTo (sawG[c],   O.on ? O.saw   * O.level : 0.f, smCoef);
                smoothTo (pulseG[c], O.on ? O.pulse * O.level : 0.f, smCoef);
            }
            smoothTo (noiseG, P.noiseLevel, smCoef);
            smoothTo (sineG, P.sineLevel, smCoef);
            const float ch1Semis = baseSemis + P.osc[0].footSemis + P.osc[0].fine * 0.01f
                                 + driftCents[0] * 0.01f;
            sineOsc.setFrequency (440.f * std::exp2 ((ch1Semis - 69.f) / 12.f), sr);
            updateSub (P, ch1Semis, smCoef);
        }
        else
        {
            const auto& J = P.jp;
            const float s1 = baseSemis + J.vco1FootSemis + driftCents[0] * 0.01f;
            vcoHz[0] = 440.f * std::exp2 ((s1 - 69.f) / 12.f);
            vco[0].setFrequency (vcoHz[0], sr);

            // VCO-2 LOW: drops out of keyboard tracking and runs at LF, which
            // is how most Jupiter-8 sync and cross-mod patches are built.
            if (J.vco2Low)
                vcoHz[1] = 2.f * std::exp2 ((J.vco2FootSemis + J.semi + J.fine * 0.01f) / 12.f);
            else
                vcoHz[1] = 440.f * std::exp2 ((baseSemis + J.vco2FootSemis + J.semi
                                               + J.fine * 0.01f + driftCents[1] * 0.01f - 69.f) / 12.f);
            vco[1].setFrequency (vcoHz[1], sr);

            smoothTo (pwEff[0], J.pw + tolPwOffset * P.drift, smCoef);
            smoothTo (pwmDepth[0], J.pwmDepth, smCoef);
            smoothTo (jpMixA, 1.f - J.mix, smCoef);
            smoothTo (jpMixB, J.mix, smCoef);
            updateSub (P, s1, smCoef);
        }

        // --- Ring modulator control
        ringOn = P.ring.on;
        if (ringOn)
        {
            smoothTo (ringDepth, P.ring.depth, smCoef);
            ringHz = P.ring.rateHz;
            ringEnvAmt = P.ring.envAmt;
        }

        // --- Filter cutoff modulation (octave domain). Only the *parameter*
        // is smoothed - envelope, LFO and touch modulation must stay fast.
        const float lpBase   = model == modelCS80 ? P.lpfCutoff : P.jp.lpf;
        const float hpBase   = model == modelCS80 ? P.hpfCutoff : P.jp.hpf;
        const float keyTrk   = model == modelCS80 ? P.keyTrack : P.jp.keyTrack;
        const float envAmt   = model == modelCS80 ? P.filterEnvAmt
                                                  : (P.jp.envInv ? -P.jp.envAmt : P.jp.envAmt);
        const float resTgt   = model == modelCS80 ? P.resonance : P.jp.res;
        const float driveTgt = model == modelCS80 ? P.filterDrive : P.jp.drive;
        smoothTo (lpOct, std::log2 (std::fmax (30.f, lpBase)), smCoef);
        smoothTo (hpOct, std::log2 (std::fmax (10.f, hpBase)), smCoef);
        smoothTo (resSm, resTgt, smCoef);
        smoothTo (driveSm, driveTgt, smCoef);
        smoothTo (hpResSm, P.hpfRes, smCoef);
        smoothTo (brillSm, P.brilliance, smCoef);
        smoothTo (resOffSm, P.resOffset, smCoef);

        // Common modulation, shared by both CS-80 channels
        float oct = lpOct;
        oct += keyTrk * (currentNoteF - 60.f) / 12.f;
        oct += lfoVal * P.lfoToFilter * 3.f;
        oct += pressure * P.touchToBright * 2.f;
        oct += (velocity - 0.7f) * P.velToFilter * 2.f;
        oct += tolCutoffOct * P.drift;
        // BRILLIANCE moves every filter in the instrument at once. It opens
        // the high-pass too, at half depth - on the original it moves both,
        // but full depth there turns a bright patch thin.
        oct += brillSm * 3.f;
        const float hpHz = std::exp2 (clampf (hpOct + brillSm * 1.5f, 3.3f, 14.2f));
        const float res  = clampf (resSm + resOffSm, 0.f, 1.f);

        if (model == modelCS80)
        {
            const float lpHz1 = std::exp2 (clampf (oct + envAmt * fEnv.value() * 5.f, 4.3f, 14.2f));
            filter[0].setParams (hpHz, lpHz1, res, driveSm, hpResSm);

            // Channel II: its own cutoff, resonance and envelope depth, and
            // its own envelope generator running at its own speed.
            const float env2 = clampf (envAmt + P.ch2.envAmt, -1.f, 1.f);
            const float lpHz2 = std::exp2 (clampf (oct + P.ch2.cutoffOct
                                                   + env2 * fEnv2.value() * 5.f, 4.3f, 14.2f));
            const float res2 = clampf (res + P.ch2.res, 0.f, 1.f);
            filter[1].setParams (hpHz, lpHz2, res2, driveSm, hpResSm);
        }
        else
        {
            const float lpHz = std::exp2 (clampf (oct + envAmt * fEnv.value() * 5.f, 4.3f, 14.2f));
            jpFilter.setParams (hpHz, lpHz, res, P.jp.slope24, driveSm);
        }

        // --- Amp scaling: velocity, touch level, card level tolerance
        smoothTo (ampScale, (1.f - P.velToAmp * (1.f - velocity))
                          * (1.f + pressure * P.touchToLevel * 0.6f)
                          * (1.f + tolLevel * P.drift)
                          * noteGain
                          * (model == modelCS80 ? P.csGain : P.jpGain), smCoef);

        // --- Pan: spread places cards across the field (odd/even L/R like
        // the CS-80 voice card outputs), plus tolerance offset + unison spread
        float pan = cardSide * 0.5f * P.stereoSpread
                  + tolPan * P.drift + unisonPan;
        pan = clampf (pan, -1.f, 1.f);
        smoothTo (panPos, pan, smCoef);
        const float a = (panPos + 1.f) * 0.25f * 3.14159265f;
        panL = std::cos (a);
        panR = std::sin (a);

        smoothInit = false;
    }

    const VoiceParams* params = nullptr;
    VCO vco[2], subVco;
    SineOsc sineOsc, ringOsc;
    Noise noise;
    CS80Filter filter[2];      // CS-80 channel I and channel II
    JP8Filter jpFilter;
    ADSR fEnv, aEnv;           // channel I (and the whole JP-8 card)
    ADSR fEnv2, aEnv2;         // CS-80 channel II
    ADEnv ringEnv;

    float sr = 44100.f;
    int note = -1;
    int model = modelCS80;
    int owner = kOwnerLive;
    float lastVco2 = 0.f;
    float vcoHz[2] = { 440.f, 440.f };
    float velocity = 1.f;
    bool held = false, sustained = false;
    bool steal = false;
    uint32_t serial = 0;

    float targetNoteF = 60.f, currentNoteF = 60.f, velBend = 0.f;
    float unisonDetune = 0.f, unisonPan = 0.f, noteGain = 1.f;
    float heldSeconds = 0.f, pressure = 0.f, polyPressure = 0.f;
    float driftWalk[2] = { 0.f, 0.f }, driftCents[2] = { 0.f, 0.f };

    // control-rate cached values, one-pole smoothed toward the block-rate
    // parameter snapshot (see smoothTo). smoothInit snaps them all on the
    // first control block of a note so a fresh voice never sweeps in.
    bool  smoothInit = true;
    float pwEff[2] = { 0.5f, 0.5f }, pwmDepth[2] = { 0.f, 0.f };
    float sawG[2] = { 0.f, 0.f }, pulseG[2] = { 0.f, 0.f }, noiseG = 0.f, sineG = 0.f;
    float jpMixA = 0.5f, jpMixB = 0.5f;
    float subG = 0.f;
    int   subWave = 0;
    float lpOct = 13.f, hpOct = 4.32f, resSm = 0.15f, driveSm = 0.2f, hpResSm = 0.f;
    float brillSm = 0.f, resOffSm = 0.f;
    float ampScale = 1.f, panPos = 0.f, panL = 0.707f, panR = 0.707f;
    bool  ringOn = false;
    float ringDepth = 0.f, ringHz = 220.f, ringEnvAmt = 0.f;

    // per-card tolerances
    float tolPitchCents[2] = { 0.f, 0.f };
    float tolCutoffOct = 0.f, tolEnvScale = 0.f;
    float tolPwOffset = 0.f, tolLevel = 0.f, tolPan = 0.f, cardSide = -1.f;
};
} // namespace eighty
