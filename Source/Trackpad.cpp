#include "Trackpad.h"

// Non-Apple trackpad source. There is no portable way to read individual
// fingers off an indirect touch device, so the class exists and reports
// itself unavailable: the editor asks isAvailable() once and the whole
// gesture layer stays dark, rather than being conditionally compiled out of
// the middle of the UI. Trackpad.mm is the macOS half.
#if ! defined (__APPLE__)

namespace eighty
{
struct TrackpadSource::Impl {};

TrackpadSource::TrackpadSource() = default;
TrackpadSource::~TrackpadSource() = default;

void TrackpadSource::attach (void*)          {}
void TrackpadSource::detach()                {}
void TrackpadSource::setEnabled (bool)       {}
bool TrackpadSource::isEnabled() const       { return false; }
void TrackpadSource::setFreezePointer (bool) {}
bool TrackpadSource::isAvailable() const     { return false; }
} // namespace eighty

#endif
