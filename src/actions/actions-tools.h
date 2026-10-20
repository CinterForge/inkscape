// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for switching tools. Also includes functions to set and get active tool.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_TOOLS_H
#define INK_ACTIONS_TOOLS_H

#include <glibmm.h>
#include <2geom/point.h>

class InkscapeWindow;
class SPDesktop;
class SPItem;

Glib::ustring get_active_tool(InkscapeWindow *win);
int get_active_tool_enum(InkscapeWindow *win);

void set_active_tool(InkscapeWindow* win, Glib::ustring const &tool);
void set_active_tool(InkscapeWindow* win, SPItem *item, Geom::Point const p);

void open_tool_preferences(InkscapeWindow* win, Glib::ustring const &tool);

// Deprecated: Long term goal to remove SPDesktop.
Glib::ustring get_active_tool(SPDesktop *desktop);
int get_active_tool_enum(SPDesktop *desktop);

void set_active_tool(SPDesktop *desktop, Glib::ustring const &tool);
void set_active_tool(SPDesktop *desktop, SPItem *item, Geom::Point const p);

void tool_preferences(Glib::ustring const &tool, InkscapeWindow *win);

// Standard function to add actions.
void add_actions_tools(InkscapeWindow* win);

#endif // INK_ACTIONS_TOOLS_H

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
