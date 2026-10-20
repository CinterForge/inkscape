// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *   Derek P. Moore <derekm@hackunix.org>
 *   buliabyak@gmail.com
 *
 * Copyright (C) 2004-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_SCALAR_UNIT_H
#define INKSCAPE_UI_WIDGET_SCALAR_UNIT_H

#include "scalar.h"
#include "unit-menu.h"

namespace Inkscape::UI::Widget {

/**
 * A labelled text box, with spin buttons and optional icon, for
 * entering the values of various unit types.
 *
 * A ScalarUnit is a control for entering, viewing, or manipulating
 * numbers with units.  This differs from ordinary numbers like 2 or
 * 3.14 because the number portion of a scalar *only* has meaning
 * when considered with its unit type.  For instance, 12 m and 12 in
 * have very different actual values, but 1 m and 100 cm have the same
 * value.  The ScalarUnit allows us to abstract the presentation of
 * the scalar to the user from the internal representations used by
 * the program.
 */
class ScalarUnit : public Scalar
{
public:
    /**
     * Construct a ScalarUnit.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param unit_type  Unit type (defaults to UNIT_TYPE_LINEAR).
     * @param icon       Icon name, placed before the label (defaults to empty).
     * @param unit_menu  UnitMenu drop down; if not specified, one will be created
     *                   and displayed after the widget (defaults to NULL).
     * @param should_convert_limits If true, the values of the minimum and
                                    maximum limits will be converted when the
                                    units of the input value are changed.
     */
    ScalarUnit(Glib::ustring const &label, Glib::ustring const &tooltip,
               UnitType unit_type = UNIT_TYPE_LINEAR,
               Glib::ustring const &icon = {},
               UnitMenu *unit_menu = nullptr,
               bool should_convert_limits = false);

    /**
     * Construct a ScalarUnit.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param take_unitmenu  Use the unitmenu from this parameter.
     * @param icon       Icon name, placed before the label (defaults to empty).
     */
    ScalarUnit(Glib::ustring const &label, Glib::ustring const &tooltip,
               ScalarUnit &take_unitmenu,
               Glib::ustring const &icon = {});

    /**
     * Initializes the scalar based on the settings in _unit_menu.
     * Requires that _unit_menu has already been initialized.
     */
    void      initScalar(double min_value, double max_value);

    /**
     * Gets the object for the currently selected unit.
     */
    Unit const * getUnit() const;

    /**
     * Gets the UnitType ID for the unit.
     */
    UnitType  getUnitType() const;

    /**
     * Returns the value in the given unit system.
     */
    double    getValue(Glib::ustring const &units) const;

    /**
     * Sets the unit for the ScalarUnit widget.
     */
    bool      setUnit(Glib::ustring const &units);

    /**
     * Adds the unit type to the ScalarUnit widget.
     */
    void      setUnitType(UnitType unit_type);

    /**
     * Resets the unit type for the ScalarUnit widget.
     */
    void      resetUnitType(UnitType unit_type);

    /**
     * allow align text in entry.
     */
    void      setAlignment(double xalign);

    /**
     * Sets the number and unit system.
     */
    void      setValue(double number, Glib::ustring const &units);

    /**
     * Convert and sets the number only and keeps the current unit.
     */
    void      setValueKeepUnit(double number, Glib::ustring const &units);

    /**
     * Sets the number only.
     */
    void      setValue(double number);

    /**
     * Grab focus, and select the text that is in the entry field.
     */
    void      grabFocusAndSelectEntry();

    void      setHundredPercent(double number);

    void      setAbsoluteIsIncrement(bool value);

    void      setPercentageIsIncrement(bool value);

    /**
     * Convert value from % to absolute, using _hundred_percent and *_is_increment flags.
     */
    double PercentageToAbsolute(double value);

    /**
     * Convert value from absolute to %, using _hundred_percent and *_is_increment flags.
     */
    double AbsoluteToPercentage(double value);

    /**
     * Assuming the current unit is absolute, get the corresponding % value.
     */
    double getAsPercentage();

    /**
     * Assuming the current unit is absolute, set the value corresponding to a given %.
     */
    void setFromPercentage(double value);

    /**
     * Signal handler for updating the value when unit is changed.
     */
    void on_unit_changed();

protected:
    double convertValue(double old_value, Inkscape::Util::Unit const &old_unit, Inkscape::Util::Unit const &new_unit);

    UnitMenu  *_unit_menu;

    double _hundred_percent; // the length that corresponds to 100%, in px, for %-to/from-absolute conversions

    bool _absolute_is_increment; // if true, 120% with _hundred_percent=100px gets converted to/from 20px; otherwise, to/from 120px
    bool _percentage_is_increment; // if true, 120px with _hundred_percent=100px gets converted to/from 20%; otherwise, to/from 120%
                                            // if both are true, 20px is converted to/from 20% if _hundred_percent=100px

    // If true, range limits will be converted when the input unit is changed.
    // This is paramaterized as it's a desirable behavior, but behaves poorly
    // in some circumstances, e.g. when converting to/from percentages when
    // the hundred percent value has not been properly initialized.
    bool should_convert_limits;

    Glib::ustring lastUnits; // previously selected unit, for conversions
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_SCALAR_UNIT_H

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
