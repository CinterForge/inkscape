// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for operating on nodes without GUI.
 *
 * Requires nodes to be selected which currently can only be done with node tool.
 *
 * Copyright (C) 2026 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_ACTIONS_NODE_TOOL_H
#define INK_ACTIONS_NODE_TOOL_H

class InkscapeWindow;

void add_actions_node_tool(InkscapeWindow* win);

#endif // INK_ACTIONS_NODE_TOOL_H

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
