#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include "Trackpad.h"

namespace eighty
{
// Turns trackpad fingers into playing gestures - a strum across the notes
// you are holding, or the CS-80's ribbon.
//
// Deliberately free of both JUCE and AppKit: normalised finger positions in,
// notes out through callbacks. The platform layer under it and the synth
// over it can then be reasoned about separately, and the geometry here -
// where the strings are, what counts as a stroke, what a lift means - is
// readable on its own.
//
// A "lane" is one finger. There are two, because two is what a hand on a
// trackpad has room for and because two is how many engines this synth has:
// with a finger per engine the CS-80 and the JP-8 strum independently.
class GestureEngine
{
public:
    enum class Mode { off, strum, ribbon };

    // Cross-axis destinations (`cross`), matching the CROSS AXIS parameter.
    enum Cross { crossOff = 0, crossVelocity, crossBright, crossPressure };

    struct Config
    {
        int   span = 2;            // octaves the held chord is laid across the pad
        float force = 0.6f;        // how much stroke speed drives velocity
        int   axis = 0;            // 0 = strum along x, 1 = along y
        int   cross = crossVelocity;
        int   lift = 2;            // 0 ring, 1 damp, 2 decide from the gesture
        int   fingers = 0;         // 0 = a finger per engine, 1 = both engines
        bool  pattern = false;     // strokes clock the arp/sequencer instead
        float ribbonRange = 12.f;  // semitones at either extreme of the pad
        int   ribbonMode = 0;      // 0 relative to touch-down, 1 absolute
    };

    struct Callbacks
    {
        // Fills `dest` with the notes to strum, low to high, already spread
        // over `octaves`; returns how many. This is the engine's latched
        // note set, which is why Hold and the arpeggiator feed the strum for
        // free.
        std::function<int (int* dest, int maxDest, int octaves)> chord;

        // model: 0 = CS-80, 1 = JP-8, -1 = follow the engine mode
        std::function<void (int lane, int note, float vel, int model)> note;
        std::function<void (int lane, int dir)> patternStep;
        std::function<void (int lane, bool damp)> laneOff;
        std::function<void (int model, float semis)> ribbon;
        std::function<void (int dest, float value)> cross;
        std::function<void (float value)> pressure;
    };

    Config cfg;
    Callbacks cb;

    Mode mode() const { return current; }

    void setMode (Mode m)
    {
        if (m == current) return;
        // Leaving a mode lets go of everything it was holding - a lane that
        // is still ringing when the key comes up is handled by the engine's
        // disarm, not left dangling here.
        for (int i = 0; i < kLanes; ++i)
            if (lanes[i].active) lanes[i] = Lane();
        if (current == Mode::ribbon && cb.ribbon)
            for (int i = 0; i < kLanes; ++i) cb.ribbon (modelFor (i), 0.f);
        current = m;
    }

    bool anyFingerDown() const
    {
        for (auto& l : lanes) if (l.active) return true;
        return false;
    }

    // A read-only look at what one finger is doing, for the touch monitor.
    // Reported rather than recomputed, so the display shows what was actually
    // played instead of a second opinion about it.
    struct LaneState
    {
        bool  active = false;
        float pos = 0.f, cross = 0.f, speed = 0.f;
        int   index = -1;        // string / step slot under the finger
        int   model = -1;        // engine it is driving
        int   note = -1;         // last note it struck
        float vel = 0.f;
        float semis = 0.f;       // ribbon offset
        float origin = 0.f;      // where the finger landed (the ribbon's reference)
    };

    static constexpr int laneCount() { return kLanes; }

    // How many crossings the pad is divided into right now. The monitor draws
    // the grid the strum actually plays rather than a second guess at it.
    int slots (int chordSize) const
    { return cfg.pattern ? 8 * std::max (1, cfg.span) : chordSize; }

    LaneState laneState (int lane) const
    {
        LaneState s;
        if (lane < 0 || lane >= kLanes) return s;
        const Lane& L = lanes[lane];
        s.active = L.active;
        s.pos = L.pos;
        s.cross = L.cross;
        s.speed = L.speed;
        s.index = L.index;
        s.model = modelFor (lane);
        s.note = L.lastNote;
        s.vel = L.lastVel;
        s.semis = L.lastSemis;
        s.origin = cfg.ribbonMode == 1 ? 0.5f : L.origin;
        return s;
    }

