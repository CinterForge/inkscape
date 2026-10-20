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

#ifndef INK_ACTIONS_EFFECT_H
#define INK_ACTIONS_EFFECT_H

class InkscapeApplication;
class SPDocument;

void add_actions_effect(InkscapeApplication* app);
void add_document_actions_effect(SPDocument *doc);

void enable_effect_actions(InkscapeApplication* app, bool enabled);

#endif // INK_ACTIONS_EFFECT_H
