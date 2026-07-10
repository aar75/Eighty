#pragma once
#include <cmath>

namespace eighty
{
// Single VCO core, CS-80 style: one phase accumulator produces both a
// sawtooth and a variable-width pulse, mixed downstream. PolyBLEP
// anti-aliasing on both discontinuities.
class VCO
{
public:
    void reset (float startPhase = 0.f) { phase = startPhase; }

    void setFrequency (float hz, float sampleRate)
    {
        inc = hz / sampleRate;
        if (inc > 0.5f) inc = 0.5f;
    }

    // Advances the core one sample. Outputs both waves from the same core.
    void tick (float pw, float& sawOut, float& pulseOut)
    {
        phase += inc;
        wrappedFlag = phase >= 1.f;
        if (wrappedFlag) phase -= 1.f;

        // Saw: ramp with BLEP at wrap
        float saw = 2.f * phase - 1.f;
        saw -= polyBlep (phase, inc);
        sawOut = saw;

        // Pulse: two BLEP-corrected edges
        float pulse = (phase < pw) ? 1.f : -1.f;
        pulse += polyBlep (phase, inc);
        float t2 = phase - pw;
        if (t2 < 0.f) t2 += 1.f;
        pulse -= polyBlep (t2, inc);
        // Remove DC introduced by asymmetric pulse width
        pulse -= (2.f * pw - 1.f);
        pulseOut = pulse;
    }

    float currentPhase() const { return phase; }
    bool justWrapped() const   { return wrappedFlag; }

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

    float phase = 0.f, inc = 0.f;
    bool wrappedFlag = false;
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