    float lastPressure() const { return pressure; }

    void frame (const TouchFrame& f)
    {
        if (current == Mode::off) return;

        // 1. lanes whose finger has gone
        for (int i = 0; i < kLanes; ++i)
        {
            if (! lanes[i].active) continue;
            bool stillDown = false;
            for (auto& t : f.touches)
                if (t.id == lanes[i].id) { stillDown = true; break; }
            if (! stillDown) end (i, f.timeSeconds);
        }

        // 2. lanes that already own their finger
        for (auto& t : f.touches)
            for (int i = 0; i < kLanes; ++i)
                if (lanes[i].active && lanes[i].id == t.id)
                    move (i, t, f.timeSeconds);

        // 3. new fingers claim a free lane, leftmost first, so that with a
        //    finger per engine the hands land where you put them rather than
        //    in whatever order the window server happened to report.
        const Touch* fresh[8];
        int numFresh = 0;
        for (auto& t : f.touches)
        {
            bool known = false;
            for (int i = 0; i < kLanes && ! known; ++i)
                known = lanes[i].active && lanes[i].id == t.id;
            if (! known && numFresh < 8) fresh[numFresh++] = &t;
        }
        std::sort (fresh, fresh + numFresh,
                   [] (const Touch* a, const Touch* b) { return a->x < b->x; });

        for (int n = 0; n < numFresh; ++n)
            for (int i = 0; i < kLanes; ++i)
                if (! lanes[i].active) { begin (i, *fresh[n], f.timeSeconds); break; }

        // Force Touch, where the hardware has it: one pressure for the pad,
        // so it lands on channel aftertouch rather than per note.
        pressure = anyFingerDown() ? f.pressure : 0.f;
        if (cfg.cross == crossPressure && cb.pressure && anyFingerDown())
            cb.pressure (f.pressure);
    }

private:
    static constexpr int   kLanes = 2;
    static constexpr int   kMaxSlots = 128;
    static constexpr float kStillSpeed = 0.4f;    // pad-widths per second
    static constexpr double kStillTime = 0.12;    // resting this long = damped

    struct Lane
    {
        bool     active = false;
        uint64_t id = 0;
        float    pos = 0.f, cross = 0.f, origin = 0.f;
        float    speed = 0.f;
        double   time = 0.0, movedAt = 0.0;
        int      index = -1;         // string the finger last crossed
        int      dir = 0;            // which way it was last travelling
        int      lastNote = -1;      // reported to the touch monitor
        float    lastVel = 0.f, lastSemis = 0.f;
    };

    float axisPos  (const Touch& t) const { return cfg.axis == 0 ? t.x : t.y; }
    float crossPos (const Touch& t) const { return cfg.axis == 0 ? t.y : t.x; }

    // A finger per engine only means anything once there are two fingers
    // down - a single finger plays whatever the engine mode says, so
    // one-handed strumming still works in CS-80, JP-8, Split and Layer.
    int modelFor (int lane) const
    {
        if (cfg.fingers != 0) return -1;
        const bool two = lanes[0].active && lanes[1].active;
        return two ? (lane == 0 ? 0 : 1) : -1;
    }

    void begin (int i, const Touch& t, double time)
    {
        Lane& L = lanes[i];
        L = Lane();
        L.active = true;
        L.id = t.id;
        L.pos = L.origin = axisPos (t);
        L.cross = crossPos (t);
        L.time = L.movedAt = time;

        if (current == Mode::ribbon) { applyRibbon (i); return; }
        strumTo (i);          // putting a finger down plucks the string under it
    }

    void move (int i, const Touch& t, double time)
    {
        Lane& L = lanes[i];
        const float p = axisPos (t);
        const double dt = std::fmax (1.0e-3, time - L.time);
        const float inst = (float) (std::fabs (p - L.pos) / dt);

        L.speed += (inst - L.speed) * 0.5f;      // one-pole, so a stroke has weight
        if (L.speed > kStillSpeed) L.movedAt = time;
        L.pos = p;
        L.cross = crossPos (t);
        L.time = time;

        if (current == Mode::ribbon) { applyRibbon (i); return; }

        if (cfg.cross == crossBright && cb.cross) cb.cross (cfg.cross, L.cross);
        strumTo (i);
    }

