// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Johan B. C. Engelen
 *
 * Copyright (C) 2011 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "spinbutton.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/eventcontrollerfocus.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/checkbutton.h>

#include "scroll-utils.h"
#include "ui/controller.h"
#include "ui/tools/tool-base.h"
#include "ui/util.h"
#include "unit-menu.h"
#include "unit-tracker.h"
#include "util/expression-evaluator.h"
#include "util-string/ustring-format.h"

namespace Inkscape::UI::Widget {

MathSpinButton::MathSpinButton(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade) :
    Glib::ObjectBase("MathSpinButtonWrapper"), InkSpinButton(cobject)
{
    set_evaluator_function([this](auto& text) { return on_input(text); });
}

double MathSpinButton::on_input(const Glib::ustring& text) {
    return Util::ExpressionEvaluator{text.c_str()}.evaluate().value;
}

void SpinButton::_construct(BaseObjectType* cobject)
{
    auto const key = Gtk::EventControllerKey::create();
    key->signal_key_pressed().connect([this, &key = *key](auto &&...args) { return on_key_pressed(key, args...); }, true);
    key->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    add_controller(key);

    auto focus = Gtk::EventControllerFocus::create();
    focus->signal_enter().connect([this] {
        // When focus is obtained, save the value to enable undo later.
        _on_focus_in_value = get_value();
    });
    add_controller(focus);

    set_context_menu_callback([this]{ return on_popup_menu({}); });

    set_evaluator_function([this](auto& text) { return on_input(text); });

    InkSpinButton::signal_value_changed().connect([this](auto) {
        _signal_value_changed.emit();
    });

    signal_destroy().connect([this] { _unparentChildren(); });
}

double SpinButton::on_input(const Glib::ustring& text) {
    auto value = std::stod(text);
    if (_dont_evaluate) return value;

    Inkscape::Util::EvaluatorQuantity result;
    if (_unit_menu || _unit_tracker) {
        Unit const *unit = nullptr;
        if (_unit_menu) {
            unit = _unit_menu->getUnit();
        } else {
            unit = _unit_tracker->getActiveUnit();
        }
        result = Util::ExpressionEvaluator{text.c_str(), unit}.evaluate();
        // check if output dimension corresponds to input unit
        if (result.dimension != (unit->isAbsolute() ? 1 : 0) ) {
            throw Inkscape::Util::EvaluatorException("Input dimensions do not match with parameter dimensions.","");
        }
    } else {
        result = Util::ExpressionEvaluator{text.c_str()}.evaluate();
    }

    return result.value;
}

bool SpinButton::on_key_pressed(Gtk::EventControllerKey const &controller,
                                unsigned keyval, unsigned keycode, Gdk::ModifierType state)
{
    bool inc = false;
    double val = 0;

    if (_increment > 0) {
        constexpr auto modifiers = Gdk::ModifierType::SHIFT_MASK |
			   Gdk::ModifierType::CONTROL_MASK |
			   Gdk::ModifierType::ALT_MASK |
			   Gdk::ModifierType::SUPER_MASK |
			   Gdk::ModifierType::HYPER_MASK |
			   Gdk::ModifierType::META_MASK;
        // no modifiers pressed?
        if (!Controller::has_flag(state, modifiers)) {
            inc = true;
            val = get_value();
        }
    }

    switch (Inkscape::UI::Tools::get_latin_keyval(controller, keyval, keycode, state)) {
        case GDK_KEY_z:
        case GDK_KEY_Z:
            if (Controller::has_flag(state, Gdk::ModifierType::CONTROL_MASK)) {
                undo();
                return true; // I consumed the event
            }
            break;

        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            if (inc) {
                set_value(val + _increment);
                return true;
            }
            break;

        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            if (inc) {
                set_value(val - _increment);
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

void SpinButton::on_numeric_menu_item_activate(double value)
{
    get_adjustment()->set_value(value);
}

bool SpinButton::on_popup_menu(PopupMenuOptionalClick)
{
    if (!_custom_popup) {
        return false;
    }
    create_popover_menu();
    _popover_menu->popup_at_center(*this);
    return true;
}

void SpinButton::create_popover_menu()
{
    auto adj = get_adjustment();
    auto adj_value = adj->get_value();
    auto lower = adj->get_lower();
    auto upper = adj->get_upper();
    auto page = adj->get_page_increment();

    auto values = NumericMenuData{};

    for (auto const &custom_data : _custom_menu_data) {
        if (custom_data.first >= lower && custom_data.first <= upper) {
            values.emplace(custom_data);
        }
    }

    values.emplace(adj_value, "");
    values.emplace(std::fmin(adj_value + page, upper), "");
    values.emplace(std::fmax(adj_value - page, lower), "");

    if (!_popover_menu) {
        _popover_menu = std::make_unique<UI::Widget::PopoverMenu>(Gtk::PositionType::BOTTOM);
        _popover_menu->set_parent(*this);
    } else {
        _popover_menu->remove_all();
    }
    Gtk::CheckButton *group = nullptr;

    for (auto const &value : values) {
        bool const enable = adj_value == value.first;
        auto const item_label = !value.second.empty() ? Glib::ustring::compose("%1: %2", value.first, value.second)
                                                      : Inkscape::ustring::format_classic(value.first);
        auto const radio_button = Gtk::make_managed<Gtk::CheckButton>(item_label);
        if (!group) {
            group = radio_button;
        } else {
            radio_button->set_group(*group);
        }
        radio_button->set_active(enable);

        auto const item = Gtk::make_managed<UI::Widget::PopoverMenuItem>();
        item->set_child(*radio_button);
        item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &SpinButton::on_numeric_menu_item_activate), value.first));
        _popover_menu->append(*item);
    }
}

void SpinButton::undo()
{
    set_value(_on_focus_in_value);
}

void SpinButton::_unparentChildren()
{
    if (_popover_menu) {
        _popover_menu->unparent();
    }
}

SpinButton::~SpinButton()
{
    _unparentChildren();
}

void SpinButton::set_custom_numeric_menu_data(NumericMenuData &&custom_menu_data)
{
    _custom_popup = true;
    _custom_menu_data = std::move(custom_menu_data);
}

void SpinButton::set_increment(double delta) {
    _increment = delta;
}

void SpinButton::set_increments(double step, double page) {
    set_step(step);
    set_page_step(page);
}

void SpinButton::get_increments(double &step, double &page) const {
    auto& spin = const_cast<SpinButton&>(*this);
    step = spin.get_adjustment()->get_step_increment();
    page = spin.get_adjustment()->get_page_increment();
}

void SpinButton::get_range(double &min, double &max) const {
    auto& spin = const_cast<SpinButton&>(*this);
    auto adj = spin.get_adjustment();
    min = adj->get_lower();
    max = adj->get_upper();
}

void SpinButton::set_range(double min, double max) {
    auto adj = get_adjustment();
    adj->set_lower(min);
    adj->set_upper(max);
}

void SpinButton::set_width_chars(int chars) {
    property_width_chars().set_value(chars);
}

void SpinButton::set_max_width_chars(int chars) {
    //TODO if needed
}

Glib::ustring SpinButton::get_text() const {
    //TODO: remove
    return {};
}

int SpinButton::get_value_as_int() const {
    //TODO: round the value?
    return static_cast<int>(get_value());
}

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
