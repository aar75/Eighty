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

    float hpfCutoff = 20.f, lpfCutoff = 9000.f, resonance = 0.15f;
    float filterEnvAmt = 0.35f, keyTrack = 0.5f, filterDrive = 0.2f;

    float fA = 0.005f, fD = 0.35f, fS = 0.4f, fR = 0.3f;
    float aA = 0.004f, aD = 0.4f,  aS = 0.8f, aR = 0.35f;
    float velToAmp = 0.5f, velToFilter = 0.3f;

    float lfoToPitch = 0.f, lfoToFilter = 0.f, lfoToAmp = 0.f;

    float touchRise = 0.6f, touchToVib = 0.25f, touchToBright = 0.2f, touchToLevel = 0.15f;

    float drift = 0.35f;
    float stereoSpread = 0.6f;
    float csGain = 1.f, jpGain = 1.f;   // CS/JP mix (Split & Layer modes)
    float glideTime = 0.05f;
    int   glideMode = 0;           // 0 off, 1 legato, 2 always
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
        bool  sync = false;            // VCO2 hard-synced to VCO1
        float xmod = 0.f;              // VCO2 -> VCO1 frequency cross-mod
        float hpf = 20.f, lpf = 9000.f, res = 0.15f;
        bool  slope24 = true;
        float envAmt = 0.35f, keyTrack = 0.5f;
        float fA = 0.005f, fD = 0.35f, fS = 0.4f, fR = 0.3f;
        float aA = 0.004f, aD = 0.4f,  aS = 0.8f, aR = 0.35f;
    } jp;
};

enum VoiceModel { modelCS80 = 0, modelJP8 = 1 };

// One CS-80-style voice card: two VCO channels (saw + PWM pulse from a
// shared core each), noise, HPF->LPF filter, filter + amp ADSRs.
// Each card gets fixed component-tolerance offsets seeded by its index,
// scaled at runtime by the Drift parameter - like eight slightly
// different boards in one chassis.
class Voice
{
public:
    static constexpr int kCtrl = 16; // control-rate interval in samples

