// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape Units
 * These classes are used for defining different unit systems.
 *
 * Copyright (C) 2013-2025 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UTIL_UNITS_H
#define INKSCAPE_UTIL_UNITS_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <boost/operators.hpp>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <glibmm/ustring_hash.h>
#include <2geom/coord.h>
#include <glibmm/object.h>

#include "svg/svg-length.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define DEFAULT_UNIT_NAME "mm"
#endif

namespace Inkscape::Util {

enum UnitType {
    UNIT_TYPE_DIMENSIONLESS,     /* Percentage */
    UNIT_TYPE_LINEAR,
    UNIT_TYPE_LINEAR_SCALED,
    UNIT_TYPE_RADIAL,
    UNIT_TYPE_TIME,
    UNIT_TYPE_FONT_HEIGHT,
    UNIT_TYPE_QTY,
    UNIT_TYPE_NONE = -1
};

const char DEG[] = "°";

struct UnitMetric
{
    Glib::ustring name;
    std::vector<double> ruler_scale;
    std::vector<int>    subdivide;
};

class Unit : boost::equality_comparable<Unit>
{
public:
    Unit() = default;
    Unit(UnitType type,
         double factor,
         Glib::ustring name,
         Glib::ustring name_plural,
         Glib::ustring abbr,
         Glib::ustring description);
    Unit(const Unit& src) = default;

    // create a new Unit definition with abbreviation alone; this is useful when
    // we want to add an item to a list of units, like "lines" for line-height
    static std::unique_ptr<const Unit> create(const Glib::ustring& abbr) {
        return std::make_unique<Unit>(UnitType::UNIT_TYPE_DIMENSIONLESS, 1.0, abbr, "", abbr, "");
    }

    bool           isAbsolute() const { return type != UNIT_TYPE_DIMENSIONLESS; }

    /**
     * Returns the suggested precision to use for displaying numbers
     * of this unit.
     */
    int            defaultDigits() const;

    /** Checks if a unit is compatible with the specified unit. */
    bool           compatibleWith(Unit const *u) const;
    bool           compatibleWith(Glib::ustring const &) const;
    bool           compatibleWith(char const *) const;

    UnitType       type = UNIT_TYPE_DIMENSIONLESS;
    double         factor = 1.0;
    Glib::ustring  name;
    Glib::ustring  name_plural;
    Glib::ustring  abbr;
    Glib::ustring  description;
    Glib::ustring  metric_name;
    int            precision = 3; // show that many digits after decimal point in spin buttons
    double         step = 0.1;    // increment/decrement step for spin buttons

    /** Check if units are equal. */
    bool operator==(Unit const &other) const;

    /** Get SVG unit code. */
    int svgUnit() const;

    /** Convert value from this unit **/
    double convert(double from_dist, Unit const *to) const;
    double convert(double from_dist, Glib::ustring const &to) const;
    double convert(double from_dist, char const *to) const;

    /** Get the ways this unit is subdivided in rulers **/
    UnitMetric const *getUnitMetric() const;
};

class Quantity
    : boost::totally_ordered<Quantity>
{
public:
    Unit const *unit;
    double quantity;
    
    /** Initialize a quantity. */
    Quantity(double q, Unit const *u);
    Quantity(double q, Glib::ustring const &u);
    Quantity(double q, char const *u);
    
    /** Checks if a quantity is compatible with the specified unit. */
    bool compatibleWith(Unit const *u) const;
    bool compatibleWith(Glib::ustring const &u) const;
    bool compatibleWith(char const *u) const;
    
    /** Return the quantity's value in the specified unit. */
    double value(Unit const *u) const;
    double value(Glib::ustring const &u) const;
    double value(char const *u) const;
    
    /** Return a printable string of the value in the specified unit. */
    Glib::ustring string(Unit const *u) const;
    Glib::ustring string(Glib::ustring const &u) const;
    Glib::ustring string() const;
    
    /** Convert distances. 
       no NULL check is performed on the passed pointers to Unit objects!  */
    static double convert(double from_dist, Unit const *from, Unit const *to);
    static double convert(double from_dist, Glib::ustring const &from, Unit const *to);
    static double convert(double from_dist, Unit const *from, Glib::ustring const &to);
    static double convert(double from_dist, Glib::ustring const &from, Glib::ustring const &to);
    static double convert(double from_dist, char const *from, char const *to);

    /** Comparison operators. */
    bool operator<(Quantity const &rhs) const;
    bool operator==(Quantity const &other) const;
};

inline bool are_near(Quantity const &a, Quantity const &b, double eps=Geom::EPSILON)
{
    return Geom::are_near(a.quantity, b.value(a.unit), eps);
}

// Gio ListModel-friendly unit wrapper
struct UnitObject: Glib::Object {
    Unit unit;

