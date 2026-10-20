// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_GRADIENT_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_GRADIENT_TOOLBAR_H

/**
 * @file Gradient toolbar
 */
/*
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"
#include "ui/operation-blocker.h"
#include "ui/widget/generic/icon-combobox.h"

namespace Gtk {
class SingleSelection;
class DropDown;
class Builder;
class Button;
class ToggleButton;
} // namespace Gtk

class SPGradient;
class SPStop;
class SPObject;

namespace Inkscape {
class Selection;
namespace UI {
namespace Tools { class ToolBase; }
namespace Widget {
class DropDownList;
class SpinButton;
} // namespace Widget
} // namespace UI
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class GradientToolbar : public Toolbar
{
public:
    GradientToolbar();
    ~GradientToolbar() override;

    void setDesktop(SPDesktop *desktop) override;

private:
    GradientToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    using Store = Gio::ListStore<UI::Widget::IconComboBox::ListItem>;
    Glib::RefPtr<Store> _gradient_store = Store::create();
    Glib::RefPtr<Store> _stop_store = Store::create();
    std::vector<Gtk::ToggleButton *> _new_type_buttons;
    std::vector<Gtk::ToggleButton *> _new_fillstroke_buttons;
    UI::Widget::IconComboBox& _select_cb;
    Gtk::ToggleButton &_linked_btn;
    Gtk::Button &_stops_reverse_btn;
    UI::Widget::IconComboBox& _spread_cb;
    UI::Widget::IconComboBox& _stop_cb;
    UI::Widget::SpinButton &_offset_item;
    Gtk::Button &_stops_add_btn;
    Gtk::Button &_stops_delete_btn;
    bool _offset_adj_changed = false;
    OperationBlocker _blocker;

    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value);
    void new_type_changed(int mode);
    void new_fillstroke_changed(int mode);
    void gradient_changed(int active);
    SPGradient *get_selected_gradient();
    void spread_changed(int active);
    void stop_changed(int active);
    void select_dragger_by_stop(SPStop* stop, Tools::ToolBase *ev);
    SPStop *get_selected_stop();
    void stop_set_offset(SPStop* stop);
    void stop_offset_adjustment_changed();
    void add_stop();
    void remove_stop();
    void reverse();
    void linked_changed();
    void _update();
    int update_stop_list(SPGradient *gradient, SPStop *new_stop, bool gr_multi);
    int select_stop_in_list(SPGradient *gradient, SPStop *new_stop);
    void select_stop_by_draggers(SPGradient *gradient, UI::Tools::ToolBase *ev);

    sigc::connection _connection_changed;
    sigc::connection _connection_modified;
    sigc::connection _connection_subselection_changed;
    sigc::connection _connection_defs_release;
    sigc::connection _connection_defs_modified;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_GRADIENT_TOOLBAR_H
