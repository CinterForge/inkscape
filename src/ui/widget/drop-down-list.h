// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Simple DropDown widget presenting popup list of string items to choose from
 */
/*
 * Authors:
 *   Mike Kowalski
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/builder.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/stringlist.h>

#ifndef INCLUDE_DROP_DOWN_LIST_H
#define INCLUDE_DROP_DOWN_LIST_H

namespace Gtk {
class Label;
}

namespace Inkscape::UI::Widget {

class DropDownList : public Gtk::DropDown {
public:
    DropDownList();

    // GtkBuilder constructor
    DropDownList(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

    // append a new item to the dropdown list, return its position
    unsigned int append(const Glib::ustring& item);
    // get n-th item
    Glib::ustring get_string(unsigned int position) const { return _model->get_string(position); }
    // get number of items
    unsigned int get_item_count() const { return _model->get_n_items(); }
    // delete all items
    void remove_all() { _model->splice(0, _model->get_n_items(), {}); }
    // set to limit the width of the dropdown itself (-1 to impose no limit)
    void set_button_max_chars(int max_chars);

    // selected item changed signal
    Glib::SignalProxyProperty signal_changed() { return property_selected().signal_changed(); }

    // enable searching in a popup list
    void enable_search(bool enable = true);

    // if set, this callback will be invoked for each item position - returning true will
    // insert a separator on top of that item
    void set_row_separator_func(std::function<bool (unsigned int)> callback);

    // if set, this function will be used to extract string from items stored in the model
    void set_to_string_func(std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)> callback);

    // enable ellipsizing strings in the dropdown button itself
    void set_ellipsize_button(bool ellipsize = true) { _ellipsize_button = ellipsize; }

private:
    void _init();
    Glib::ustring get_item_string(const Glib::RefPtr<Glib::ObjectBase>& item);
    Gtk::Label* set_up_item(bool ellipsize);
    Glib::RefPtr<Gtk::StringList> _model = Gtk::StringList::create({});
    Glib::RefPtr<Gtk::SignalListItemFactory> _factory = Gtk::SignalListItemFactory::create();
    std::function<bool (unsigned int)> _separator_callback;
    std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)> _to_string;
    int _button_max_chars = -1;
    bool _ellipsize_button = false;
};

} // namespace

#endif // INCLUDE_DROP_DOWN_LIST_H
