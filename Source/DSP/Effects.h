#pragma once
#include <vector>
#include <cmath>

namespace eighty
{
// Stereo bucket-brigade-style chorus: two short modulated delay lines in
// quadrature (the CS-80's ensemble/chorus section vibe, minus the noise).
class Chorus
{
public:
    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = (float) sampleRate;
        size = (int) (sr * 0.06f) + 4;
        bufL.assign ((size_t) size, 0.f);
        bufR.assign ((size_t) size, 0.f);
        w = 0; phase = 0.f;
    }

    void setParams (float rateHz, float depth01, float mix01)
    {
        inc = rateHz / sr;
        depth = depth01;
        mix = mix01;
    }

    void process (float* l, float* r, int n)
    {
        const float base = 0.012f * sr;          // 12 ms centre
        const float span = 0.006f * sr * depth;  // +/- up to 6 ms
        for (int i = 0; i < n; ++i)
        {
            phase += inc;
            if (phase >= 1.f) phase -= 1.f;
            const float m1 = std::sin (phase * 6.2831853f);
            const float m2 = std::sin ((phase + 0.25f) * 6.2831853f);

            bufL[(size_t) w] = l[i];
            bufR[(size_t) w] = r[i];

            const float dl = base + span * m1;
            const float dr = base + span * m2;
            const float wetL = readInterp (bufL, dl);
            const float wetR = readInterp (bufR, dr);

            l[i] = l[i] * (1.f - mix * 0.5f) + wetL * mix;
            r[i] = r[i] * (1.f - mix * 0.5f) + wetR * mix;

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
    float sr = 44100.f, phase = 0.f, inc = 0.f, depth = 0.3f, mix = 0.4f;
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
} // namespace eighty
