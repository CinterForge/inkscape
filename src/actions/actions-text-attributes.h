// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_ACTIONS_TEXT_ATTRIBUTES_H
#define INK_ACTIONS_TEXT_ATTRIBUTES_H

class InkscapeWindow;
class SPStyle;

void text_decoration_line_set(InkscapeWindow* win, SPStyle* query);
void add_actions_text_attributes(InkscapeWindow* win);

#endif // INK_ACTIONS_TEXT_ATTRIBUTES_H
