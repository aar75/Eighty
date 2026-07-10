#pragma once
#include <cmath>

namespace eighty
{
// Exponential-segment ADSR with analog-style curvature (fast-start attack,
// RC-style decay/release). timeScale lets each voice card run slightly
// fast/slow for the drift model.
class ADSR
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; }

    void setParams (float a, float d, float s, float r, float timeScale = 1.f)
    {
        attackT  = a * timeScale;
        decayT   = d * timeScale;
        sustainL = s;
        releaseT = r * timeScale;
    }

    void noteOn()
    {
        stage = Stage::attack;
        attackCoef  = coefFor (attackT);
    }

    void noteOff()
    {
        if (stage != Stage::idle)
        {
            stage = Stage::release;
            releaseCoef = coefFor (releaseT);
        }
    }

    void kill() { stage = Stage::idle; level = 0.f; }
    bool isActive() const { return stage != Stage::idle; }
    bool isReleasing() const { return stage == Stage::release; }
    float value() const { return level; }

    float tick()
    {
        switch (stage)
        {
            case Stage::attack:
                // Aim above 1.0 so the attack has an analog "punch" shape
                level += (1.28f - level) * attackCoef;
                if (level >= 1.f) { level = 1.f; stage = Stage::decay; decayCoef = coefFor (decayT); }
                break;
            case Stage::decay:
                level += (sustainL - level) * decayCoef;
                if (level - sustainL < 0.0005f) stage = Stage::sustain;
                break;
            case Stage::sustain:
                level = sustainL;
                break;
            case Stage::release:
                level += (0.f - level) * releaseCoef;
                if (level < 0.0001f) { level = 0.f; stage = Stage::idle; }
                break;
            case Stage::idle:
            default:
                break;
        }
        return level;
    }

private:
    enum class Stage { idle, attack, decay, sustain, release };

    float coefFor (float seconds) const
    {
        float samples = seconds * sr;
        if (samples < 1.f) return 1.f;
        return 1.f - std::exp (-4.2f / samples); // ~settles within the time
    }

    Stage stage = Stage::idle;
    float sr = 44100.f;
    float level = 0.f;
    float attackT = 0.01f, decayT = 0.3f, sustainL = 0.8f, releaseT = 0.3f;
    float attackCoef = 0.01f, decayCoef = 0.01f, releaseCoef = 0.01f;
};
} // namespace eighty
