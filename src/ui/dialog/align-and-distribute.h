// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Align and Distribute widget
 */
/* Authors:
 *   Tavmjong Bah
 *
 *   Based on dialog by:
 *     Bryce W. Harrington <bryce@bryceharrington.org>
 *     Aubanel MONNIER <aubi@libertysurf.fr>
 *     Frank Felfe <innerspace@iname.com>
 *     Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_ALIGN_AND_DISTRIBUTE_H
#define INKSCAPE_UI_WIDGET_ALIGN_AND_DISTRIBUTE_H

#include <set>
#include <gtkmm/box.h>

#include "preferences.h"

namespace Gtk {
class Builder;
class Button;
class ComboBox;
class Frame;
class SpinButton;
class ToggleButton;
} // namespace Gtk

class SPDesktop;

namespace Inkscape {
namespace UI {
namespace Widget {
class SpinButton;
}

namespace Tools {
class ToolBase;
}

namespace Dialog {
class DialogBase;

class AlignAndDistribute : public Gtk::Box
{
public:
    AlignAndDistribute(Inkscape::UI::Dialog::DialogBase* dlg);
    ~AlignAndDistribute() override;

    void desktop_changed(SPDesktop* desktop);
    void tool_changed(SPDesktop* desktop); // Need to show different widgets for node vs. other tools.

private:

    // ********* Widgets ********** //
    Glib::RefPtr<Gtk::Builder> builder;

    Gtk::Box &align_and_distribute_box;
    Gtk::Box &align_and_distribute_object;  // Hidden when node tool active.
    Gtk::Frame &remove_overlap_frame;       // Hidden when node tool active.
    Gtk::Box &align_and_distribute_node;    // Visible when node tool active.

    // Align
    Gtk::ToggleButton &align_move_as_group;
    Gtk::ComboBox     &align_relative_object;
    Gtk::ComboBox     &align_relative_node;

    // Remove overlap
    Gtk::Button       &remove_overlap_button;
    UI::Widget::SpinButton   &remove_overlap_hgap;
    UI::Widget::SpinButton   &remove_overlap_vgap;

    // Valid relative alignment entries for single selection.
    std::set<Glib::ustring> single_selection_relative_categories = {"drawing", "page"};
    Glib::ustring single_selection_align_to = "page";
    Glib::ustring multi_selection_align_to;
    bool single_item = false;

    // ********* Signal handlers ********** //

    void on_align_as_group_clicked();
    void on_align_relative_object_changed();
    void on_align_relative_node_changed();

    void on_align_clicked         (std::string const &align_to);
    void on_remove_overlap_clicked();
    void on_align_node_clicked    (std::string const &align_to);

    sigc::connection tool_connection;
    sigc::scoped_connection sel_changed;
    Inkscape::PrefObserver _icon_sizes_changed;
};

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_ALIGN_AND_DISTRIBUTE_H

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
