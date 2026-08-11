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
//   ENS  a string-machine ensemble rather than a chorus, and not a polite
//        one: a slow deep sweep with a *fast* second modulator riding on it
//        (the Solina's two-oscillator ensemble is what makes it shimmer
//        rather than swim), one dominant tap per side plus a quieter
//        companion for the comb, the right channel's modulators inverted so
//        the image swings side to side, and a wetter blend than the chorus
//        modes take - it is meant to read as an effect you switched on
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
        const float rateMul  = mode == 1 ? 2.2f : mode == 2 ? 0.55f : 1.f;
        const float depthMul = mode == 1 ? 1.5f : mode == 2 ? 1.9f : 1.f;
        inc  = rateHz * rateMul / sr;
        // ENS runs its second modulator far *above* the first - a slow swim
        // with a ~5 Hz shimmer riding on it is the string-machine sound, and
        // it is the fast one you actually hear. The other modes keep it as a
        // slow drift between their taps.
        inc2 = mode == 2 ? rateHz * 9.f / sr : rateHz * rateMul * 0.19f / sr;
        depth = depth01 * depthMul;
        mix = mix01;
        // An ensemble is supposed to take the sound over, not sit politely
        // behind it, so MIX pulls the dry down harder here and pushes a
        // louder wet up in its place. At the default 40% mix the wet is
        // already almost level with the dry.
        wetGain = mode == 2 ? 1.55f : 1.f;
        dryCut  = mode == 2 ? 0.8f : 0.5f;
    }

    void process (float* l, float* r, int n)
    {
        const bool ens = mode == 2;
        // 18 ms centre for the ensemble, 12 for the chorus modes
        const float base = (ens ? 0.018f : 0.012f) * sr;
        // The sweep has to stay inside the delay line: the ensemble's fast
        // modulator adds to the slow one, so allow for both before clamping.
        const float reach = ens ? 1.f + fastDepth : 1.f;
        const float maxSpan = (base - 2.f) / reach;
        const float span = fminf ((ens ? 0.009f : 0.006f) * sr * depth, maxSpan);
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
            if (ens)
            {
                // One dominant tap per side carrying the whole sweep, plus a
                // quieter companion a third of a cycle behind it for the
                // ensemble's comb. Two equally weighted taps average each
                // other's movement out, which is exactly how this mode ended
                // up sounding like a slightly wider chorus instead of an
                // ensemble - the weights are deliberately lopsided.
                //
                // Right runs both modulators inverted, so its delay is the
                // mirror of the left's: the image swings side to side rather
                // than just getting thicker.
                const float fast = fastDepth * std::sin (phase2 * 6.2831853f);
                auto sweep = [] (float ph, float f)
                { return std::sin (ph * 6.2831853f) + f; };

                wetL = readInterp (bufL, base + span * sweep (phase, fast))
                     + readInterp (bufL, base + span * sweep (phase + third, fast)) * 0.45f;
                wetR = readInterp (bufR, base - span * sweep (phase, fast))
                     + readInterp (bufR, base - span * sweep (phase + third, fast)) * 0.45f;
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

            l[i] = l[i] * (1.f - mix * dryCut) + lpL * mix * wetGain;
            r[i] = r[i] * (1.f - mix * dryCut) + lpR * mix * wetGain;

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
    float depth = 0.3f, mix = 0.4f, wetGain = 1.f, dryCut = 0.5f;
    float lpL = 0.f, lpR = 0.f, bbd = 0.5f;
    static constexpr float fastDepth = 0.32f;   // ENS second modulator, x span
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
