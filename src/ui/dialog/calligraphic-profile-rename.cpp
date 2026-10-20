// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Dialog for naming calligraphic profiles.
 *
 * @note This file is in the wrong directory because of link order issues -
 * it is required by widgets/toolbox.cpp, and libspwidgets.a comes after
 * libinkdialogs.a in the current link order.
 */
/* Author:
 *   Aubanel MONNIER
 *
 * Copyright (C) 2007 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "calligraphic-profile-rename.h"

#include <glibmm/i18n.h>

#include "desktop.h"
#include "ui/dialog-run.h"

namespace Inkscape::UI::Dialog {

CalligraphicProfileRename::CalligraphicProfileRename()
{
    set_name("CalligraphicProfileRename");
    set_title(_("Edit profile"));

    _layout_table.set_column_spacing(4);
    _layout_table.set_row_spacing(4);

    _profile_name_entry.set_activates_default(true);

    _profile_name_label.set_label(_("Profile name:"));
    _profile_name_label.set_halign(Gtk::Align::END);
    _profile_name_label.set_valign(Gtk::Align::CENTER);

    _layout_table.attach(_profile_name_label, 0, 0, 1, 1);

    _profile_name_entry.set_hexpand();
    _layout_table.attach(_profile_name_entry, 1, 0, 1, 1);

    auto mainbox = get_content_area();
    mainbox->set_margin(4);
    mainbox->append(_layout_table);

    // Buttons
    _close_button.set_use_underline();
    _close_button.set_label(_("_Cancel"));
    _close_button.set_receives_default();

    _delete_button.set_use_underline(true);
    _delete_button.set_label(_("_Delete"));
    _delete_button.set_receives_default();
    _delete_button.set_visible(false);

    _apply_button.set_use_underline(true);
    _apply_button.set_label(_("_Save"));
    _apply_button.set_receives_default();

    _close_button.signal_clicked()
            .connect(sigc::mem_fun(*this, &CalligraphicProfileRename::_close));
    _delete_button.signal_clicked()
            .connect(sigc::mem_fun(*this, &CalligraphicProfileRename::_delete));
    _apply_button.signal_clicked()
            .connect(sigc::mem_fun(*this, &CalligraphicProfileRename::_apply));

    add_action_widget(_close_button, Gtk::ResponseType::CLOSE);
    add_action_widget(_delete_button, Gtk::ResponseType::DELETE_EVENT);
    add_action_widget(_apply_button, Gtk::ResponseType::APPLY);

    set_default_widget(_apply_button);
}

void CalligraphicProfileRename::_apply()
{
    _profile_name = _profile_name_entry.get_text();
    _applied = true;
    _deleted = false;
    _close();
}

void CalligraphicProfileRename::_delete()
{
    _profile_name = _profile_name_entry.get_text();
    _applied = true;
    _deleted = true;
    _close();
}

void CalligraphicProfileRename::_close()
{
    set_visible(false);
}

void CalligraphicProfileRename::show(SPDesktop *desktop, Glib::ustring const &profile_name)
{
    auto &dial = _getInstance();
    dial._applied = false;
    dial._deleted = false;
    dial._profile_name = profile_name;
    dial._profile_name_entry.set_text(profile_name);

    if (profile_name.empty()) {
        dial.set_title(_("Add profile"));
        dial._delete_button.set_visible(false);
    } else {
        dial.set_title(_("Edit profile"));
        dial._delete_button.set_visible(true);
    }

    desktop->setWindowTransient(dial);
    Inkscape::UI::dialog_run(dial);
}

CalligraphicProfileRename &CalligraphicProfileRename::_getInstance()
{
    static CalligraphicProfileRename instance;
    return instance;
}

} // namespace Inkscape::UI::Dialog

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
