// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Utilities to more easily use Gtk::EventController & subclasses like Gesture.
 */
/*
 * Authors:
 *   Daniel Boles <dboles.src+inkscape@gmail.com>
 *
 * Copyright (C) 2023 Daniel Boles
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_CONTROLLER_H
#define SEEN_UI_CONTROLLER_H

#include <gtkmm/gesture.h>

/// Utilities to more easily use Gtk::EventController & subclasses like Gesture.
namespace Inkscape::UI::Controller {

/// Helper to query if ModifierType state contains one or more of given flag(s).
// This will be needed in GTK4 as enums are scoped there, so bitwise is tougher.
[[nodiscard]] inline bool has_flag(Gdk::ModifierType const state,
                                   Gdk::ModifierType const flags)
    { return (state & flags) != Gdk::ModifierType{}; }

// migration aid for above, to later replace GdkModifierType w Gdk::ModifierType
[[nodiscard]] inline bool has_flag(GdkModifierType const state,
                                   GdkModifierType const flags)
    { return (state & flags) != GdkModifierType{}; }

// We add the requirement that slots return an EventSequenceState, which if itʼs
// not NONE we set on the controller. This makes it easier & less error-prone to
// migrate code that returned a bool whether GdkEvent is handled, to Controllers
// & their way of claiming the sequence if handled – as then we only require end
// users to change their returned type/value – rather than need them to manually
// call controller.set_state(), which is easy to forget & unlike a return cannot
// be enforced by the compiler. So… this wraps a callerʼs slot that returns that
// state & uses it, with a void-returning wrapper as thatʼs what GTK/mm expects.
template <typename Slot>
[[nodiscard]] auto use_state(Slot &&slot)
{
    return [slot = std::forward<Slot>(slot)](auto &controller, auto &&...args) {
        if constexpr (std::is_convertible_v<Slot, bool>) {
            if (!slot)
                return;
        }
        Gtk::EventSequenceState const state = slot(controller, std::forward<decltype(args)>(args)...);
        if (state != Gtk::EventSequenceState::NONE) {
            controller.set_state(state);
        }
    };
}

template <typename Slot, typename Controller>
[[nodiscard]] auto use_state(Slot &&slot, Controller &controller)
{
    return [&controller, new_slot = use_state(std::forward<Slot>(slot))](auto &&...args) {
        return new_slot(controller, std::forward<decltype(args)>(args)...);
    };
}

} // namespace Inkscape::UI::Controller

#endif // SEEN_UI_CONTROLLER_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