    void end (int i, double time)
    {
        Lane& L = lanes[i];
        L.active = false;

        if (current == Mode::ribbon)
        {
            L.lastSemis = 0.f;
            if (cb.ribbon) cb.ribbon (modelFor (i), 0.f);
            return;
        }

        // What lifting means. "Ring" leaves the strings sounding and "Damp"
        // lets go; on the default, the gesture decides - lift mid-stroke and
        // it rings on, rest your finger on the strings first and it damps,
        // which is what a hand on a real instrument does.
        bool damp = cfg.lift == 1;
        if (cfg.lift == 2) damp = (time - L.movedAt) >= kStillTime;
        if (cb.laneOff) cb.laneOff (i, damp);
    }

    // Velocity of one stroke: how fast the finger is travelling, blended
    // against a fixed value by FORCE so the feature is usable either as a
    // dynamic instrument or as an even, machine-like strum.
    float velocity (const Lane& L) const
    {
        const float sp = std::clamp (L.speed / 3.f, 0.f, 1.f);
        const float dynamic = 0.15f + 0.85f * sp;
        float v = 0.75f + (dynamic - 0.75f) * std::clamp (cfg.force, 0.f, 1.f);
        if (cfg.cross == crossVelocity) v *= 0.35f + 0.65f * L.cross;
        return std::clamp (v, 0.05f, 1.f);
    }

    void strumTo (int lane)
    {
        Lane& L = lanes[lane];

        int notes[kMaxSlots];
        const int n = (! cfg.pattern && cb.chord)
                    ? cb.chord (notes, kMaxSlots, cfg.span) : 0;

        // Chord mode lays one string per note; hand-clocking a pattern has
        // no notes to lay out, so it uses a fixed grid of crossings instead
        // and SPAN sets how densely they sit.
        const int slotCount = slots (n);
        if (slotCount <= 0) { L.index = -1; return; }

        int idx = (int) (L.pos * (float) slotCount);
        idx = std::clamp (idx, 0, slotCount - 1);
        if (idx == L.index) return;

        const int from = L.index;
        const int dir = (from < 0 || idx > from) ? 1 : -1;

        // Turning around re-strikes the string the finger is sitting on, the
        // way a pick does when it changes direction. Without it, stopping at
        // the top of a sweep and running back down starts one string late -
        // the note you were parked on is silently skipped. Only in chord
        // mode: a hand-clocked pattern is a clock, and one crossing there has
        // to stay worth exactly one step.
        const bool turned = from >= 0 && L.dir != 0 && dir != L.dir && ! cfg.pattern;
        L.index = idx;
        // Only a real crossing sets the direction. Putting a finger down
        // plucks a string without travelling anywhere, and if that counted as
        // heading right, the first stroke leftwards would read as a turn and
        // strike the same string twice.
        if (from >= 0) L.dir = dir;

        // A quick sweep crosses several strings inside one event: sound every
        // one it passed, in order, rather than only where the finger landed.
        // Otherwise a fast strum plays two notes instead of six.
        const int first = from < 0 ? idx : (turned ? from : from + dir);
        for (int s = first, guard = 0; guard <= slotCount + 1; s += dir, ++guard)
        {
            if (s < 0 || s >= slotCount) break;
            if (cfg.pattern) { if (cb.patternStep) cb.patternStep (lane, dir); }
            else
            {
                L.lastNote = notes[s];
                L.lastVel = velocity (L);
                if (cb.note) cb.note (lane, L.lastNote, L.lastVel, modelFor (lane));
            }
            if (s == idx) break;
        }
    }

    void applyRibbon (int lane)
    {
        if (! cb.ribbon) return;
        const Lane& L = lanes[lane];
        const float ref = cfg.ribbonMode == 1 ? 0.5f : L.origin;
        const float semis = std::clamp ((L.pos - ref) * 2.f * cfg.ribbonRange,
                                        -cfg.ribbonRange, cfg.ribbonRange);
        lanes[lane].lastSemis = semis;
        cb.ribbon (modelFor (lane), semis);
    }

    Lane lanes[kLanes];
    Mode current = Mode::off;
    float pressure = 0.f;
};
} // namespace eighty
