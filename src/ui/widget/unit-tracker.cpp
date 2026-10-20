// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::UI::Widget::UnitTracker
 * Simple mediator to synchronize changes to unit menus
 *
 * Authors:
 *   Jon A. Cruz <jon@joncruz.org>
 *   Matthew Petroff <matthew@mpetroff.net>
 *
 * Copyright (C) 2007 Jon A. Cruz
 * Copyright (C) 2013 Matthew Petroff
 * Copyright (C) 2018 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "unit-tracker.h"

#include <cassert>
#include <iostream>
#include <gtkmm/liststore.h>

#include "combo-tool-item.h"

#define COLUMN_STRING 0

namespace Inkscape::UI::Widget {

UnitTracker::UnitTracker(UnitType unit_type) :
    _active(0),
    _isUpdating(false),
    _activeUnit(nullptr),
    _activeUnitInitialized(false),
    _store(nullptr),
    _priorValues()
{
    _store = Gio::ListStore<UnitObject>::create();

    _store->splice(0, 0, UnitTable::get().get_units(unit_type));

    _setActive(_active);
}

UnitTracker::~UnitTracker()
{
    _combo_list.clear();

    // Unhook weak references to GtkAdjustments
    for (auto i : _adjList) {
        g_object_weak_unref(G_OBJECT(i), _adjustmentFinalizedCB, this);
    }
    _adjList.clear();
}

bool UnitTracker::isUpdating() const
{
    return _isUpdating;
}

Unit const * UnitTracker::getActiveUnit() const
{
    return _activeUnit;
}

Glib::ustring UnitTracker::getCurrentLabel()
{
    return _store->get_item(_active)->unit.abbr;
}

void UnitTracker::setActiveUnit(Unit const *unit) {
    if (!unit) return;

    auto obj = UnitObject::from_unit(unit);
    auto [found, pos] = _store->find(obj, [](auto& a, auto& b) {
        return a->unit.abbr == b->unit.abbr;
    });
    if (found) {
        _setActive(pos);
    }
    else {
        g_warning("UnitTracker::setActiveUnit: unit '%s' not found!", unit->abbr.c_str());
    }
}

void UnitTracker::setActiveUnitByLabel(Glib::ustring label)
{
    for (int i = 0; i < _store->get_n_items(); ++i) {
        const auto& unit = _store->get_item(i)->unit;
        if (unit.name == label) {
            _setActive(i);
            return;
        }
    }

    g_warning("UnitTracker::setActiveUnitByLabel - unit '%s' not found", label.c_str());
}

void UnitTracker::setActiveUnitByAbbr(char const *abbr)
{
    auto unit = UnitTable::get().getUnit(abbr);
    if (abbr && unit->abbr != abbr) {
        // if abbreviation does not match any registered unit, create a temp one,
        // so we can use setActiveUnit method and let it search for a match in a _store
        auto tmp = Unit::create(abbr);
        setActiveUnit(tmp.get());
    }
    else {
        setActiveUnit(unit);
    }
}

void UnitTracker::addAdjustment(GtkAdjustment *adj)
{
    if (std::find(_adjList.begin(),_adjList.end(),adj) == _adjList.end()) {
        g_object_weak_ref(G_OBJECT(adj), _adjustmentFinalizedCB, this);
        _adjList.push_back(adj);
    } else {
        std::cerr << "UnitTracker::addAjustment: Adjustment already added!" << std::endl;
    }
}

void UnitTracker::addUnit(Unit const *u)
{
    _store->append(UnitObject::from_unit(u));
}

void UnitTracker::prependUnit(Unit const *u)
{
    _store->insert(0, UnitObject::from_unit(u));

    /* Re-shuffle our default selection here (_active gets out of sync) */
    setActiveUnit(_activeUnit);
}

void UnitTracker::setFullVal(GtkAdjustment * const adj, double const val)
{
    _priorValues[adj] = val;
}

UnitMenu* UnitTracker::create_unit_dropdown() {
    auto menu = new UnitMenu();
    menu->set_name("unit-tracker");
    menu->set_to_string_func([](auto& item){ return std::dynamic_pointer_cast<UnitObject>(item)->unit.abbr; });
    menu->set_model(_store);
    menu->set_selected(_active);
    menu->signal_changed().connect([this, menu]{ _setActive(menu->get_selected()); });
    _combo_list.push_back(menu);
    return menu;
}

void UnitTracker::_adjustmentFinalizedCB(gpointer data, GObject *where_the_object_was)
{
    if (data && where_the_object_was) {
        UnitTracker *self = reinterpret_cast<UnitTracker *>(data);
        self->_adjustmentFinalized(where_the_object_was);
    }
}

void UnitTracker::_adjustmentFinalized(GObject *where_the_object_was)
{
    GtkAdjustment* adj = (GtkAdjustment*)(where_the_object_was);
    auto it = std::find(_adjList.begin(),_adjList.end(), adj);
    if (it != _adjList.end()) {
        _adjList.erase(it);
    } else {
        g_warning("Received a finalization callback for unknown object %p", where_the_object_was);
    }
}

void UnitTracker::_setActive(gint active)
{
    auto const &unit_table = UnitTable::get();

    if (active == _active && _activeUnitInitialized) return;

    auto oldActive = _active;

    if (_store) {
        // Find old and new units
        Glib::ustring oldAbbr( "NotFound" );
        Glib::ustring newAbbr( "NotFound" );
        if (auto obj = _store->get_item(_active)) {
            oldAbbr = obj->unit.abbr;
        }
        if (auto obj = _store->get_item(active)) {
            newAbbr = obj->unit.abbr;
        }
        if (oldAbbr != "NotFound") {
            if (newAbbr != "NotFound") {
                auto oldUnit = unit_table.getUnit(oldAbbr);
                auto newUnit = unit_table.getUnit(newAbbr);
                _activeUnit = newUnit;

                if (!_adjList.empty()) {
                    _fixupAdjustments(oldUnit, newUnit);
                }
            } else {
                std::cerr << "UnitTracker::_setActive: Did not find new unit: " << active << std::endl;
            }
        } else {
            std::cerr << "UnitTracker::_setActive: Did not find old unit: " << oldActive
                      << "  new: " << active << std::endl;
        }
    }
    _active = active;

    for (auto combo : _combo_list) {
        if (combo) combo->set_selected(active);
    }

    _activeUnitInitialized = true;
}

void UnitTracker::_fixupAdjustments(Unit const *oldUnit, Unit const *newUnit)
{
    _isUpdating = true;
    for ( auto adj : _adjList ) {
        auto const oldVal = gtk_adjustment_get_value(adj);
        auto val = oldVal;

        if (oldUnit->type != UNIT_TYPE_DIMENSIONLESS
            && newUnit->type == UNIT_TYPE_DIMENSIONLESS)
        {
            val = newUnit->factor * 100;
            _priorValues[adj] = Quantity::convert(oldVal, oldUnit, "px");
        } else if (oldUnit->type == UNIT_TYPE_DIMENSIONLESS
            && newUnit->type != UNIT_TYPE_DIMENSIONLESS)
        {
            if (_priorValues.find(adj) != _priorValues.end()) {
                val = Quantity::convert(_priorValues[adj], "px", newUnit);
            }
        } else {
            val = Quantity::convert(oldVal, oldUnit, newUnit);
        }

        gtk_adjustment_set_value(adj, val);
    }
    _isUpdating = false;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
