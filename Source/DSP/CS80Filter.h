#pragma once
#include <cmath>
#include "Oversampling.h"

namespace eighty
{
// CS-80 filter section: a 12 dB/oct high-pass followed by a 12 dB/oct
// low-pass, both resonant, with a soft-saturating input stage for the
// "warm" drive. Two cascaded OTA-based 2-pole state-variable filters per
// channel is what the real machine has, and resonance on *both* is where
// its bandpass / chime / vocal patches come from - close the LPF onto an
// opened resonant HPF and you get the formant character the CS-80 is
// known for.
//
// Deliberately cannot self-oscillate: Yamaha put limiting resistors in the
// resonance path so a hard-played velocity-sensitive keyboard could never
// tip the filter into howling. The Q ceilings below are that limit.
//
// Implemented as TPT (topology-preserving transform) SVFs, which stay
// stable under fast modulation, with ADAA on the drive stage so pushing it
// does not fold aliases back down the spectrum.
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
        sat.reset();
    }

    // res / hpRes / drive: 0..1
    void setParams (float hpHz, float lpHz, float res, float driveAmt, float hpRes)
    {
        hpHz = clamp (hpHz, 10.f, sr * 0.45f);
        lpHz = clamp (lpHz, 10.f, sr * 0.45f);

        gH = std::tan (pi * hpHz / sr);
        gL = std::tan (pi * lpHz / sr);

        // Both stages are resonant. The HPF tops out lower than the LPF -
        // on the original it is the gentler of the two - and neither reaches
        // the self-oscillation threshold.
        const float qH = 0.55f + hpRes * hpRes * 6.f;
        kH = 1.f / qH;
        const float qL = 0.5f + res * res * 11.5f;
        kL = 1.f / qL;

        drive = 1.f + driveAmt * 4.f;
        makeup = 1.f / std::sqrt (drive);
    }

    float process (float x)
    {
        // Input saturation (per-voice, like an overdriven OTA input stage)
        x = sat.process (x * drive) * makeup;

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
    float gH = 0.1f, kH = 1.8f, gL = 0.5f, kL = 1.f;
    float drive = 1.f, makeup = 1.f;
    float hp1 = 0.f, hp2 = 0.f, lp1 = 0.f, lp2 = 0.f;
    ADAATanh sat;
};
} // namespace eighty
