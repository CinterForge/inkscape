// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for selection tied to the application and without GUI.
 *
 * Copyright (C) 2023 Martin Owens
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_HELPER_GUI_H
#define INK_ACTIONS_HELPER_GUI_H

#include "actions-helper.h"

class InkscapeWindow;

void activate_any_actions(action_vector_t const &actions, Glib::RefPtr<Gio::Application> app, InkscapeWindow *win, SPDocument *doc);

#endif // INK_ACTIONS_HELPER_GUI_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
