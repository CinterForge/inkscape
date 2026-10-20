// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for dialogs.
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_DIALOGS_H
#define INK_ACTIONS_DIALOGS_H

class InkscapeApplication;
class InkscapeWindow;

// Standard function to add actions.
void add_actions_dialogs(InkscapeApplication *app);
void add_actions_dialogs(InkscapeWindow *win);

#endif // INK_ACTIONS_DIALOGS_H

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
