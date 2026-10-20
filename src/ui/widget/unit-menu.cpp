// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "unit-menu.h"

namespace Inkscape::UI::Widget {

UnitMenu::UnitMenu() : _type(UNIT_TYPE_NONE)
{
    set_selected(0);
    property_selected().signal_changed().connect([this](){ on_changed(); });
}

UnitMenu::UnitMenu(BaseObjectType * const cobject, Glib::RefPtr<Gtk::Builder> const &builder)
    : DropDownList{cobject, builder}
{
    // We assume the UI file sets the active item & thus we do not do that here.

    property_selected().signal_changed().connect([this](){ on_changed(); });
}

UnitMenu::~UnitMenu() = default;

bool UnitMenu::setUnitType(UnitType unit_type, bool svg_length)
{
    // Expand the unit widget with unit entries from the unit table
    auto const &unit_table = Util::UnitTable::get();
    auto const &m = unit_table.units(unit_type);

    for (auto & i : m) {
        // We block the use of non SVG units if requested
        if (!svg_length || i->svgUnit() > 0) {
            append(i->abbr);
        }
    }
    _type = unit_type;
    setUnit(unit_table.primary(unit_type));

    return true;
}

bool UnitMenu::resetUnitType(UnitType unit_type, bool svg_length)
{
    remove_all();

    return setUnitType(unit_type, svg_length);
}

void UnitMenu::addUnit(Unit const& u)
{
    append(u.abbr);
}

Unit const * UnitMenu::getUnit() const
{
    auto const &unit_table = Util::UnitTable::get();
    auto current = get_selected_string();
    if (current.empty()) {
        g_assert(_type != UNIT_TYPE_NONE);
        return unit_table.getUnit(unit_table.primary(_type));
    }
    return unit_table.getUnit(current);
}

bool UnitMenu::setUnit(Glib::ustring const & unit)
{
    // TODO:  Determine if 'unit' is available in the dropdown.
    //        If not, return false

    // search for "unit" string
    // Note: find() method is not yet available (https://docs.gtk.org/gtk4/method.StringList.find.html), so use the loop
    // TODO: replace with "find" when it becomes practical to do so
    for (int i = 0; i < get_item_count(); ++i) {
        if (get_string(i) == unit) {
            set_selected(i);
            break;
        }
    }
    return true;
}

Glib::ustring UnitMenu::getUnitAbbr() const
{
    if (get_selected_string().empty()) {
        return "";
    }
    return getUnit()->abbr;
}

UnitType UnitMenu::getUnitType() const
{
    return getUnit()->type;
}

double  UnitMenu::getUnitFactor() const
{
    return getUnit()->factor;
}

int UnitMenu::getDefaultDigits() const
{
    return getUnit()->defaultDigits();
}

double UnitMenu::getDefaultStep() const
{
    return getUnit()->step;
}

double UnitMenu::getDefaultPage() const
{
    return 10 * getDefaultStep();
}

double UnitMenu::getConversion(Glib::ustring const &new_unit_abbr, Glib::ustring const &old_unit_abbr) const
{
    auto const &unit_table = Util::UnitTable::get();

    double old_factor = getUnit()->factor;
    if (old_unit_abbr != "no_unit") {
        old_factor = unit_table.getUnit(old_unit_abbr)->factor;
    }
    Unit const * new_unit = unit_table.getUnit(new_unit_abbr);

    // Catch the case of zero or negative unit factors (error!)
    if (old_factor < 0.0000001 ||
        new_unit->factor < 0.0000001) {
        // TODO:  Should we assert here?
        return 0.00;
    }

    return old_factor / new_unit->factor;
}

bool UnitMenu::isAbsolute() const
{
    return getUnitType() != UNIT_TYPE_DIMENSIONLESS;
}

bool UnitMenu::isRadial() const
{
    return getUnitType() == UNIT_TYPE_RADIAL;
}

Glib::SignalProxyProperty UnitMenu::signal_changed() {
    return property_selected().signal_changed();
}

void UnitMenu::on_changed() {
    // no op
}

Glib::ustring UnitMenu::get_selected_string() const {
    auto n = get_selected();
    return get_string(n);
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
