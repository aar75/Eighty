#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace eighty
{
// One finger resting on the trackpad. Positions are normalised over the pad
// itself, *not* over the screen: x runs 0 (left) to 1 (right), y runs 0
// (near edge) to 1 (far edge), the way macOS reports them. The pointer is
// irrelevant here - this is the pad as a surface, not as a mouse.
struct Touch
{
    uint64_t id = 0;
    float x = 0.f, y = 0.f;
};

struct TouchFrame
{
    std::vector<Touch> touches;
    float  pressure = 0.f;      // Force Touch, 0..1 (0 where unsupported)
    double timeSeconds = 0.0;
};

// Raw indirect (trackpad) touch capture.
//
// JUCE only ever hands a trackpad to a plugin as a mouse - one cursor, no
// fingers - so this drops to AppKit's NSTouch API, which reports every
// finger on the pad with its own normalised position and identity. That is
// the whole reason strumming can work at all: a strum is a position along a
// surface, not a click.
//
// Only macOS does anything; the stub elsewhere reports isAvailable() false
// and never calls back, so everything above it compiles and runs with the
// feature simply absent rather than #ifdef'd through the editor.
class TrackpadSource
{
public:
    TrackpadSource();
    ~TrackpadSource();

    // `nativeView` is the editor peer's native handle (an NSView* on macOS).
    // Safe to call repeatedly with the same view.
    void attach (void* nativeView);
    void detach();

    // Capture only runs while enabled, so the event monitor and the pointer
    // freeze below cost nothing until a gesture key is actually held down.
    void setEnabled (bool);
    bool isEnabled() const;

    // While the trackpad is an instrument it should not also be flinging the
    // pointer across the screen (and off our window, which would cut the
    // touch stream off at the source). This parks the pointer over the
    // editor and decouples it until capture ends.
    void setFreezePointer (bool);

    bool isAvailable() const;

    // Called on the message thread, once per touch event.
    std::function<void (const TouchFrame&)> onFrame;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    TrackpadSource (const TrackpadSource&) = delete;
    TrackpadSource& operator= (const TrackpadSource&) = delete;
};
} // namespace eighty
