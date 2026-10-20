// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 *
 * Effect Data to store data related to creating of
 * Filters and Effect manubar
 *
 * Authors:
 *   Sushant A A <sushant.co19@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "actions-effect-data.h"

#include <iostream>
#include <algorithm>
#include <string>
#include <tuple>

#include <glibmm/i18n.h>

const std::vector<InkActionEffectData::datum>& InkActionEffectData::give_all_data() const {
    return data;
}

bool InkActionEffectData::datum::operator < (const datum& b) const {
    const auto& a = *this;

    if (a.is_filter != b.is_filter) return a.is_filter < b.is_filter;

    // Sort by menu tree and effect name.
    const auto& a_list = a.submenu;
    const auto& b_list = b.submenu;

    auto a_it = a_list.begin();
    auto b_it = b_list.begin();
    while (a_it != a_list.end() && b_it != b_list.end()) {
        // Note: .raw() is only for performance reasons because Glib::ustring::operator < is slow
        if (a_it->raw() < b_it->raw()) return true;
        if (a_it->raw() > b_it->raw()) return false;
        a_it++;
        b_it++;
    }
    if (a_it != a_list.end()) return a_it->raw() < b.effect_name.raw();  // Compare menu name with effect name.
    if (b_it != b_list.end()) return b_it->raw() > a.effect_name.raw();  // Compare menu name with effect name.
    return a.effect_name.raw() < b.effect_name.raw(); // Same menu, order by effect name.
}

void InkActionEffectData::add_data(
    std::string effect_id,
    bool is_filter,
    std::list<Glib::ustring> effect_submenu,
    Glib::ustring const &effect_name)
{
    auto el = datum{effect_id, effect_submenu, effect_name, is_filter};
    data.insert(std::upper_bound(data.begin(), data.end(), el), el);
}

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
