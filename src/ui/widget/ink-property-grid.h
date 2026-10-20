// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * InkPropertyGrid: grid that can hold list of properties in form of label and editing widgets
 * with support for a single and two-column layout.
 */
/*
 * Author:
 *   Michael Kowalski
 *
 * Copyright (C) 2024 Michael Kowalski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_PROPERTY_GRID_H
#define INK_PROPERTY_GRID_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/sizegroup.h>

#include "generic/bin.h"
#include "widget-group.h"

namespace Inkscape::UI::Widget {

class InkPropertyGrid : public Bin {
public:
    InkPropertyGrid();
    ~InkPropertyGrid() override = default;

    // add a property row to the grid; label and values are optional; widget1 is expected, whereas widget2
    // can be specified if this is (potentially) 2-column property (like dimensions width and height)
    WidgetGroup add_property(Gtk::Label* label, Gtk::Widget* button1, Gtk::Widget* widget1, Gtk::Widget* widget2, Gtk::Widget* button2 = nullptr, int margin = 2);
    WidgetGroup add_property(const std::string& label, Gtk::Widget* button1, Gtk::Widget* widget1, Gtk::Widget* widget2, Gtk::Widget* button2 = nullptr, int margin = 2);
    // leave a gap before adding a new row; used to indicate a new group of properties
    Gtk::Widget* add_gap(int size = 8);
    // add a widget to the grid that will occupy both columns
    WidgetGroup add_row(Gtk::Widget* widget, Gtk::Widget* button = nullptr, bool whole_row = true, int margin = 2);
    WidgetGroup add_row(const std::string& label, Gtk::Widget* widget, Gtk::Widget* button = nullptr, int margin = 2);
    WidgetGroup add_row(Gtk::Label* label, Gtk::Widget* widget, Gtk::Widget* button = nullptr, int margin = 2);
    WidgetGroup add_full_row(Gtk::Widget* widget, int margin = 2);
    // add a section divider that occupies both columns and rightmost button area
    Gtk::Button* add_section(const std::string& label, int margin = 2);
    Gtk::Button* add_section(Gtk::Label* label, int margin = 2);
    static void open_section(Gtk::Button* button, bool open);
    Gtk::Widget* add_section_divider();

    // set element indentation
    void set_indent(int indent);

    // if a size group is provided, it will be used for all labels in the first column
    void set_first_column_group(Glib::RefPtr<Gtk::SizeGroup> column_size) { _first_column = column_size; }

    Glib::RefPtr<Gtk::SizeGroup> get_height_group() { return _field_height; }

private:
    void construct();
    void set_single_column(bool single);
    void update_min_size();

    Gtk::Grid _grid;
    int _row = 0;
    int _min_width = 0;
    bool _single_column = false;
    Glib::RefPtr<Gtk::SizeGroup> _field_width = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
    Glib::RefPtr<Gtk::SizeGroup> _field_height = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::VERTICAL);
    Glib::RefPtr<Gtk::SizeGroup> _first_column;
    Gtk::Box _left_margin;
};

// Move widgets that have been added to a source Grid to a new parent: InkPropertyGrid
WidgetGroup reparent_properties(Gtk::Grid& source, InkPropertyGrid& grid, bool include_button_column = true, bool force_wide = false, int group_from_row = 0);

}

#endif //INK_PROPERTY_GRID_H
