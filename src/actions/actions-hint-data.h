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

#ifndef INK_ACTIONS_HINT_DATA_H
#define INK_ACTIONS_HINT_DATA_H

#include <glibmm/ustring.h>
#include <glibmm/varianttype.h>
#include <map>
#include <utility>
#include <vector>

class InkActionHintData
{
public:
    InkActionHintData() = default ;

    std::vector<Glib::ustring> get_actions();

    void add_data(std::vector<std::vector<Glib::ustring>> &raw_data);

    Glib::ustring get_tooltip_hint_for_action(Glib::ustring const &action_name, bool translated = true);

private:
    std::map<Glib::ustring, Glib::ustring> data;
};

#endif // INK_ACTIONS_HINT_DATA_H