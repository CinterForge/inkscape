// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 * @brief Basic dialog info.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2021 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <map>
#include <span>
#include <glibmm/i18n.h>
#include <glibmm/ustring.h>

enum class ScrollProvider {
    PROVIDE = 0,
    NOPROVIDE
};

class DialogData {
public:
    std::string key;        // unique key; used internally
    Glib::ustring label;    // user-facing dialog name
    Glib::ustring icon_name;
    enum Category { Basic = 0, Typography, EffectsActions, Assets, Advanced, Settings, Diagnostics, _num_categories };
    Category category;
    ScrollProvider provide_scroll;
};

// dialog categories (used to group them in a dialog submenu)
char const *const dialog_categories[DialogData::_num_categories] = {
    // NOTE: keep names in sync with Categories enum definition above
    N_("Basic"),
    N_("Text & Typography"),
    N_("Effects & Actions"),
    N_("Assets"),
    N_("Advanced"),
    N_("Settings"),
    N_("Diagnostic"),
};

/** Get the data about all existing dialogs. */
std::map<std::string, DialogData> const &get_dialog_data();

// Return dialog data as a list, so they can be presented in the menu ordered by importance
std::span<const DialogData> get_dialog_data_list();

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
