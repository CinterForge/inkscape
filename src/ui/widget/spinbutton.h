// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Johan B. C. Engelen
 *
 * Copyright (C) 2011 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_SPINBUTTON_H
#define INKSCAPE_UI_WIDGET_SPINBUTTON_H

#include <gtkmm/spinbutton.h>

#include "generic/bin.h"
#include "generic/spin-button.h"
#include "ui/popup-menu.h"
#include "ui/widget/generic/popover-menu.h"

namespace Gtk {
class Builder;
class EventControllerKey;
} // namespace Gtk

namespace Inkscape::UI::Widget {

class UnitMenu;
class UnitTracker;

/**
 * A spin button for use with builders.
 */
class MathSpinButton : public InkSpinButton
{
public:
    MathSpinButton(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);

private:
    double on_input(const Glib::ustring& text);
};

/**
 * SpinButton widget, that allows entry of simple math expressions (also units, when linked with UnitMenu),
 * and allows entry of both '.' and ',' for the decimal, even when in numeric mode.
 *
 * Calling "set_numeric()" effectively disables the expression parsing. If no unit menu is linked, all unitlike characters are ignored.
 */
class SpinButton : public InkSpinButton
{
public:
    using NumericMenuData = std::map<double, Glib::ustring>;

    SpinButton(BaseObjectType *cobject, Glib::RefPtr<Gtk::Builder> const & b):
        Glib::ObjectBase("SpinButtonWrapper"), InkSpinButton(cobject, b)
    { _construct(cobject); }

    SpinButton(BaseObjectType *cobject):
        Glib::ObjectBase("SpinButtonWrapper"), InkSpinButton(cobject)
    { _construct(cobject); }

    explicit SpinButton(double climb_rate = 0.0, guint digits = 0):
        Glib::ObjectBase("SpinButtonWrapper")
    {
        _construct();
        set_digits(digits);
    }

    explicit SpinButton(const Glib::RefPtr<Gtk::Adjustment>& adjustment, double climb_rate = 0.0, guint digits = 0):
        Glib::ObjectBase("SpinButtonWrapper")
    {
        _construct();
        set_adjustment(adjustment);
        set_digits(digits);
    }

    ~SpinButton() override;

    void setUnitMenu(UnitMenu* unit_menu) { _unit_menu = unit_menu; };
    void addUnitTracker(UnitTracker* ut) { _unit_tracker = ut; };

    // TODO: Might be better to just have a default value and a reset() method?
    inline void set_zeroable(const bool zeroable = true) { _zeroable = zeroable; }
    inline void set_oneable(const bool oneable = true) { _oneable = oneable; }

    inline bool get_zeroable() const { return _zeroable; }
    inline bool get_oneable() const { return _oneable; }

    // set key up/down increment to override spin button adjustment step setting
    void set_increment(double delta);
    void set_increments(double step, double page);
    void get_increments(double& step, double& page) const;
    void get_range(double& min, double& max) const;
    void set_range(double min, double max);
    void set_width_chars(int chars);
    void set_max_width_chars(int chars);
    Glib::ustring get_text() const;
    int get_value_as_int() const;
    sigc::signal<void ()>& signal_value_changed() { return _signal_value_changed; }

private:
    UnitMenu    *_unit_menu    = nullptr; ///< Linked unit menu for unit conversion in entered expressions.
    UnitTracker *_unit_tracker = nullptr; ///< Linked unit tracker for unit conversion in entered expressions.
    double _on_focus_in_value  = 0.;
    bool _zeroable = false; ///< Reset-value should be zero
    bool _oneable  = false; ///< Reset-value should be one
    bool _dont_evaluate = false; ///< Don't attempt to evaluate expressions
    NumericMenuData _custom_menu_data;
    bool _custom_popup = false;
    double _increment = 0.0;    // if > 0, key up/down will increment/decrement current value by this amount
    std::unique_ptr<UI::Widget::PopoverMenu> _popover_menu;
    sigc::signal<void ()> _signal_value_changed;

    void _construct(BaseObjectType* cobject = nullptr);

    /**
     * This callback function should try to convert the entered text to a number and write it to newvalue.
     * It calls a method to evaluate the (potential) mathematical expression.
     *
     * @retval false No conversion done, continue with default handler.
     * @retval true  Conversion successful, don't call default handler.
     */
    double on_input(const Glib::ustring& text);

    /**
     * Handle specific keypress events, like Ctrl+Z.
     *
     * @retval false continue with default handler.
     * @retval true  don't call default handler.
     */
    bool on_key_pressed(Gtk::EventControllerKey const &controller,
                        unsigned keyval, unsigned keycode, Gdk::ModifierType state);

    bool on_popup_menu(PopupMenuOptionalClick);
    void create_popover_menu();
    void on_numeric_menu_item_activate(double value);

    /**
     * Undo the editing, by resetting the value upon when the spinbutton got focus.
     */
    void undo();

    void _unparentChildren();

public:
    void set_custom_numeric_menu_data(NumericMenuData &&custom_menu_data);
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_SPINBUTTON_H

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
