// SPDX-License-Identifier: GPL-2.0-or-later

#include "ink-property-grid.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/separator.h>

namespace Inkscape::UI::Widget {

InkPropertyGrid::InkPropertyGrid() {
    construct();
}

// grid columns:
constexpr int COL_MARGIN   = 0;
constexpr int COL_LABEL    = 1; // property name
constexpr int COL_BUTTON_1 = 2; // button in front of a property (like a padlock, scale lock, etc.)
constexpr int COL_FILED_1  = 3; // property widget
constexpr int COL_BUTTON_2 = 4; // button at the end of a property (like a reset/clear)
constexpr int COL_COUNT    = 5; // number of columns

void InkPropertyGrid::construct() {
    set_name("InkPropertyGrid");
    set_child(_grid);
    _grid.attach(_left_margin, COL_MARGIN, 0);

    connectBeforeResize([this](int width, int height, int baseline) {
        auto m = measure(Gtk::Orientation::HORIZONTAL);
        if (!_single_column && m.sizes.minimum < _min_width - 1) {
            _min_width = m.sizes.minimum + 1;
        }
        auto single = width <= _min_width;
        if (single != _single_column) {
            // introduce hysteresis to avoid flickering
            if (!single && abs(width - _min_width) < 2) return;
        }
        set_single_column(single);
    });
}

WidgetGroup InkPropertyGrid::add_property(Gtk::Label* label, Gtk::Widget* button1, Gtk::Widget* w1, Gtk::Widget* w2, Gtk::Widget* btn, int margin) {
    WidgetGroup group;

    if (label) {
        group.add(label);
        label->set_margin(margin);
        _field_height->add_widget(*label);
        label->set_halign(Gtk::Align::START);
        label->set_valign(Gtk::Align::START);
        label->set_xalign(0);
        label->set_ellipsize(Pango::EllipsizeMode::END);
        _grid.attach(*label, COL_LABEL, _row, button1 ? 1 : 2);
        if (_first_column) {
            _first_column->add_widget(*label);
        }
    }
    if (button1) {
        group.add(button1);
        button1->set_margin(margin);
        button1->set_margin_end(0);
        button1->set_valign(Gtk::Align::CENTER);
        _grid.attach(*button1, COL_BUTTON_1, _row);
    }
    if (w1) {
        group.add(w1);
        w1->set_margin(margin);
        w1->set_hexpand();
        _field_width->add_widget(*w1);
        _field_height->add_widget(*w1);
    }
    if (w2) {
        if (w2->get_halign() == Gtk::Align::START) {
            auto box = Gtk::make_managed<Gtk::Box>();
            box->append(*w2);
            w2 = box;
        }
        group.add(w2);
        w2->set_margin(margin);
        w2->set_hexpand();
        _field_width->add_widget(*w2);
        _field_height->add_widget(*w2);
    }
    else {
        w2 = Gtk::make_managed<Gtk::Box>();
        group.add(w2);
        w2->set_hexpand();
        w2->set_margin_start(margin);
        w2->set_margin_end(margin);
        _field_width->add_widget(*w2);
    }

    if (w1 && w2) {
        auto box = Gtk::make_managed<Gtk::Box>();
        box->add_css_class("fields");
        box->append(*w1);
        box->append(*w2);
        _grid.attach(*box, COL_FILED_1, _row);
    }
    else if (w1) {
        _grid.attach(*w1, COL_FILED_1, _row);
    }
    if (btn) {
        group.add(btn);
        btn->set_margin_start(0);
        btn->set_margin_end(0);
        _grid.attach(*btn, COL_BUTTON_2, _row);
    }

    ++_row;

    update_min_size();
    return group;
}

WidgetGroup InkPropertyGrid::add_property(const std::string& label, Gtk::Widget* button1, Gtk::Widget* widget1, Gtk::Widget* widget2, Gtk::Widget* button2, int margin) {
    auto l = Gtk::make_managed<Gtk::Label>(label);
    l->set_halign(Gtk::Align::START);
    l->set_xalign(0);
    l->set_ellipsize(Pango::EllipsizeMode::END);
    return add_property(l, button1, widget1, widget2, button2, margin);
}

Gtk::Widget* InkPropertyGrid::add_gap(int size) {
    auto gap = Gtk::make_managed<Gtk::Box>();
    gap->set_size_request(1, size);
   _grid.attach(*gap, COL_LABEL, _row++);
    return gap;
}

WidgetGroup InkPropertyGrid::add_row(Gtk::Widget* widget, Gtk::Widget* button, bool whole_row, int margin) {
    WidgetGroup group;
    if (!widget) return group;

    widget->set_margin(margin);
    _grid.attach(*widget, whole_row ? COL_LABEL : COL_FILED_1, _row, whole_row ? 3 : 2);
    group.add(widget);

    if (button) {
        button->set_margin_start(0);
        button->set_margin_end(0);
        _grid.attach(*button, COL_BUTTON_2, _row);
        group.add(button);
    }

    update_min_size();
    ++_row;
    return group;
}

WidgetGroup InkPropertyGrid::add_row(const std::string& label, Gtk::Widget* widget, Gtk::Widget* button, int margin) {
    Gtk::Label* l = nullptr;
    if (!label.empty()) {
        l = Gtk::make_managed<Gtk::Label>(label);
        l->set_halign(Gtk::Align::START);
        l->set_xalign(0);
        l->set_ellipsize(Pango::EllipsizeMode::END);
        l->set_margin(margin);
    }
    return add_row(l, widget, button, margin);
}

WidgetGroup InkPropertyGrid::add_row(Gtk::Label* label, Gtk::Widget* widget, Gtk::Widget* button, int margin) {
    WidgetGroup group;
    if (label) {
        group.add(label);
        label->set_margin(margin);
        label->set_halign(Gtk::Align::START);
        label->set_xalign(0);
        label->set_ellipsize(Pango::EllipsizeMode::END);
        _grid.attach(*label, COL_LABEL, _row, widget ? 2 : 3);
    }

    if (widget) {
        widget->set_margin(margin);
        _grid.attach(*widget, COL_FILED_1, _row);
        group.add(widget);
    }

    if (button) {
        button->set_margin_start(0);
        button->set_margin_end(0);
        _grid.attach(*button, COL_BUTTON_2, _row);
        group.add(button);
    }

    ++_row;
    update_min_size();
    return group;
}

WidgetGroup InkPropertyGrid::add_full_row(Gtk::Widget* widget, int margin) {
    WidgetGroup group;
    if (!widget) return group;

    widget->set_margin(margin);
    _grid.attach(*widget, COL_LABEL, _row, COL_COUNT - COL_LABEL);
    group.add(widget);

    update_min_size();
    ++_row;
    return group;
}

Gtk::Button* InkPropertyGrid::add_section(const std::string& label, int margin) {
    auto label_widget = Gtk::make_managed<Gtk::Label>(label);
    label_widget->add_css_class("grid-section-title");
    return add_section(label_widget, margin);
}

Gtk::Button* InkPropertyGrid::add_section(Gtk::Label* label, int margin) {
    auto button = Gtk::make_managed<Gtk::Button>();
    button->add_css_class("grid-section-button");
    button->set_has_frame(false);
    button->set_can_focus(false);

    auto box = Gtk::make_managed<Gtk::Box>();
    if (label) {
        box->append(*label);
        label->set_halign(Gtk::Align::START);
        label->set_xalign(0);
    }

    auto icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name("section-expand");
    icon->set_hexpand();
    icon->set_halign(Gtk::Align::END);
    icon->set_margin_end(margin);
    box->append(*icon);
    button->set_child(*box);

    _grid.attach(*button, COL_LABEL, _row, COL_COUNT - COL_LABEL);

    ++_row;
    update_min_size();
    return button;
}

void InkPropertyGrid::open_section(Gtk::Button* button, bool open) {
    if (!button) return;

    auto& image = dynamic_cast<Gtk::Image&>(*button->get_child()->get_last_child());
    image.set_from_icon_name(open ? "section-collapse" : "section-expand");
}

Gtk::Widget* InkPropertyGrid::add_section_divider() {
    auto separator = Gtk::make_managed<Gtk::Separator>();
    separator->add_css_class("grid-section-divider");
    _grid.attach(*separator, COL_MARGIN, _row++, COL_COUNT);
    return separator;
}

void InkPropertyGrid::set_indent(int indent) {
    _left_margin.set_size_request(indent, -1);
}

void InkPropertyGrid::set_single_column(bool single) {
    if (_single_column == single) return;

    _single_column = single;

    for (int row = 0; row < _row; ++row) {
        if (auto box = dynamic_cast<Gtk::Box*>(_grid.get_child_at(COL_FILED_1, row))) {
            if (box->has_css_class("fields")) {
                box->set_orientation(single ? Gtk::Orientation::VERTICAL : Gtk::Orientation::HORIZONTAL);
            }
        }
    }
}

void InkPropertyGrid::update_min_size() {
    auto m = measure(Gtk::Orientation::HORIZONTAL);
    _min_width = m.sizes.minimum + 1;
}

WidgetGroup reparent_properties(Gtk::Grid& source, InkPropertyGrid& grid, bool include_button_column, bool force_wide, int group_from_row) {
    auto unparent = [&source](auto& widget) {
        if (!widget) return widget;
        widget->reference();
        source.remove(*widget);
        return widget;
    };
    auto unref = [](auto& widget) {
        if (widget) {
            widget->unreference();
        }
    };

    auto add_property = [&](Gtk::Label* label, Gtk::Widget* button1, Gtk::Widget* w1, Gtk::Widget* w2, Gtk::Widget* btn) {
        WidgetGroup group;
        if (!w1 && !label) {
            group.add(grid.add_gap());
            return group;
        }

        bool in_row = false;
        bool take_up_btn_space = false;
        if (!w1) {
            in_row = true;
        }
        else if (!w2 && !button1) {
            int col, row, w, h;
            source.query_child(*w1, col, row, w, h);
            in_row = w > 1 || force_wide;
            take_up_btn_space = w == 5;
        }

        if (in_row) {
            if (take_up_btn_space) {
                group.add(grid.add_full_row(unparent(w1)));
                unref(w1);
            }
            else {
                group.add(grid.add_row(unparent(label), unparent(w1), unparent(btn)));
                if (label) grid.get_height_group()->add_widget(*label);
                unref(label);
                unref(w1);
                unref(btn);
            }
        }
        else {
            group.add(grid.add_property(unparent(label), unparent(button1), unparent(w1), unparent(w2), unparent(btn)));
            unref(label);
            unref(w1);
            unref(w2);
            unref(btn);
            unref(button1);
        }

        return group;
    };

    WidgetGroup items;
    for (int row = 0; row < 999; ++row) {
        // two consecutive empty rows - exit
        if (!source.get_child_at(0, row) && !source.get_child_at(0, row + 1) &&
            !source.get_child_at(2, row) && !source.get_child_at(2, row + 1)) break;

        int col = 0;
        auto label = dynamic_cast<Gtk::Label*>(source.get_child_at(col++, row));
        auto button1 = include_button_column ? dynamic_cast<Gtk::Button*>(source.get_child_at(col++, row)) : nullptr;
        auto w1 = source.get_child_at(col++, row);
        auto w2 = source.get_child_at(col++, row);
        if (w2 == w1) {
            w2 = nullptr;
        }
        if (button1 == w1 || button1 == w2) {
            button1 = nullptr;
        }
        auto button2 = source.get_child_at(col++, row);
        if (button2 == w1 || button2 == w2) {
            button2 = nullptr;
        }

        auto group = add_property(label, button1, w1, w2, button2);

        if (group.empty()) break;

        if (row >= group_from_row) {
            items.add(group);
        }
    }

    return items;
}

} // namespace
