#pragma once
#include <vector>
#include <cmath>

namespace eighty
{
// Stereo bucket-brigade chorus. Both machines lean on their chorus for a
// large part of their identity, so this has three modes rather than one:
//
//   I    two taps in quadrature, slow and shallow - the Jupiter-8's first
//        chorus button, the one that just makes things wide
//   II   the same topology run faster and deeper - the second button
//   ENS  three taps 120 degrees apart plus a slow secondary sweep, which is
//        the thicker CS-80 ensemble character rather than a chorus
//
// The wet path is deliberately darker than the dry: a BBD is a chain of
// sample-and-holds with a limited clock, and that top-end roll-off is most
// of why an analog chorus sits behind the sound instead of on top of it.
class Chorus
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = (float) sampleRate;
        size = (int) (sr * 0.06f) + 4;
        bufL.assign ((size_t) size, 0.f);
        bufR.assign ((size_t) size, 0.f);
        w = 0; phase = 0.f; phase2 = 0.f;
        lpL = lpR = 0.f;
        // one-pole at ~5.5 kHz: the BBD clock's practical ceiling
        bbd = 1.f - std::exp (-6.2831853f * 5500.f / sr);
    }

    // mode: 0 = I, 1 = II, 2 = ENS
    void setParams (float rateHz, float depth01, float mix01, int modeIdx)
    {
        mode = modeIdx < 0 ? 0 : (modeIdx > 2 ? 2 : modeIdx);
        const float rateMul  = mode == 1 ? 2.2f : mode == 2 ? 0.7f : 1.f;
        const float depthMul = mode == 1 ? 1.5f : mode == 2 ? 1.25f : 1.f;
        inc  = rateHz * rateMul / sr;
        inc2 = rateHz * rateMul * 0.19f / sr;      // slow secondary sweep (ENS)
        depth = depth01 * depthMul;
        mix = mix01;
    }

    void process (float* l, float* r, int n)
    {
        const float base = 0.012f * sr;          // 12 ms centre
        const float span = 0.006f * sr * depth;  // +/- up to 6 ms
        const float third = 1.f / 3.f;
        for (int i = 0; i < n; ++i)
        {
            phase += inc;
            if (phase >= 1.f) phase -= 1.f;
            phase2 += inc2;
            if (phase2 >= 1.f) phase2 -= 1.f;

            bufL[(size_t) w] = l[i];
            bufR[(size_t) w] = r[i];

            float wetL, wetR;
            if (mode == 2)
            {
                // Three taps evenly spaced round the cycle, each one drifting
                // against the others on the secondary sweep. Taps 1 and 3 are
                // hard-placed L/R and tap 2 sits in the middle.
                const float drift = 0.35f * std::sin (phase2 * 6.2831853f);
                const float m0 = std::sin ((phase) * 6.2831853f);
                const float m1 = std::sin ((phase + third + drift) * 6.2831853f);
                const float m2 = std::sin ((phase + 2.f * third - drift) * 6.2831853f);

                const float t0 = readInterp (bufL, base + span * m0);
                const float t1 = readInterp (bufL, base + span * m1);
                const float t2 = readInterp (bufR, base + span * m2);
                const float t3 = readInterp (bufR, base + span * m1);

                wetL = (t0 * 0.62f + t1 * 0.38f);
                wetR = (t2 * 0.62f + t3 * 0.38f);
            }
            else
            {
                const float m1 = std::sin (phase * 6.2831853f);
                const float m2 = std::sin ((phase + 0.25f) * 6.2831853f);
                wetL = readInterp (bufL, base + span * m1);
                wetR = readInterp (bufR, base + span * m2);
            }

            // BBD top-end roll-off, wet path only
            lpL += (wetL - lpL) * bbd;
            lpR += (wetR - lpR) * bbd;

            l[i] = l[i] * (1.f - mix * 0.5f) + lpL * mix;
            r[i] = r[i] * (1.f - mix * 0.5f) + lpR * mix;

            if (++w >= size) w = 0;
        }
    }

