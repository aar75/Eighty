#pragma once
#include <cmath>
#include <cstdint>

namespace eighty
{
// Single VCO core: one phase accumulator produces both a sawtooth and a
// variable-width pulse, mixed downstream. PolyBLEP anti-aliasing on both
// discontinuities.
//
// The core also tracks *where inside the sample* it wrapped, which is what
// hard sync needs. Resetting a slave oscillator on the sample grid quantises
// the sync pitch to the sample rate and injects an uncorrected step - that
// is the difference between a Jupiter-8 sync scream and a buzz. syncTo()
// resets at the true sub-sample instant and stages a BLEP correction for
// the step it creates.
class VCO
{
public:
    void reset (float startPhase = 0.f)
    {
        phase = startPhase;
        pendingSync = false;
        syncBlepSaw = syncBlepPulse = 0.f;
    }

    void setFrequency (float hz, float sampleRate)
    {
        if (! (hz > 0.f)) hz = 0.f;                 // also catches NaN
        inc = hz / sampleRate;
        if (inc > 0.5f) inc = 0.5f;
    }

    // Advances the core one sample. Outputs both waves from the same core.
    void tick (float pw, float& sawOut, float& pulseOut)
    {
        // A sync reset already placed the phase for this sample, so the
        // usual increment (and the usual wrap BLEP) must not run again.
        const bool synced = pendingSync;
        pendingSync = false;

        if (! synced)
        {
            phase += inc;
            wrappedFlag = phase >= 1.f;
            if (wrappedFlag)
            {
                phase -= 1.f;
                wrapFrac = inc > 1.0e-9f ? phase / inc : 0.f;
            }
        }
        else
            wrappedFlag = false;

        // Saw: ramp with BLEP at wrap
        float saw = 2.f * phase - 1.f;
        if (! synced) saw -= polyBlep (phase, inc);

        // Pulse: two BLEP-corrected edges
        float pulse = (phase < pw) ? 1.f : -1.f;
        if (! synced)
        {
            pulse += polyBlep (phase, inc);
            float t2 = phase - pw;
            if (t2 < 0.f) t2 += 1.f;
            pulse -= polyBlep (t2, inc);
        }
        // Remove DC introduced by asymmetric pulse width
        pulse -= (2.f * pw - 1.f);

        saw   += syncBlepSaw;
        pulse += syncBlepPulse;
        syncBlepSaw = syncBlepPulse = 0.f;

        sawOut = saw;
        pulseOut = pulse;
    }

    float currentPhase() const { return phase; }
    bool  justWrapped() const  { return wrappedFlag; }
    // How far into the current sample the wrap happened, as a fraction of a
    // sample already elapsed since it. Only meaningful when justWrapped().
    float wrapFraction() const { return wrapFrac; }

    // Hard sync. frac is the master's wrapFraction(): the reset instant is
    // that far in the past relative to this sample. Call before tick().
    void syncTo (float frac, float pw)
    {
        if (frac < 0.f) frac = 0.f;
        if (frac > 0.999f) frac = 0.999f;

        // Where this oscillator would have been at the reset instant
        float pAt = phase + (1.f - frac) * inc;
        pAt -= std::floor (pAt);

        const float sawBefore = 2.f * pAt - 1.f;
        const float sawAfter  = -1.f;
        const float pulBefore = (pAt < pw) ? 1.f : -1.f;
        const float pulAfter  = (0.f < pw) ? 1.f : -1.f;

        // Same residual the wrap BLEP uses, evaluated at the sub-sample
        // offset and scaled by half the actual step height. This corrects
        // the sample after the discontinuity; the one before it has already
        // been emitted, so a little of the step survives - which is what the
        // oversampling is there to mop up.
        const float r = -0.5f * (2.f * frac - frac * frac - 1.f);
        syncBlepSaw   = (sawBefore - sawAfter) * r;
        syncBlepPulse = (pulBefore - pulAfter) * r;

        phase = frac * inc;
        pendingSync = true;
    }

    // Triangle derived from the same core (aliasing is negligible at LF harmonics)
    float triangle() const     { return 2.f * std::fabs (2.f * phase - 1.f) - 1.f; }

private:
    static float polyBlep (float t, float dt)
    {
        if (dt <= 0.f) return 0.f;
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.f;
        }
        if (t > 1.f - dt)
        {
            t = (t - 1.f) / dt;
            return t * t + t + t + 1.f;
        }
        return 0.f;
    }

    float phase = 0.f, inc = 0.f, wrapFrac = 0.f;
    float syncBlepSaw = 0.f, syncBlepPulse = 0.f;
    bool  wrappedFlag = false, pendingSync = false;
};

// Plain sine. The CS-80 routes one of these around the filters straight
// into the VCA mixer - a sine has no harmonics for a filter to work on, so
// filtering it would only cost level. It is why CS-80 pads keep a solid
// fundamental under a heavily swept top end.
class SineOsc
{
public:
    void reset (float p = 0.f) { phase = p; }

    void setFrequency (float hz, float sampleRate)
    {
        if (! (hz > 0.f)) hz = 0.f;
        inc = hz / sampleRate;
        if (inc > 0.5f) inc = 0.5f;
    }

    float tick()
    {
        phase += inc;
        if (phase >= 1.f) phase -= 1.f;
        return std::sin (phase * 6.28318530718f);
    }

private:
    float phase = 0.f, inc = 0.f;
};

// Cheap white noise (xorshift)
class Noise
{
public:
    void seed (uint32_t s) { state = s | 1u; }
    float tick()
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return (float) (int32_t) state * 4.6566129e-10f; // ~[-1, 1]
    }
private:
    uint32_t state = 0x9e3779b9u;
};
} // namespace eighty
