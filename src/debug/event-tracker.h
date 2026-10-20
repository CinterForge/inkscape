// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::EventTracker - semi-automatically track event lifetimes
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2005 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DEBUG_EVENT_TRACKER_H
#define SEEN_INKSCAPE_DEBUG_EVENT_TRACKER_H

#include "debug/logger.h"

namespace Inkscape {

namespace Debug {

#ifdef NDEBUG
// Make event tracking a no-op for non-debug builds
template <typename = void> struct EventTracker {
    template <typename... Args> EventTracker(Args &&...) {}
    template <typename, typename... Args> void set(Args &&...) {}
    void clear() {}
};
#else

struct NoInitialEvent {};

template <typename Event=NoInitialEvent> class EventTracker;

class EventTrackerBase {
public:
    virtual ~EventTrackerBase() {
        if (_active) {
            Logger::finish();
        }
    }

    template <typename EventType, typename... Args>
    inline void set(Args&&... args) {
        if (_active) {
            Logger::finish();
        }
        Logger::start<EventType>(std::forward<Args>(args)...);
        _active = true;
    }

    void clear() {
        if (_active) {
            Logger::finish();
            _active = false;
        }
    }

protected:
    EventTrackerBase(bool active) : _active(active) {}

private:
    EventTrackerBase(EventTrackerBase const &) = delete; // no copy
    void operator=(EventTrackerBase const &) = delete; // no assign
    bool _active;
};

template <typename EventType> class EventTracker : public EventTrackerBase {
public:
    template <typename... Args>
    EventTracker(Args&&... args) : EventTrackerBase(true) {
        Logger::start<EventType>(std::forward<Args>(args)...);
    }
};

template <> class EventTracker<NoInitialEvent> : public EventTrackerBase {
public:
    EventTracker() : EventTrackerBase(false) {}
};

#endif

}

}

#endif
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
