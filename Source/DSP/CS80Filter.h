#pragma once
#include <cmath>

namespace eighty
{
// CS-80 filter section: a 12 dB/oct high-pass followed by a 12 dB/oct
// resonant low-pass (the CS-80 used two cascaded OTA-based 2-pole filters
// per channel), with a soft-saturating input stage for the "warm" drive.
// Implemented as two TPT (topology-preserving transform) state-variable
// filters, which stay stable under fast modulation.
class CS80Filter
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        reset();
    }

    void reset()
    {
        hp1 = hp2 = lp1 = lp2 = 0.f;
    }

    // res: 0..1, drive: 0..1
    void setParams (float hpHz, float lpHz, float res, float driveAmt)
    {
        hpHz = clamp (hpHz, 10.f, sr * 0.45f);
        lpHz = clamp (lpHz, 10.f, sr * 0.45f);

        gH = std::tan (pi * hpHz / sr);
        gL = std::tan (pi * lpHz / sr);

        // HPF has gentle fixed damping; LPF resonance maps to Q ~0.5 .. ~12
        kH = 1.f / 0.6f;
        float q = 0.5f + res * res * 11.5f;
        kL = 1.f / q;

        drive = 1.f + driveAmt * 4.f;
        makeup = 1.f / std::sqrt (drive);
    }

    float process (float x)
    {
        // Input saturation (per-voice, like an overdriven OTA input stage)
        x = std::tanh (x * drive) * makeup;

        // 2-pole TPT SVF, high-pass output
        {
            float hp = (x - (kH + gH) * hp1 - hp2) / (1.f + gH * (kH + gH));
            float bp = gH * hp + hp1;
            float lp = gH * bp + hp2;
            hp1 = gH * hp + bp;
            hp2 = gH * bp + lp;
            x = hp;
        }
        // 2-pole TPT SVF, low-pass output with resonance
        {
            float hp = (x - (kL + gL) * lp1 - lp2) / (1.f + gL * (kL + gL));
            float bp = gL * hp + lp1;
            float lp = gL * bp + lp2;
            lp1 = gL * hp + bp;
            lp2 = gL * bp + lp;
            x = lp;
        }
        return x;
    }

private:
    static float clamp (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static constexpr float pi = 3.14159265358979f;

    float sr = 44100.f;
    float gH = 0.1f, kH = 1.6f, gL = 0.5f, kL = 1.f;
    float drive = 1.f, makeup = 1.f;
    float hp1 = 0.f, hp2 = 0.f, lp1 = 0.f, lp2 = 0.f;
};
} // namespace eighty
