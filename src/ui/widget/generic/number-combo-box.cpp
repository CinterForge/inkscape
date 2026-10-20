// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 1/7/26.
//

#include "number-combo-box.h"

#include <giomm/menuitem.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/accelerator.h>
#include <gtkmm/eventcontrollerkey.h>

#include "ui/containerize.h"

namespace Inkscape::UI::Widget {

NumberComboBox::NumberComboBox() :
    Glib::ObjectBase("NumberComboBox"),
    CssNameClassInit{"number-combobox"}
{
    construct();
}

NumberComboBox::NumberComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
    Glib::ObjectBase("NumberComboBox"),
    CssNameClassInit{"number-combobox"},
    BuildableWidget(cobject, builder)
{
    construct();
}


void NumberComboBox::construct() {
    // instance-specific action name:
    _action_name = Glib::ustring::format("number-combo-item-select-", this);
    _box.append(_number);
    _box.append(_menu_btn);
    _box.insert_at_start(*this);
    _box.add_css_class("linked");
    _menu_btn.set_halign(Gtk::Align::END);
    _menu_btn.set_can_focus(false);
    _popup.set_menu_model(_menu);
    _popup.set_has_arrow(false);
    _menu_btn.set_popover(_popup);
    _number.signal_value_changed().connect([this](auto value) {
        _signal_value_changed.emit(value);
    });
    // action used by our combo
    auto action_group = Gio::SimpleActionGroup::create();
    action_group->add_action_with_parameter(_action_name, Glib::Variant<Glib::ustring>::variant_type(), [this] (const Glib::VariantBase& param) {
        auto str = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(param).get();
        auto index = std::stoi(str);
        if (index >= 0 && index < _list.size()) {
            auto value = _list[index];
            _number.set_value(value);
        }
    });
    insert_action_group("win", action_group);

    _key_entry = Gtk::EventControllerKey::create();
    _key_entry->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    _key_entry->signal_key_pressed().connect([this](auto keyval, auto keycode, auto modifiers){ return on_key_pressed(keyval, modifiers); }, false); // Before default handler.
    add_controller(_key_entry);
    containerize(*this);

    // align popup menu with combobox rather than menu button
    _popup.signal_show().connect([this] {
        auto alloc = get_allocation();
        double x = 0, y = 0;
        translate_coordinates(_menu_btn, 0, 0, x, y);
        _popup.set_pointing_to(Gdk::Rectangle(x, y, alloc.get_width(), alloc.get_height()));
    });
}

bool NumberComboBox::on_key_pressed(guint keyval, Gdk::ModifierType state) {
    state &= Gtk::Accelerator::get_default_mod_mask();

    switch (keyval) {
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(+1);
            return true;
        }
        else if (state == Gdk::ModifierType::ALT_MASK) {
            _menu_btn.popdown();
            return true;
        }
        break;
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(+1);
            return true;
        }
        break;
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(-1);
            return true;
        }
        else if (state == Gdk::ModifierType::ALT_MASK) {
            _menu_btn.popup();
            return true;
        }
        break;
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(-1);
            return true;
        }
        break;
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_item(0);
            return true;
        }
        break;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_item(_list.size() - 1);
            return true;
        }
        break;
    }
    return false;
}

void NumberComboBox::select_next(int delta) {
    if (_list.empty()) return;

    select_item(find_index(_number.get_value()) + delta);
}

void NumberComboBox::select_item(int index) {
    if (index >= 0 && index < _list.size()) {
        auto value = _list[index];
        _number.set_value(value);
    }
}

int NumberComboBox::find_index(double value) const {
    if (_list.empty()) return -1;

    auto it = std::ranges::lower_bound(_list, value);
    return std::distance(begin(_list), it);
}

void NumberComboBox::measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural,
    int& minimum_baseline, int& natural_baseline) const {
    _box.measure(orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
}

void NumberComboBox::size_allocate_vfunc(int width, int height, int baseline) {
    Gtk::Allocation allocation;
    allocation.set_height(height);
    allocation.set_width(width);
    allocation.set_x(0);
    allocation.set_y(0);
    _box.size_allocate(allocation, baseline);

    _popup.set_size_request(width, -1);
    _popup.queue_resize();
}

void NumberComboBox::append(double value) {
    Glib::ustring label = InkSpinButton::format_number(value, _number.get_digits(), true, false);
    auto menu_item = Gio::MenuItem::create(label, "");
    double index = _menu->get_n_items();
    // note: using string target instead of more sensible int, since only string allows menu items to be enabled
    menu_item->set_action_and_target("win." + _action_name, Glib::Variant<Glib::ustring>::create(std::to_string(index).c_str()));
    _menu->append_item(menu_item);
    _list.push_back(value);
}

void NumberComboBox::set_selected_item(int index) {
    if (index >= 0 && index < _list.size()) {
        _number.set_value(_list[index]);
    }
}

void NumberComboBox::set_value(double value) {
    _number.set_value(value);
}

void NumberComboBox::set_menu_options(const std::vector<double>& list) {
    _list.clear();
    _list.reserve(list.size());
    _menu->remove_all();
    for (auto value : list) {
        append(value);
    }
}

void NumberComboBox::set_popup_position(Gtk::PositionType position) {
    _popup.set_position(position);
}

} // namespace
