// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Fine-grained modifier tracker for event handling.
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/tool/modifier-tracker.h"
#include "ui/widget/events/canvas-event.h"

namespace Inkscape::UI {

void ModifierTracker::event(CanvasEvent const &event)
{
    inspect_event(event,
    [&] (KeyPressEvent const &event) {
        switch (event.keyval) {
        case GDK_KEY_Shift_L:
            _left_shift = true;
            break;
        case GDK_KEY_Shift_R:
            _right_shift = true;
            break;
        case GDK_KEY_Control_L:
            _left_ctrl = true;
            break;
        case GDK_KEY_Control_R:
            _right_ctrl = true;
            break;
        case GDK_KEY_Alt_L:
            _left_alt = true;
            break;
        case GDK_KEY_Alt_R:
            _right_alt = true;
            break;
        }
    },
    [&] (KeyReleaseEvent const &event) {
        switch (event.keyval) {
        case GDK_KEY_Shift_L:
            _left_shift = false;
            break;
        case GDK_KEY_Shift_R:
            _right_shift = false;
            break;
        case GDK_KEY_Control_L:
            _left_ctrl = false;
            break;
        case GDK_KEY_Control_R:
            _right_ctrl = false;
            break;
        case GDK_KEY_Alt_L:
            _left_alt = false;
            break;
        case GDK_KEY_Alt_R:
            _right_alt = false;
            break;
        }
    },
    [&] (CanvasEvent const &event) {}
    );
}

} // namespace Inkscape::UI

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
