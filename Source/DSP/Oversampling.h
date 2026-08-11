#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace eighty
{
// ----------------------------------------------------------------- ADAA
// First-order antiderivative anti-aliasing for tanh.
//
// A plain tanh() in a per-sample loop generates harmonics above Nyquist and
// folds every one of them back into the audible band as fizz - which is
// exactly what the filter drive stages here were doing. Integrating the
// nonlinearity across the segment between two consecutive input samples
// removes most of that fold-back for the cost of one log-cosh and a divide:
// no filter bank, no latency, and it composes with the oversampler below
// rather than competing with it.
class ADAATanh
{
public:
    void reset() { x1 = 0.f; F1 = antideriv (0.f); }

    float process (float x)
    {
        const float F = antideriv (x);
        const float d = x - x1;
        // The difference quotient is ill-conditioned when two samples are
        // nearly equal; the midpoint value is the limit it is approaching.
        const float y = std::fabs (d) < 1.0e-5f ? std::tanh ((x + x1) * 0.5f)
                                                : (F - F1) / d;
        x1 = x;
        F1 = F;
        return y;
    }

private:
    // log(cosh(x)), written so it cannot overflow for large |x|
    static float antideriv (float x)
    {
        const float a = std::fabs (x);
        return a + std::log1p (std::exp (-2.f * a)) - 0.6931472f;
    }

    float x1 = 0.f, F1 = 0.f;    // antideriv(0) == 0
};

// ------------------------------------------------------- halfband stage
// One 2:1 decimation stage: a 63-tap linear-phase halfband FIR, designed at
// construction from a Blackman-windowed sinc so there is no hand-copied
// coefficient table to get wrong. Every even tap except the centre is zero
// and the kernel is symmetric, so a stage costs 16 multiply-adds and one
// extra multiply per output sample instead of 63.
class HalfbandDecimator
{
public:
    static constexpr int kTaps  = 63;
    static constexpr int kHalf  = kTaps / 2;      // 31 = group delay, in input samples
    static constexpr int kPairs = (kHalf + 1) / 2; // 16 symmetric pairs of non-zero taps
    static constexpr int kSize  = 64, kMask = kSize - 1;

    HalfbandDecimator() { design(); reset(); }

    void reset() { z.fill (0.f); w = 0; }

    // Consumes 2*n input samples and writes n output samples.
    void process (const float* in, float* out, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            push (in[2 * i]);
            push (in[2 * i + 1]);
            out[i] = tap();
        }
    }

private:
    void design()
    {
        // h[k] = 0.5 * sinc(k/2) * blackman(k), k = -31..31. sinc(k/2) is
        // zero for every even k except k = 0 - that is what makes it a
        // halfband: 33 non-zero taps out of 63.
        float sum = 0.f;
        for (int i = 0; i < kTaps; ++i)
        {
            const int k = i - kHalf;
            float h;
            if (k == 0)              h = 0.5f;
            else if ((k % 2) == 0)   h = 0.f;
            else
            {
                const float t = 3.14159265358979f * (float) k * 0.5f;
                h = 0.5f * std::sin (t) / t;
            }
            const float p = 6.28318530718f * (float) i / (float) (kTaps - 1);
            h *= 0.42f - 0.5f * std::cos (p) + 0.08f * std::cos (2.f * p);
            c[(size_t) i] = h;
            sum += h;
        }
        // normalise to unity gain at DC
        const float g = sum > 1.0e-9f ? 1.f / sum : 1.f;
        for (auto& v : c) v *= g;
    }

    void push (float x) { z[(size_t) w] = x; w = (w + 1) & kMask; }

    float tap() const
    {
        // x[n-i] lives at (w - 1 - i) & kMask
        const int base = (w - 1) & kMask;
        float acc = c[(size_t) kHalf] * z[(size_t) ((base - kHalf) & kMask)];
        for (int p = 0; p < kPairs; ++p)
        {
            const int i = p * 2;                 // 0, 2, .. 30
            const int j = kTaps - 1 - i;         // 62, 60, .. 32
            acc += c[(size_t) i] * (z[(size_t) ((base - i) & kMask)]
                                  + z[(size_t) ((base - j) & kMask)]);
        }
        return acc;
    }

    std::array<float, kTaps> c {};
    std::array<float, kSize> z {};
    int w = 0;
};

// ----------------------------------------------------------- decimator
// Cascaded 2:1 halfband stages: 2x uses one, 4x two, 8x three. The synth
// generates at factor * sampleRate and this brings it back down; there is
// no upsampling half because there is no input signal to upsample.
class Decimator
{
public:
    static constexpr int kChannels = 2;
    static constexpr int kMaxStages = 3;

    // maxOutputSamples is the largest base-rate block we will ever be asked
    // for; the intermediate buffers are sized once, here, off the audio thread.
    void prepare (int maxOutputSamples)
    {
        cap = std::max (32, maxOutputSamples);
        tmp[0].assign ((size_t) cap * 4, 0.f);
        tmp[1].assign ((size_t) cap * 2, 0.f);
        reset();
    }

    // 1, 2, 4 or 8. Allocation-free, so it is safe to call per block when
    // the user moves the quality selector.
    void setFactor (int f)
    {
        const int clamped = f >= 8 ? 8 : f >= 4 ? 4 : f >= 2 ? 2 : 1;
        if (clamped == factor) return;
        factor = clamped;
        stages = factor == 8 ? 3 : factor == 4 ? 2 : factor == 2 ? 1 : 0;
        reset();
    }

    int  getFactor() const { return factor; }
    int  capacity() const  { return cap; }
    void reset() { for (auto& c : chain) for (auto& s : c) s.reset(); }

    // Group delay in base-rate samples. Stage s runs at 2^(stages-s) times
    // the base rate, so its 31-sample delay is worth that much less here.
    int latencySamples() const
    {
        float lat = 0.f;
        for (int s = 0; s < stages; ++s)
            lat += (float) HalfbandDecimator::kHalf / (float) (1 << (stages - s));
        return (int) std::lround (lat);
    }

    // in holds n * factor samples; out receives n.
    void process (const float* in, float* out, int n, int channel)
    {
        if (stages == 0 || n <= 0)
        {
            for (int i = 0; i < n; ++i) out[i] = in[i];
            return;
        }
        const float* src = in;
        int len = n * factor;
        for (int s = 0; s < stages; ++s)
        {
            const int outLen = len / 2;
            float* dst = (s == stages - 1) ? out : tmp[(size_t) (s & 1)].data();
            chain[(size_t) channel][(size_t) s].process (src, dst, outLen);
            src = dst;
            len = outLen;
        }
    }

private:
    std::array<std::array<HalfbandDecimator, kMaxStages>, kChannels> chain;
    std::vector<float> tmp[2];
    int cap = 0, factor = 1, stages = 0;
};
} // namespace eighty
