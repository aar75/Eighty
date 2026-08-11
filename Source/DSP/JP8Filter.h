#pragma once
#include <cmath>
#include "Oversampling.h"

namespace eighty
{
// Jupiter-8 filter section: a gentle 6 dB/oct non-resonant high-pass into
// an IR3109-style four-stage ladder low-pass with one global resonance
// feedback path, switchable 12/24 dB by taking the output from stage 2 or
// stage 4 - which is literally what the hardware does.
//
// This topology, not the cutoff curve, is what separates a Jupiter-8 from
// the CS-80's state-variable filter:
//
//  * A ladder with global feedback loses DC gain as resonance rises
//    (1/(1+k)), which is why a Minimoog thins out when you crank it.
//    Roland compensated by boosting the input back into the chip as
//    resonance goes up, so the JP-8 keeps its body - the single most
//    identifiable thing about the filter, and the reason it gets called
//    the fattest of the classic Rolands.
//  * Four stages reach 180 degrees of phase shift, so 24 dB mode genuinely
//    self-oscillates. Two stages never do, so 12 dB mode resonates but
//    cannot howl - matching the real slope switch rather than bypassing a
//    stage.
//
// Solved zero-delay-feedback (TPT), so resonance stays accurate under fast
// modulation, with an ADAA saturator in the feedback path standing in for
// the OTA's soft clipping - it limits self-oscillation the way the chip
// does without spraying aliases back down the spectrum.
class JP8Filter
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }

    void reset()
    {
        z1 = z2 = z3 = z4 = 0.f;
        hz = 0.f;
        sat.reset();
    }

    void setParams (float hpHz, float lpHz, float res, bool slope24, float driveAmt)
    {
        hpHz = clamp (hpHz, 10.f, sr * 0.45f);
        lpHz = clamp (lpHz, 10.f, sr * 0.45f);

        const float gh = std::tan (pi * hpHz / sr);
        GH = gh / (1.f + gh);

        const float g = std::tan (pi * lpHz / sr);
        G  = g / (1.f + g);
        G2 = G * G;
        G4 = G2 * G2;

        use24 = slope24;
        // 4 is the self-oscillation threshold for a four-pole ladder, so the
        // top of the 24 dB range sings; the 2-pole tap tops out below it and
        // could not oscillate at any k anyway.
        k = res * res * (slope24 ? 4.4f : 1.9f);

        // Partial Q compensation. Full would be (1 + k); the real thing does
        // still lose a little level at extreme settings, so does this.
        inGain = 1.f + k * 0.65f;

        drive  = 1.f + driveAmt * 4.f;
        makeup = 1.f / std::sqrt (drive);
    }

    float process (float x)
    {
        // 6 dB/oct non-resonant high-pass (the JP-8's HPF slider)
        {
            const float v  = (x - hz) * GH;
            const float lp = v + hz;
            hz = lp + v;
            x -= lp;
        }

        x *= inGain;

        // Instantaneous response of each stage: y = G*x + S
        const float c  = 1.f - G;
        const float S1 = z1 * c, S2 = z2 * c, S3 = z3 * c, S4 = z4 * c;

        // Solve the loop for the tapped stage, then back out the input the
        // ladder actually has to see this sample.
        float u;
        if (use24)
        {
            const float sigma = ((S1 * G + S2) * G + S3) * G + S4;
            const float y = (G4 * x + sigma) / (1.f + k * G4);
            u = x - k * y;
        }
        else
        {
            const float sigma = S1 * G + S2;
            const float y = (G2 * x + sigma) / (1.f + k * G2);
            u = x - k * y;
        }

        u = sat.process (u * drive) * makeup;

        const float y1 = onePole (u,  z1);
        const float y2 = onePole (y1, z2);
        const float y3 = onePole (y2, z3);
        const float y4 = onePole (y3, z4);

        return use24 ? y4 : y2;
    }

private:
    float onePole (float x, float& z) const
    {
        const float v = (x - z) * G;
        const float y = v + z;
        z = y + v;
        return y;
    }

    static float clamp (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static constexpr float pi = 3.14159265358979f;

    float sr = 44100.f;
    float G = 0.5f, G2 = 0.25f, G4 = 0.0625f, GH = 0.01f;
    float k = 0.f, inGain = 1.f, drive = 1.f, makeup = 1.f;
    bool  use24 = true;
    float z1 = 0.f, z2 = 0.f, z3 = 0.f, z4 = 0.f, hz = 0.f;
    ADAATanh sat;
};
} // namespace eighty
