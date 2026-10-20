// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Matthew Jakeman <mjakeman26@outlook.co.nz>
 *   PBS <pbs3141@gmail.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_UI_WIDGET_EVENTS_ENUMS_H
#define INKSCAPE_UI_WIDGET_EVENTS_ENUMS_H

namespace Inkscape {

/**
 * The type of a CanvasEvent.
 */
enum class EventType
{
    ENTER,
    LEAVE,
    MOTION,
    BUTTON_PRESS,
    BUTTON_RELEASE,
    KEY_PRESS,
    KEY_RELEASE,
    SCROLL,
    NUM_EVENTS
};

/**
 * A mask representing a subset of EventTypes.
 */
class EventMask
{
public:
    EventMask() = default;
    constexpr EventMask(EventType type) : _mask(1 << (int)type) {} // allow implicit conversion - unsurprising

    constexpr operator bool() const { return _mask; }
    constexpr EventMask operator~() const { return EventMask(~_mask); }
    constexpr EventMask operator&(EventMask other) const { return EventMask(_mask & other._mask); }
    constexpr EventMask operator|(EventMask other) const { return EventMask(_mask | other._mask); }

private:
    unsigned _mask = 0;
    static_assert(std::numeric_limits<decltype(_mask)>::digits >= (int)EventType::NUM_EVENTS);

    explicit constexpr EventMask(unsigned mask) : _mask(mask) {}
};

constexpr EventMask operator~(EventType a) { return ~EventMask(a); }
constexpr EventMask operator&(EventType a, EventMask b) { return EventMask(a) & b; }
constexpr EventMask operator|(EventType a, EventMask b) { return EventMask(a) | b; }

} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_EVENTS_ENUMS_H
