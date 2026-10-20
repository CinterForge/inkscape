// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Select toolbar
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <bulia@dr.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2003 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLBAR_SELECT_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_SELECT_TOOLBAR_H

#include "preferences.h"
#include "toolbar.h"
#include "ui/operation-blocker.h"

namespace Gtk {
class Adjustment;
class ToggleButton;
class Builder;
} // namespace Gtk

namespace Inkscape {
class Selection;
namespace UI::Widget {
class SpinButton;
class UnitTracker;
} // namespace UI::Widget
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class SelectToolbar : public Toolbar
{
public:
    SelectToolbar();
    ~SelectToolbar() override;

    void setDesktop(SPDesktop *desktop) override;
    void setActiveUnit(Util::Unit const *unit) override;

protected:
    void documentReplaced() override;

private:
    SelectToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    std::unique_ptr<UI::Widget::UnitTracker> _tracker;

    Gtk::ToggleButton &_select_touch_btn;

    Gtk::ToggleButton &_transform_stroke_btn;
    Gtk::ToggleButton &_transform_corners_btn;
    Gtk::ToggleButton &_transform_gradient_btn;
    Gtk::ToggleButton &_transform_pattern_btn;

    UI::Widget::SpinButton &_x_item;
    UI::Widget::SpinButton &_y_item;
    UI::Widget::SpinButton &_w_item;
    UI::Widget::SpinButton &_h_item;
    Gtk::ToggleButton &_lock_btn;

    std::vector<Gtk::Widget *> _context_items;

    PrefObserver _box_observer;
    OperationBlocker _blocker;

    std::string _action_key;
    std::string const _action_prefix;

    char const *get_action_key(double mh, double sh, double mv, double sv);
    void any_value_changed(Glib::RefPtr<Gtk::Adjustment> const &adj);
    void layout_widget_update();
    void toggle_lock();
    void toggle_touch();
    void toggle_stroke();
    void toggle_corners();
    void toggle_gradient();
    void toggle_pattern();
    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name);
    void _sensitize();

    void _selectionChanged(Selection *selection);
    void _selectionModified(Selection *selection, unsigned flags);
    sigc::connection _selection_changed_conn;
    sigc::connection _selection_modified_conn;
    sigc::scoped_connection _page_origin_conn;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_SELECT_TOOLBAR_H

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