    void prepare (double sampleRate, int voiceIndex, const VoiceParams* sharedParams)
    {
        sr = (float) sampleRate;
        params = sharedParams;
        filter.prepare (sampleRate);
        jpFilter.prepare (sampleRate);
        fEnv.prepare (sampleRate);
        aEnv.prepare (sampleRate);
        noise.seed (0xBADA55u + (uint32_t) voiceIndex * 7919u);

        // Fixed per-card tolerances (stable across runs)
        uint32_t s = 0x1234ABCDu + (uint32_t) voiceIndex * 2654435761u;
        auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5;
                            return (float) (int32_t) s * 4.6566129e-10f; };
        tolPitchCents = rnd() * 6.f;
        tolCutoffOct  = rnd() * 0.25f;
        tolEnvScale   = rnd() * 0.12f;
        tolPwOffset   = rnd() * 0.03f;
        tolLevel      = rnd() * 0.08f;
        tolPan        = rnd() * 0.2f;
        cardSide      = (voiceIndex % 2 == 0) ? -1.f : 1.f;
    }

    void noteOn (int midiNote, float vel, float detuneCents, float panOffset,
                 float glideFromNote, bool retrigger, float gainScale = 1.f,
                 int voiceModel = modelCS80)
    {
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
        else if (! aEnv.isActive())
            currentNoteF = targetNoteF;

        if (retrigger || ! aEnv.isActive())
        {
            const float ts = 1.f + tolEnvScale * params->drift;
            if (model == modelJP8)
            {
                const auto& J = params->jp;
                fEnv.setParams (J.fA, J.fD, J.fS, J.fR, ts);
                aEnv.setParams (J.aA, J.aD, J.aS, J.aR, ts);
            }
            else
            {
                fEnv.setParams (params->fA, params->fD, params->fS, params->fR, ts);
                aEnv.setParams (params->aA, params->aD, params->aS, params->aR, ts);
            }
            fEnv.noteOn();
            aEnv.noteOn();
            if (! wasActive())
            {
                vco[0].reset (0.f);
                vco[1].reset (0.37f); // free-running feel: channels never phase-locked
                filter.reset();
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
    void setSustained (bool s)   { sustained = s; if (! s && ! held) release(); }
    void release()               { fEnv.noteOff(); aEnv.noteOff(); }
    void kill()                  { fEnv.kill(); aEnv.kill(); held = false; sustained = false; }

    bool wasActive() const       { return aEnv.isActive(); }
    bool isHeld() const          { return held; }
    bool isReleasing() const     { return aEnv.isReleasing(); }
    int  currentNote() const     { return note; }
    float envLevel() const       { return aEnv.value(); }
    uint32_t age() const         { return serial; }
    void setPolyPressure (float p) { polyPressure = p; }

    // lfoBuf / pwmBuf: one value per sample for this block segment
    void render (float* left, float* right, int numSamples,
                 const float* lfoBuf, const float* pwmBuf)
    {
        if (! aEnv.isActive())
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

                if (model == modelCS80)
                {
                    // PWM from the dedicated PWM LFO (per channel depth)
                    float s0 = 0.f, p0 = 0.f, s1 = 0.f, p1 = 0.f;
                    float pwA = clampf (pwEff[0] + P.osc[0].pwmDepth * 0.45f * pwmBuf[idx], 0.03f, 0.97f);
                    float pwB = clampf (pwEff[1] + P.osc[1].pwmDepth * 0.45f * pwmBuf[idx], 0.03f, 0.97f);
                    vco[0].tick (pwA, s0, p0);
                    vco[1].tick (pwB, s1, p1);

                    if (P.osc[0].on) x += (s0 * P.osc[0].saw + p0 * P.osc[0].pulse) * gain[0];
                    if (P.osc[1].on) x += (s1 * P.osc[1].saw + p1 * P.osc[1].pulse) * gain[1];
                    x += noise.tick() * P.noiseLevel * 0.7f;

                    x = filter.process (x);
                }
                else
                {
                    const auto& J = P.jp;
                    const float pw = clampf (pwEff[0] + J.pwmDepth * 0.45f * pwmBuf[idx], 0.03f, 0.97f);

                    // cross-mod: VCO2 (previous sample) FMs VCO1
                    if (J.xmod > 0.001f)
                        vco[0].setFrequency (vcoHz[0] * (1.f + J.xmod * 2.f * lastVco2), sr);

                    float s0, p0, s1, p1;
                    vco[0].tick (J.vco1Wave == 2 ? pw : 0.5f, s0, p0);
                    if (J.sync && vco[0].justWrapped())
                        vco[1].reset (0.f);
                    vco[1].tick (J.vco2Wave == 2 ? pw : 0.5f, s1, p1);

                    const float o1 = J.vco1Wave == 0 ? vco[0].triangle()
                                   : J.vco1Wave == 1 ? s0 : p0;   // pulse & square via p0
                    const float o2 = J.vco2Wave == 0 ? vco[1].triangle()
                                   : J.vco2Wave == 1 ? s1
                                   : J.vco2Wave == 2 ? p1 : noise.tick();
                    lastVco2 = o2;

                    x = (o1 * (1.f - J.mix) + o2 * J.mix) * 1.2f;
                    x = jpFilter.process (x);
                }

                fEnv.tick();
                float amp = aEnv.tick() * ampScale;
                // global LFO tremolo routing (bipolar -> unipolar dip)
                amp *= 1.f - P.lfoToAmp * 0.5f * (1.f + lfoBuf[idx]) * 0.5f;

                const float o = x * amp * 0.8f; // headroom
                left[idx]  += o * panL;
                right[idx] += o * panR;
            }
            i += n;
        }
    }

private:
    static float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

    void updateControl (const VoiceParams& P, float lfoVal)
    {
        const float ctrlDt = (float) kCtrl / sr;
        heldSeconds += held ? ctrlDt : 0.f;

        // --- Touch pressure: simulated ramp while held, or real AT if higher
        float simP = held ? (1.f - std::exp (-heldSeconds / std::fmax (0.05f, P.touchRise))) : pressure;
        float extP = std::fmax (P.channelPressure, polyPressure);
        pressure = std::fmax (simP * (held ? 1.f : 0.f), extP);

        // --- Slow per-card pitch wander (analog drift random walk)
        driftWalk += (noise.tick() - driftWalk * 0.02f) * 0.01f;
        const float driftCents = (tolPitchCents + driftWalk * 40.f) * P.drift;

        // --- Glide
        if (P.glideMode == 0 || P.glideTime <= 0.001f)
            currentNoteF = targetNoteF;
        else
        {
            const float coef = 1.f - std::exp (-ctrlDt * 6.f / std::fmax (0.01f, P.glideTime));
            currentNoteF += (targetNoteF - currentNoteF) * coef;
        }

        // --- Vibrato: global LFO routing + touch vibrato + mod wheel
        const float vibCents = lfoVal * (P.lfoToPitch * P.lfoToPitch * 200.f
                                         + pressure * P.touchToVib * 60.f
                                         + P.modWheel * 60.f);

        const float baseSemis = currentNoteF + P.bendSemis
                              + (P.masterTuneCents + driftCents + unisonDetune + vibCents) * 0.01f;

        if (model == modelCS80)
        {
            for (int c = 0; c < 2; ++c)
            {
                const auto& O = P.osc[c];
                const float semis = baseSemis + O.footSemis + O.semi + O.fine * 0.01f;
                const float hz = 440.f * std::exp2 ((semis - 69.f) / 12.f);
                vco[c].setFrequency (hz, sr);
                vcoHz[c] = hz;
                pwEff[c] = O.pw + tolPwOffset * P.drift;
                gain[c]  = O.level;
            }
        }
        else
        {
            const auto& J = P.jp;
            const float s1 = baseSemis + J.vco1FootSemis;
            const float s2 = baseSemis + J.vco2FootSemis + J.semi + J.fine * 0.01f;
            vcoHz[0] = 440.f * std::exp2 ((s1 - 69.f) / 12.f);
            vcoHz[1] = 440.f * std::exp2 ((s2 - 69.f) / 12.f);
            vco[0].setFrequency (vcoHz[0], sr);
            vco[1].setFrequency (vcoHz[1], sr);
            pwEff[0] = J.pw + tolPwOffset * P.drift;
        }

        // --- Filter cutoff modulation (octave domain)
        const float lpBase   = model == modelCS80 ? P.lpfCutoff : P.jp.lpf;
        const float keyTrk   = model == modelCS80 ? P.keyTrack : P.jp.keyTrack;
        const float envAmt   = model == modelCS80 ? P.filterEnvAmt : P.jp.envAmt;
        float oct = std::log2 (std::fmax (30.f, lpBase));
        oct += keyTrk * (currentNoteF - 60.f) / 12.f;
        oct += envAmt * fEnv.value() * 5.f;
        oct += lfoVal * P.lfoToFilter * 3.f;
        oct += pressure * P.touchToBright * 2.f;
        oct += (velocity - 0.7f) * P.velToFilter * 2.f;
        oct += tolCutoffOct * P.drift;
        const float lpHz = std::exp2 (clampf (oct, 4.3f, 14.2f)); // ~20 Hz .. 18.8 kHz
        if (model == modelCS80)
            filter.setParams (P.hpfCutoff, lpHz, P.resonance, P.filterDrive);
        else
            jpFilter.setParams (P.jp.hpf, lpHz, P.jp.res, P.jp.slope24);

        // --- Amp scaling: velocity, touch level, card level tolerance
        ampScale = (1.f - P.velToAmp * (1.f - velocity))
                 * (1.f + pressure * P.touchToLevel * 0.6f)
                 * (1.f + tolLevel * P.drift)
                 * noteGain
                 * (model == modelCS80 ? P.csGain : P.jpGain);

        // --- Pan: spread places cards across the field (odd/even L/R like
        // the CS-80 voice card outputs), plus tolerance offset + unison spread
        float pan = cardSide * 0.5f * P.stereoSpread
                  + tolPan * P.drift + unisonPan;
        pan = clampf (pan, -1.f, 1.f);
        const float a = (pan + 1.f) * 0.25f * 3.14159265f;
        panL = std::cos (a);
        panR = std::sin (a);
    }

    const VoiceParams* params = nullptr;
    VCO vco[2];
    Noise noise;
    CS80Filter filter;
    JP8Filter jpFilter;
    ADSR fEnv, aEnv;

    float sr = 44100.f;
    int note = -1;
    int model = modelCS80;
    float lastVco2 = 0.f;
    float vcoHz[2] = { 440.f, 440.f };
    float velocity = 1.f;
    bool held = false, sustained = false;
    bool steal = false;
    uint32_t serial = 0;

    float targetNoteF = 60.f, currentNoteF = 60.f;
    float unisonDetune = 0.f, unisonPan = 0.f, noteGain = 1.f;
    float heldSeconds = 0.f, pressure = 0.f, polyPressure = 0.f;
    float driftWalk = 0.f;

    // control-rate cached values
    float pwEff[2] = { 0.5f, 0.5f };
    float gain[2] = { 0.9f, 0.9f };
    float ampScale = 1.f, panL = 0.707f, panR = 0.707f;

    // per-card tolerances
    float tolPitchCents = 0.f, tolCutoffOct = 0.f, tolEnvScale = 0.f;
    float tolPwOffset = 0.f, tolLevel = 0.f, tolPan = 0.f, cardSide = -1.f;
};
} // namespace eighty