private:
    float readInterp (const std::vector<float>& buf, float delaySamples) const
    {
        float rp = (float) w - delaySamples;
        while (rp < 0.f) rp += (float) size;
        int i0 = (int) rp;
        float frac = rp - (float) i0;
        int i1 = i0 + 1; if (i1 >= size) i1 = 0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
    }

    std::vector<float> bufL, bufR;
    int size = 0, w = 0, mode = 0;
    float sr = 44100.f, phase = 0.f, phase2 = 0.f, inc = 0.f, inc2 = 0.f;
    float depth = 0.3f, mix = 0.4f;
    float lpL = 0.f, lpR = 0.f, bbd = 0.5f;
};

// Stereo delay with feedback and gentle damping in the loop.
class StereoDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        size = (int) (sr * 2.5f) + 4;
        bufL.assign ((size_t) size, 0.f);
        bufR.assign ((size_t) size, 0.f);
        w = 0; lpL = lpR = 0.f;
        smoothedDelay = 0.35f * sr;
    }

    void setParams (float timeSeconds, float feedback, float mix01)
    {
        targetDelay = std::fmax (1.f, std::fmin (timeSeconds * sr, (float) size - 2.f));
        fb = feedback;
        mix = mix01;
    }

    void process (float* l, float* r, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            // slew delay time to avoid zipper/pitch jumps on tempo change
            smoothedDelay += (targetDelay - smoothedDelay) * 0.0005f;

            const float dl = readInterp (bufL, smoothedDelay);
            const float dr = readInterp (bufR, smoothedDelay);

            // damping in the feedback loop (analog-ish tape rolloff)
            lpL += (dl - lpL) * 0.35f;
            lpR += (dr - lpR) * 0.35f;

            bufL[(size_t) w] = l[i] + lpR * fb;   // gentle ping-pong cross-feed
            bufR[(size_t) w] = r[i] + lpL * fb;

            l[i] += dl * mix;
            r[i] += dr * mix;

            if (++w >= size) w = 0;
        }
    }

private:
    float readInterp (const std::vector<float>& buf, float delaySamples) const
    {
        float rp = (float) w - delaySamples;
        while (rp < 0.f) rp += (float) size;
        int i0 = (int) rp;
        float frac = rp - (float) i0;
        int i1 = i0 + 1; if (i1 >= size) i1 = 0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
    }

    std::vector<float> bufL, bufR;
    int size = 0, w = 0;
    float sr = 44100.f, targetDelay = 1000.f, smoothedDelay = 1000.f;
    float fb = 0.3f, mix = 0.25f, lpL = 0.f, lpR = 0.f;
};

// Output tremolo with slight L/R phase offset for movement.
class Tremolo
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; phase = 0.f; }
    void setParams (float rateHz, float depth01) { inc = rateHz / sr; depth = depth01; }

    void process (float* l, float* r, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            phase += inc;
            if (phase >= 1.f) phase -= 1.f;
            const float gl = 1.f - depth * 0.5f * (1.f + std::sin (phase * 6.2831853f));
            const float gr = 1.f - depth * 0.5f * (1.f + std::sin ((phase + 0.08f) * 6.2831853f));
            l[i] *= gl;
            r[i] *= gr;
        }
    }

private:
    float sr = 44100.f, phase = 0.f, inc = 0.f, depth = 0.5f;
};

// One-knob output limiter: drive (dB) pushed into a fixed ceiling just
// under 0 dBFS. Instant attack on the stereo peak, smooth release; at
// zero drive it still acts as a transparent output safety.
class Limiter
{
public:
    void prepare (double sampleRate)
    {
        release = 1.f - std::exp (-1.f / (0.06f * (float) sampleRate));
        env = 0.f;
    }

    void setDrive (float dB) { drive = std::pow (10.f, dB / 20.f); }

    void process (float* l, float* r, int n)
    {
        constexpr float ceiling = 0.98f;
        for (int i = 0; i < n; ++i)
        {
            const float L = l[i] * drive, R = r[i] * drive;
            const float peak = std::fmax (std::fabs (L), std::fabs (R));
            if (peak > env) env = peak;                      // clamp instantly
            else            env += (peak - env) * release;   // recover smoothly
            const float g = env > ceiling ? ceiling / env : 1.f;
            l[i] = L * g;
            r[i] = R * g;
        }
    }

private:
    float release = 0.001f, drive = 1.f, env = 0.f;
};
} // namespace eighty
