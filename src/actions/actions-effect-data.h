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

#ifndef INK_ACTIONS_EFFECT_DATA_H
#define INK_ACTIONS_EFFECT_DATA_H

#include <list>
#include <string>
#include <vector>
#include <utility>

#include <glibmm/ustring.h>

class InkActionEffectData
{
public:
    InkActionEffectData()  = default;

    struct datum {
        std::string effect_id;
        std::list<Glib::ustring> submenu;
        Glib::ustring effect_name;
        bool is_filter;

        bool operator < (const datum& d) const;
    };

    // Get Vector
    const std::vector<datum>& give_all_data() const;

    // Add Data
    void add_data(std::string effect_id, bool is_filter, std::list<Glib::ustring> effect_submenu_vector, Glib::ustring const &effect_name);

private:
    std::vector<datum> data;
};

#endif // INK_ACTIONS_EFFECT_DATA_H

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
