// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Dialog for naming calligraphic profiles
 */
/* Author:
 *   Aubanel MONNIER
 *
 * Copyright (C) 2007 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_CALLIGRAPHIC_PROFILE_H
#define INKSCAPE_UI_DIALOG_CALLIGRAPHIC_PROFILE_H

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/grid.h>

class SPDesktop;

namespace Inkscape::UI::Dialog {

class CalligraphicProfileRename : public Gtk::Dialog
{
public:
    static void show(SPDesktop *desktop, Glib::ustring const &profile_name);

    static bool applied() {
        return _getInstance()._applied;
    }
    static bool deleted() {
        return _getInstance()._deleted;
    }
    static Glib::ustring const &getProfileName() {
        return _getInstance()._profile_name;
    }

private:
    CalligraphicProfileRename();

    void _close();
    void _apply();
    void _delete();

    Gtk::Label _profile_name_label;
    Gtk::Entry _profile_name_entry;
    Gtk::Grid _layout_table;

    Gtk::Button _close_button;
    Gtk::Button _delete_button;
    Gtk::Button _apply_button;

    Glib::ustring _profile_name;
    bool _applied;
    bool _deleted;

    static CalligraphicProfileRename &_getInstance();
};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_CALLIGRAPHIC_PROFILE_H

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
