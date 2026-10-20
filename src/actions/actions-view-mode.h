// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_ACTIONS_VIEW_MODE_H
#define INK_ACTIONS_VIEW_MODE_H

class InkscapeWindow;

bool get_widescreen_default();
void add_actions_view_mode(InkscapeWindow* win);
void view_set_gui(InkscapeWindow* win);

#endif // INK_ACTIONS_VIEW_MODE_H
