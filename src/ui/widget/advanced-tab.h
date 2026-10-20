// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Advanced Tab for F&S
 */
/* Authors:
 *   Ayan Das <ayandazzz@outlook.com>
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#pragma once

#include <gtkmm.h>

#include "recolor-art.h"
#include "selection.h"

namespace Inkscape::UI::Widget {

class AdvancedTab : public Gtk::Box
{
public:
    AdvancedTab();
    ~AdvancedTab() override = default;

    void setDesktop(SPDesktop *desktop);
    void updateFromSelection(Selection *selection);

private:
    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::DropDown *_cmb_format = nullptr;
    Gtk::DropDown *_cmb_interp = nullptr;
    Gtk::Expander *_expander = nullptr;
    RecolorArt *_recolor_widget = nullptr;
    SPDesktop *_desktop = nullptr;

    void on_interp_changed();
    sigc::connection _cms_conn;
    void on_cms_changed();

    sigc::connection _interp_conn;
    void rebuild_interp_model(bool is_inherited, Glib::ustring target_name);
    sigc::connection _format_conn;
    void rebuild_format_model(bool is_inherited, Glib::ustring target_name);
};

} // namespace Inkscape::UI::Widget

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
