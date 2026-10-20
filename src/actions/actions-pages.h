// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for pages
 *
 * Copyright (C) 2021 Martin Owens
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_PAGES_H
#define INK_ACTIONS_PAGES_H

class SPDocument;
class InkscapeWindow;

void add_actions_pages(SPDocument* doc);
void add_actions_page_tools(InkscapeWindow* win);

#endif // INK_ACTIONS_PAGES_H

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