    UnitObject(Unit const *p) : unit(*p) {}

    static Glib::RefPtr<UnitObject> from_unit(Unit const *unit) {
        return Glib::make_refptr_for_instance(new UnitObject(unit));
    }
};

class UnitTable final {
public:
    /**
     * Initializes the unit tables and identifies the primary unit types.
     *
     * The primary unit's conversion factor is required to be 1.00
     */
    UnitTable();
    UnitTable(std::string const &filename);
    ~UnitTable() = default;

    UnitTable(UnitTable const &t) = delete;
    UnitTable operator = (UnitTable const &t) = delete;

    // This is split into units-internal.cpp for unit test building
    static std::string getUnitsFilename();

    typedef std::unordered_map<Glib::ustring, UnitMetric> MetricMap;
    typedef std::unordered_map<unsigned, Unit const *> UnitCodeMap;

    /** Unit metrics **/
    void    addMetric(UnitMetric const &u, bool primary);
    UnitMetric const *getUnitMetric(Glib::ustring const &name) const;

    /** Add a new unit to the table */
    void    addUnit(std::unique_ptr<Unit> unit, bool primary);

    /** Retrieve a given unit based on its string identifier */
    Unit const *getUnit(Glib::ustring const &name) const;
    Unit const *getUnit(char const *name) const;

    // Take a predefined unit definition
    Unit const *unit(const std::string& abbr) const;
    Unit const *unit(const char* abbr) const;

    /** Try to find a unit based on its conversion factor to the primary */
    Unit const *findUnit(double factor, UnitType type) const;

    /** Retrieve a given unit based on its SVGLength unit */
    Unit const *getUnit(SVGLength::Unit u) const;

    /** Retrieve a quantity based on its string identifier */
    Quantity parseQuantity(Glib::ustring const &q) const;

    /** Remove a unit definition from the given unit type table * /
     * DISABLED, unsafe with the current passing around pointers to Unit objects in this table */
    //bool    deleteUnit(Unit const &u);

    /** Returns true if the given string 'name' is a valid unit in the table */
    bool    hasUnit(Glib::ustring const &name) const;

    /** Provides an iterable list of items in the given unit table */
    std::vector<Unit const *> units(UnitType type) const;
    std::vector<Glib::RefPtr<UnitObject>> get_units(UnitType type) const;

    /** Returns the default unit abbr for the given type */
    Glib::ustring primary(UnitType type) const;

    double  getScale() const;

    void    setScale();

    /** Load units from an XML file.
     *
     * Loads and merges the contents of the given file into the UnitTable,
     * possibly overwriting existing unit definitions.
     *
     * @param filename file to be loaded
     */
    bool    load(std::string const &filename);

    static UnitTable &get();

private:
    // keep track of units
    std::vector<std::unique_ptr<Unit>>_ordered_store;
    MetricMap           _metric_map;
    UnitCodeMap         _unit_map;
    Glib::ustring       _primary_unit[UNIT_TYPE_QTY];
    Glib::ustring       _default_metric;
    double              _linear_scale;
    static Unit         _empty_unit;
};

}

#endif // define INKSCAPE_UTIL_UNITS_H
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
