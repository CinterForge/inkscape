// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 1/7/26.
//
//
// Simple number editing widget combined with a list of predefined values
// for users to choose from. A combobox for numbers only.

#ifndef INKSCAPE_NUMBER_COMBO_BOX_H
#define INKSCAPE_NUMBER_COMBO_BOX_H

#include <giomm/menu.h>
#include <gtkmm/box.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/popovermenu.h>

#include "css-name-class-init.h"
#include "spin-button.h"
#include "ui/widget/gtk-registry.h"

namespace Inkscape::UI::Widget {

class NumberComboBox : public CssNameClassInit, public BuildableWidget<NumberComboBox, Gtk::Widget> {
public:
    typedef GtkWidget BaseObjectType;

    NumberComboBox();
    explicit NumberComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});
    ~NumberComboBox() override = default;

    // add a 'value' to combobox menu
    void append(double value);
    // set selected row (TODO)
    void set_selected_item(int index);
    // set numeric value in the combobox
    void set_value(double value);
    // populate combobox menu
    void set_menu_options(const std::vector<double>& list);
    // set position of the menu
    void set_popup_position(Gtk::PositionType position);
    // get access to number editing widget
    InkSpinButton& get_entry() { return _number; }

    // signal fired when the value changes
    sigc::signal<void (double)>& signal_value_changed() { return _signal_value_changed; }
private:
    void construct();
    bool on_key_pressed(guint keyval, Gdk::ModifierType state);
    void select_next(int delta);
    void select_item(int index);
    int find_index(double value) const;
    void measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural, int& minimum_baseline, int& natural_baseline) const override;
    void size_allocate_vfunc(int width, int height, int baseline) override;

    Gtk::Box _box;
    InkSpinButton _number;
    Gtk::MenuButton _menu_btn;
    Gtk::PopoverMenu _popup;
    Glib::RefPtr<Gio::Menu> _menu = Gio::Menu::create();
    Glib::ustring _action_name;
    std::vector<double> _list;
    sigc::signal<void (double)> _signal_value_changed;
    Glib::RefPtr<Gtk::EventControllerKey> _key_entry;
};

} // namespace

#endif //INKSCAPE_NUMBER_COMBO_BOX_H
