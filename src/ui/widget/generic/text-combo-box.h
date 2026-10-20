// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//
//
// This is a combobox with text entry.
// It accepts gio::model to set up a list of items to choose from.
// Users can type and input will be used to match the entry in a list.
//
// This widget can handle large amounts of data, as it only instantiates
// a handful of UI elements at a time and reuses them.

#ifndef INKSCAPE_TEXT_COMBO_BOX_H
#define INKSCAPE_TEXT_COMBO_BOX_H

#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/listview.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/popover.h>

#include "css-name-class-init.h"
#include "ui/widget/font-list.h"
#include "ui/widget/gtk-registry.h"

namespace Gtk {
class FilterListModel;
class SignalListItemFactory;
class StringList;
}

namespace Inkscape::UI::Widget {

class TextComboBox : public CssNameClassInit, public BuildableWidget<TextComboBox, Gtk::Widget> {
public:
    typedef GtkWidget BaseObjectType;

    TextComboBox();
    explicit TextComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});
    ~TextComboBox() override = default;

    // set a model for combobox to use and populate its popup list;
    // a callback must retrieve item's text from the model;
    // a second callback (optional) is used to retrieve item's markup
    void set_model(
        const Glib::RefPtr<Gio::ListModel>& model,
        const std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)>& get_label_text,
        const std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)>& get_label_markup = {});
    // convenience function for setting StringList if simple model of strings is sufficient
    void set_model(Glib::RefPtr<Gtk::StringList> model);
    //  unset model for the combobox
    void unset_model();
    // get the currently selected item's index or -1 if none is selected
    int get_selected() const;
    // set the selected item by its position in the model
    void set_selected(int index);
    // get the currently selected item's text or empty string
    Glib::ustring get_selected_text() const;
    // request a fixed with for the popup menu or pass 0 to make it match combobox's width
    void set_popup_width(int width);
    // set a callback to invoke to defocus this widget
    void set_defocus_callback(const std::function<void ()>& defocus);

    // signal emitted when the user selects an entry from a list or presses the Enter key
    sigc::signal<void (Glib::ustring)> signal_value_changed() { return _signal_value_changed; }

private:
    void construct();
    void measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural, int& minimum_baseline, int& natural_baseline) const override;
    void size_allocate_vfunc(int width, int height, int baseline) override;
    void refilter();
    void reset_filter();
    bool on_key_pressed(guint keyval, Gdk::ModifierType state);
    void select_next(int delta, bool add_suffix = true);
    void select_item(int item, bool add_suffix = true, bool notify = true);
    void append_text(const Glib::ustring& text);
    Glib::ustring get_text_item(int index) const;

    Gtk::Box _box;
    Gtk::Entry _entry;
    Gtk::MenuButton _menu_btn;
    Gtk::Popover _popup;
    Gtk::ListView _list_view;
    Glib::ustring _action_name;
    sigc::signal<void (Glib::ustring)> _signal_value_changed;
    Glib::RefPtr<Gtk::SignalListItemFactory> _factory;
    std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)> _get_item_label;
    std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)> _get_item_markup;
    Glib::RefPtr<Gio::ListModel> _model;
    Glib::RefPtr<Gtk::FilterListModel> _filtered_model;
    Glib::RefPtr<Gtk::SingleSelection> _selection_model;
    Glib::RefPtr<Gtk::BoolFilter> _filter = Gtk::BoolFilter::create({});
    Glib::RefPtr<Gtk::EventControllerKey> _key_entry = Gtk::EventControllerKey::create();
    Glib::ustring _search_text;
    OperationBlocker _update;
    bool _use_markup = false;
    int _popup_width = 0;
    std::function<void ()> _defocus;
};

} // namespace

#endif //INKSCAPE_TEXT_COMBO_BOX_H
