#pragma once
#include <cmath>
#include "Oscillator.h"

namespace eighty
{
// Global modulation LFO (the CS-80's "sub oscillator"). One instance is
// shared by all voices, as on the original. Bipolar output.
class LFO
{
public:
    enum Wave { sine = 0, triangle, square, saw, sampleHold };

    void prepare (double sampleRate) { sr = (float) sampleRate; noise.seed (0xC0FFEE); }

    void setRate (float hz) { inc = hz / sr; }
    void setWave (int w)    { wave = (Wave) w; }

    void reset() { phase = 0.f; }

    float tick()
    {
        phase += inc;
        if (phase >= 1.f)
        {
            phase -= 1.f;
            held = noise.tick();
        }
        switch (wave)
        {
            case sine:     return std::sin (phase * 6.28318530718f);
            case triangle: return phase < 0.5f ? (4.f * phase - 1.f) : (3.f - 4.f * phase);
            case square:   return phase < 0.5f ? 1.f : -1.f;
            case saw:      return 2.f * phase - 1.f;
            case sampleHold: return held;
        }
        return 0.f;
    }

private:
    float sr = 44100.f, phase = 0.f, inc = 0.f, held = 0.f;
    Wave wave = sine;
    Noise noise;
};
} // namespace eighty
