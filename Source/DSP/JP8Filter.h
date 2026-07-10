#pragma once
#include <cmath>

namespace eighty
{
// Jupiter-8 filter section: non-resonant 2-pole high-pass (the JP-8's HPF
// slider) into an IR3109-style low-pass switchable between 12 and 24 dB/oct
// with resonance. Modeled as TPT state-variable stages; in 24 dB mode a
// second, lightly-damped stage is cascaded after the resonant one.
class JP8Filter
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset() { h1 = h2 = a1 = a2 = b1 = b2 = 0.f; }

    void setParams (float hpHz, float lpHz, float res, bool slope24)
    {
        hpHz = clamp (hpHz, 10.f, sr * 0.45f);
        lpHz = clamp (lpHz, 10.f, sr * 0.45f);
        gH = std::tan (pi * hpHz / sr);
        gL = std::tan (pi * lpHz / sr);
        kH = 1.f / 0.55f;
        // the 24 dB mode self-oscillates sooner on the real unit
        const float maxQ = slope24 ? 14.f : 10.f;
        kL = 1.f / (0.5f + res * res * maxQ);
        use24 = slope24;
    }

    float process (float x)
    {
        // gentle input rounding (IR3109 OTA character, cleaner than the CS-80)
        x = std::tanh (x * 1.2f) * 0.9f;

        { // HP stage, fixed damping
            float hp = (x - (kH + gH) * h1 - h2) / (1.f + gH * (kH + gH));
            float bp = gH * hp + h1;
            float lp = gH * bp + h2;
            h1 = gH * hp + bp;
            h2 = gH * bp + lp;
            x = hp;
        }
        { // resonant LP stage
            float hp = (x - (kL + gL) * a1 - a2) / (1.f + gL * (kL + gL));
            float bp = gL * hp + a1;
            float lp = gL * bp + a2;
            a1 = gL * hp + bp;
            a2 = gL * bp + lp;
            x = lp;
        }
        if (use24)
        { // second LP stage, light damping
            const float k2 = 1.f / 0.55f;
            float hp = (x - (k2 + gL) * b1 - b2) / (1.f + gL * (k2 + gL));
            float bp = gL * hp + b1;
            float lp = gL * bp + b2;
            b1 = gL * hp + bp;
            b2 = gL * bp + lp;
            x = lp;
        }
        return x;
    }

private:
    static float clamp (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static constexpr float pi = 3.14159265358979f;

    float sr = 44100.f;
    float gH = 0.1f, kH = 1.8f, gL = 0.5f, kL = 1.f;
    bool use24 = true;
    float h1 = 0.f, h2 = 0.f, a1 = 0.f, a2 = 0.f, b1 = 0.f, b2 = 0.f;
};
} // namespace eighty
