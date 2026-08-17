#include "Trackpad.h"

#if defined (__APPLE__)

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

namespace eighty
{
// ---------------------------------------------------------------- macOS
// Touches arrive through a local NSEvent monitor rather than by subclassing
// or swizzling JUCE's view: -[NSEvent touchesMatchingPhase:inView:] works on
// any event that carried touch data, and the monitor sees those events on
// their way through the app. The one thing that genuinely has to happen on
// JUCE's own view is -setAllowedTouchTypes:, which is what makes the window
// server attach indirect touch data to our events in the first place.
struct TrackpadSource::Impl
{
    explicit Impl (TrackpadSource& o) : owner (o) {}

    ~Impl()
    {
        setEnabled (false);
        detach();
    }

    void attach (void* nativeView)
    {
        NSView* v = (NSView*) nativeView;
        if (v == view) return;
        detach();
        view = [v retain];
        if (view != nil)
            [view setAllowedTouchTypes: NSTouchTypeMaskIndirect];
    }

    void detach()
    {
        if (view == nil) return;
        [view setAllowedTouchTypes: 0];
        [view release];
        view = nil;
    }

    void setEnabled (bool e)
    {
        if (e == enabled) return;
        enabled = e;
        if (e)
        {
            install();
            applyFreeze (freeze);
        }
        else
        {
            applyFreeze (false);
            remove();
            pressure = 0.f;
            // One last empty frame, so whatever is listening sees every
            // finger lift rather than being left holding a chord.
            if (lastCount > 0)
            {
                lastCount = 0;
                TouchFrame f;
                f.timeSeconds = [NSDate timeIntervalSinceReferenceDate];
                if (owner.onFrame) owner.onFrame (f);
            }
        }
    }

    void setFreezePointer (bool f)
    {
        freeze = f;
        if (enabled) applyFreeze (f);
    }

    // ---- event capture -------------------------------------------------
    void install()
    {
        if (monitor != nil || view == nil) return;

        const NSEventMask mask = NSEventMaskGesture
                               | NSEventMaskPressure
                               | NSEventMaskMouseMoved
                               | NSEventMaskLeftMouseDown
                               | NSEventMaskLeftMouseDragged
                               | NSEventMaskLeftMouseUp;

        monitor = [[NSEvent addLocalMonitorForEventsMatchingMask: mask
                                                         handler: ^NSEvent* (NSEvent* e)
        {
            if ([e type] == NSEventTypePressure)
                pressure = (float) [e pressure];

            deliver (e);

            // While a gesture key is held the pad is an instrument, not a
            // pointer: swallow the clicks it would otherwise land on
            // whatever control the frozen pointer happens to be sitting on.
            const NSEventType t = [e type];
            if (t == NSEventTypeLeftMouseDown || t == NSEventTypeLeftMouseDragged
                || t == NSEventTypeLeftMouseUp)
                return nil;
            return e;
        }] retain];
    }

    void remove()
    {
        if (monitor == nil) return;
        [NSEvent removeMonitor: monitor];
        [monitor release];
        monitor = nil;
    }

    void deliver (NSEvent* e)
    {
        TouchFrame f;
        f.timeSeconds = [e timestamp];
        f.pressure = pressure;

        NSSet<NSTouch*>* set = [e touchesMatchingPhase: NSTouchPhaseTouching inView: nil];
        for (NSTouch* t in set)
        {
            if ([t type] != NSTouchTypeIndirect) continue;
            const NSPoint p = [t normalizedPosition];
            Touch touch;
            // The identity object is unique for the life of one finger and
            // is what lets a lane keep hold of "its" finger across frames.
            touch.id = (uint64_t) (uintptr_t) [t identity];
            touch.x  = (float) p.x;
            touch.y  = (float) p.y;
            f.touches.push_back (touch);
        }

        // Mouse-moved events fire constantly with nothing on the pad; only
        // pass a frame on when there is something to say.
        const int count = (int) f.touches.size();
        if (count == 0 && lastCount == 0) return;
        lastCount = count;

        if (owner.onFrame) owner.onFrame (f);
    }

    // ---- pointer freeze --------------------------------------------------
    static CGFloat screenHeight()
    {
        NSArray<NSScreen*>* screens = [NSScreen screens];
        if ([screens count] == 0) return 0;
        return NSMaxY ([[screens objectAtIndex: 0] frame]);
    }

    void applyFreeze (bool on)
    {
        if (on == frozen) return;

        if (on)
        {
            if (view == nil || [view window] == nil) return;

            const NSPoint mouse = [NSEvent mouseLocation];       // origin bottom-left
            const CGFloat h = screenHeight();
            saved = CGPointMake (mouse.x, h - mouse.y);          // CG is top-left

            // Touches only reach us while the pointer is over our window, so
            // park it on the editor before decoupling it - otherwise
            // freezing it somewhere else would silently kill the gesture.
            const NSRect wf = [[view window] convertRectToScreen:
                                   [view convertRect: [view bounds] toView: nil]];
            if (! NSPointInRect (mouse, wf))
                CGWarpMouseCursorPosition (CGPointMake (NSMidX (wf), h - NSMidY (wf)));

            CGAssociateMouseAndMouseCursorPosition (false);
            frozen = true;
        }
        else
        {
            CGAssociateMouseAndMouseCursorPosition (true);
            CGWarpMouseCursorPosition (saved);
            frozen = false;
        }
    }

    TrackpadSource& owner;
    NSView* view = nil;
    id monitor = nil;
    bool enabled = false, freeze = true, frozen = false;
    float pressure = 0.f;
    int lastCount = 0;
    CGPoint saved { 0, 0 };
};

TrackpadSource::TrackpadSource() : impl (std::make_unique<Impl> (*this)) {}
TrackpadSource::~TrackpadSource() = default;

void TrackpadSource::attach (void* v)        { impl->attach (v); }
void TrackpadSource::detach()                { impl->detach(); }
void TrackpadSource::setEnabled (bool e)     { impl->setEnabled (e); }
bool TrackpadSource::isEnabled() const       { return impl->enabled; }
void TrackpadSource::setFreezePointer (bool f) { impl->setFreezePointer (f); }
bool TrackpadSource::isAvailable() const     { return impl->view != nil; }
} // namespace eighty

#endif  // __APPLE__ - the stub for every other platform is in Trackpad.cpp
