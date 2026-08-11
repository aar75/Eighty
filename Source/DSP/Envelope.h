#pragma once
#include <cmath>

namespace eighty
{
// Exponential-segment envelope with analog-style curvature (fast-start
// attack, RC-style decay/release). timeScale lets each voice card run
// slightly fast/slow for the drift model, and lets the CS-80's second
// channel run its whole envelope slower or faster than the first.
//
// The CS-80's filter envelope is not an ADSR: it starts from an Initial
// Level and attacks to an Attack Level rather than always running 0 -> 1.
// That is what setShape() adds. Raising IL shortens the sweep and dulls the
// tone, lowering it enriches it - the "inverted IL" behaviour players talk
// about. With il = 0 and al = 1 this reduces exactly to the old ADSR.
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
        // A release time edited *while* a note is releasing used to do
        // nothing, because the coefficient was only ever computed at
        // note-off. Refresh it in place instead.
        if (stage == Stage::release)
            releaseCoef = coefFor (releaseT);
    }

    // CS-80 filter EG: il = level the envelope starts from, al = level the
    // attack segment peaks at. Both 0..1.
    void setShape (float initialLevel, float attackLevel)
    {
        il = initialLevel;
        al = attackLevel;
    }

    void noteOn()
    {
        // A fresh note starts from the initial level; a retrigger of a still
        // sounding voice continues from where it is, which is what keeps
        // fast repeats from clicking.
        if (stage == Stage::idle)
            level = il;
        stage = Stage::attack;
        attackCoef = coefFor (attackT);
        if (level >= al)
        {
            stage = Stage::decay;
            decayCoef = coefFor (decayT);
        }
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
                // Aim above the target so the attack has an analog "punch"
                // shape rather than an exponential crawl into it.
                level += (al * 1.28f - level) * attackCoef;
                if (level >= al) { level = al; stage = Stage::decay; decayCoef = coefFor (decayT); }
                break;
            case Stage::decay:
                level += (sustainL * al - level) * decayCoef;
                if (level - sustainL * al < 0.0005f) stage = Stage::sustain;
                break;
            case Stage::sustain:
                level = sustainL * al;
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
    float il = 0.f, al = 1.f;
};

// Two-stage attack/decay envelope with no sustain, retriggered by every key
// press. The CS-80's ring modulator has one of these driving both its depth
// and - the interesting part - the speed of its carrier.
class ADEnv
{
public:
    void prepare (double sampleRate) { sr = (float) sampleRate; }

    void setParams (float a, float d)
    {
        attackT = a;
        decayT  = d;
        if (stage == 2) decayCoef = coefFor (decayT);
    }

    void noteOn()
    {
        stage = 1;
        attackCoef = coefFor (attackT);
        decayCoef  = coefFor (decayT);
    }

    void kill() { stage = 0; level = 0.f; }
    float value() const { return level; }

    float tick()
    {
        if (stage == 1)
        {
            level += (1.22f - level) * attackCoef;
            if (level >= 1.f) { level = 1.f; stage = 2; }
        }
        else if (stage == 2)
        {
            level += (0.f - level) * decayCoef;
            if (level < 0.0001f) { level = 0.f; stage = 0; }
        }
        return level;
    }

private:
    float coefFor (float seconds) const
    {
        const float samples = seconds * sr;
        if (samples < 1.f) return 1.f;
        return 1.f - std::exp (-4.2f / samples);
    }

    int stage = 0;              // 0 idle, 1 attack, 2 decay
    float sr = 44100.f, level = 0.f;
    float attackT = 0.01f, decayT = 0.5f;
    float attackCoef = 0.01f, decayCoef = 0.01f;
};
} // namespace eighty
