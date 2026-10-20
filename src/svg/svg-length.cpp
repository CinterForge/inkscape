// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG data parser
 *//*
 * Authors: see git history
 
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cmath>
#include <cstring>
#include <string>
#include <glib.h>
#include <iostream>
#include <vector>

#include "svg.h"
#include "stringstream.h"
#include "util/units.h"
#include "util/numeric/converters.h"

using std::pow;

bool parse_number_with_unit(char const *ss, SVGLength::Unit &unit, double &value, double &computed, bool abs, char **next);

#ifndef MAX
# define MAX(a,b) ((a < b) ? (b) : (a))
#endif

unsigned int sp_svg_number_read_f(gchar const *str, float *val)
{
    if (!str) {
        return 0;
    }

    char *e;
    float const v = g_ascii_strtod(str, &e);
    if ((gchar const *) e == str) {
        return 0;
    }

    *val = v;
    return 1;
}

unsigned int sp_svg_number_read_d(gchar const *str, double *val)
{
    if (!str) {
        return 0;
    }

    char *e;
    double const v = g_ascii_strtod(str, &e);
    if ((gchar const *) e == str) {
        return 0;
    }

    *val = v;
    return 1;
}

static std::string sp_svg_number_write_d( double val, unsigned int tprec, unsigned int fprec)
{

    std::string buf;
    /* Process sign */
    if (val < 0.0) {
        buf.append("-");
        val = fabs(val);
    }

    /* Determine number of integral digits */
    int idigits = 0;
    if (val >= 1.0) {
        idigits = (int) floor(log10(val)) + 1;
    }

    /* Determine the actual number of fractional digits */
    fprec = MAX(static_cast<int>(fprec), static_cast<int>(tprec) - idigits);
    /* Round value */
    val += 0.5 / pow(10.0, fprec);
    /* Extract integral and fractional parts */
    double dival = floor(val);
    double fval = val - dival;
    /* Write integra */
    if (idigits > (int)tprec) {
        buf.append(std::to_string((unsigned int)floor(dival/pow(10.0, idigits-tprec) + .5)));
        for(unsigned int j=0; j<(unsigned int)idigits-tprec; j++) {
            buf.append("0");
        }
    } else {
       buf.append(std::to_string((unsigned int)dival));
    }

    if (fprec > 0 && fval > 0.0) {
        std::string s(".");
        do {
            fval *= 10.0;
            dival = floor(fval);
            fval -= dival;
            int const int_dival = (int) dival;
            s.append(std::to_string(int_dival));
            if(int_dival != 0){
                buf.append(s);
                s="";
            }
            fprec -= 1;
        } while(fprec > 0 && fval > 0.0);
    }
    return buf;
}

std::string sp_svg_number_write_de(double val, unsigned int tprec, int min_exp)
{
    std::string buf;
    int eval = (int)floor(log10(fabs(val)));
    if (val == 0.0 || eval < min_exp) {
        buf.append("0");
        return buf;
    }
    unsigned int maxnumdigitsWithoutExp = // This doesn't include the sign because it is included in either representation
        eval<0?tprec+(unsigned int)-eval+1:
        eval+1<(int)tprec?tprec+1:
        (unsigned int)eval+1;
    unsigned int maxnumdigitsWithExp = tprec + ( eval<0 ? 4 : 3 ); // It's not necessary to take larger exponents into account, because then maxnumdigitsWithoutExp is DEFINITELY larger
    if (maxnumdigitsWithoutExp <= maxnumdigitsWithExp) {
        buf.append(sp_svg_number_write_d(val, tprec, 0));
    } else {
        val = eval < 0 ? val * pow(10.0, -eval) : val / pow(10.0, eval);
        buf.append(sp_svg_number_write_d(val, tprec, 0));
        buf.append("e");
        buf.append(std::to_string(eval));
    }
    return buf;

}

SVGLength::SVGLength()
    : _set(false)
    , unit(NONE)
    , value(0)
    , computed(0)
{
}

/* Length */

bool SVGLength::read(gchar const *str)
{
    _set = parse_number_with_unit(str, unit, value, computed, false);
    return _set;
}

bool SVGLength::readAbsolute(gchar const *str)
{
    _set = parse_number_with_unit(str, unit, value, computed, true);
    return _set;
}

/**
 * Returns the unit used as a string.
 *
 * @returns unit string
 */
Inkscape::Util::Unit const *SVGLength::getUnit() const
{
    static auto const &unit_table = Inkscape::Util::UnitTable::get();
    return unit_table.getUnit(unit);
}

/**
 * Is this length an absolute value (uses an absolute unit).
 *
 * @returns true if unit is not NONE and not a relative unit (percent etc)
 */
bool SVGLength::isAbsolute()
{
    return unit && svg_length_absolute_unit(unit);
}

std::vector<SVGLength> sp_svg_length_list_read(gchar const *str)
{
    if (!str) {
        return {};
    }

    SVGLength::Unit unit;
    double value;
    double computed;
    char *next = (char *) str;
    std::vector<SVGLength> list;

    while (parse_number_with_unit(next, unit, value, computed, false, &next)) {
        SVGLength length;
        length.set(unit, value, computed);
        list.push_back(length);
        // Allow for a single comma in the number list between values
        if (next && *next == ',') {
            next++;
        }
    }
    return list;
}

/**
 * Parse a number with an optional unit
 *
 * @arg ss - The string stream to read, will resposition to after last read value
 * @returns unit - The unit, ususally an empty string
 * @returns val - The value read in without adjustment
 * @returns computed - The value read in with adjustment
 * @arg abs - If true limits output to absolute units only
 * @arg next - The char position after a successful read. Does not change if parsing failed.
 *
 * @returns true if a number and unit was parsed correctly and next was advanced.
 */
bool parse_number_with_unit(char const *str, SVGLength::Unit &unit, double &val, double &computed, bool abs, char **next)
{
    if (!str) {
        return false;
    }
    char *end = nullptr;
    double value = g_ascii_strtod(str, &end);

    // Parsing failed.
    if (end == str || !std::isfinite(value)) {
        return false;
    }

    // Collect the unit, no spaces are allowed after the number
    std::string unit_str;
    while (end && (*end == '%' || (*end >= 'a' && *end <= 'z') || (*end >= 'A' && *end <= 'Z'))) {
        unit_str += *(end++);
    }
    // Trim the remaining spaces
    while (end && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')) {
        end++;
    }

    static auto const &unit_table = Inkscape::Util::UnitTable::get();
    // There might be a few bugs in UunitTable, parsing pxt as px and calling em and ex Absolute units
    if (auto u = unit_table.getUnit(unit_str); unit_str == u->abbr.c_str()) {
        if (abs && !(u->isAbsolute() && u->type != Inkscape::Util::UNIT_TYPE_FONT_HEIGHT) && unit_str.size()) {
            return false;
        }
        // If we expect to parse more, return that pointer. Otherwise fail.
        if (next) {
            *next = end;
        } else if (*end) { // There were more letters, which is a failure
            return false;
        }

        unit = (SVGLength::Unit)u->svgUnit();

        // Percent is handled as it's own computed value (FIXME!)
        if (unit == SVGLength::Unit::PERCENT) {
            val = value / 100.0;
            computed = val;
            return true;
        }
        val = value;
        computed = unit ? u->convert(value, "px") : value;
        return true;
    }
    return false;
}


std::string SVGLength::write() const
{
    return sp_svg_length_write_with_units(*this);
}

/**
 * Write out length in user unit, for the user to use.
 *
 * @param out_unit - The unit to convert the computed px into
 * @returns a string containing the value in the given units
 */
std::string SVGLength::toString(const std::string &out_unit, double doc_scale, std::optional<unsigned int> precision, bool add_unit) const
{
    if (unit == SVGLength::PERCENT) {
        return write();
    }
    double value = toValue(out_unit) * doc_scale;
    Inkscape::SVGOStringStream os;
    if (precision) {
        os << Inkscape::Util::format_number(value, *precision);
    } else {
        os << value;
    }
    if (add_unit)
        os << out_unit;
    return os.str();
}

/**
 * Caulate the length in a user unit.
 *
 * @param out_unit - The unit to convert the computed px into
 * @returns a double of the computed value in this unit
 */
double SVGLength::toValue(const std::string &out_unit) const
{
    return Inkscape::Util::Quantity::convert(computed, "px", out_unit);
}

/**
 * Read from user input, any non-unitised value is converted internally.
 *
 * @param input - The string input
 * @param default_unit - The unit used by the display. Set to empty string for xml reading.
 * @param doc_scale - The scale values with units should apply to make those units correct for this document.
 */
bool SVGLength::fromString(const std::string &input, const std::string &default_unit, std::optional<double> doc_scale)
{
    if (!read((input + default_unit).c_str()))
        if (!read(input.c_str()))
            return false;
    // Rescale real units to document, since user input is not scaled
    if (doc_scale && unit != SVGLength::PERCENT && unit != SVGLength::NONE) {
        value = computed;
        unit = SVGLength::NONE;
        scale(1 / *doc_scale);
    }
    return true;
}

void SVGLength::set(SVGLength::Unit u, double v)
{
    _set = true;
    unit = u;
    value = v;
    computed = getUnit()->convert(v, "px");
}

void SVGLength::set(SVGLength::Unit u, double v, double c)
{
    _set = true;
    unit = u;
    value = v;
    computed = c;
}

void SVGLength::unset(SVGLength::Unit u, double v, double c)
{
    _set = false;
    unit = u;
    value = v;
    computed = c;
}

void SVGLength::unset(const SVGLength &l)
{
    _set = false;
    _found = l._found;
    unit = l.unit;
    value = l.value;
    computed = l.computed;
}

void SVGLength::scale(double scale)
{
    value *= scale;
    computed *= scale;
}

void SVGLength::update(double em, double ex, double scale)
{
    if (unit == EM) {
        computed = value * em;
    } else if (unit == EX) {
        computed = value * ex;
    } else if (unit == PERCENT) {
        computed = value * scale;
    }
}

double sp_svg_read_percentage(char const *str, double def)
{
    double value;
    double computed;
    SVGLength::Unit unit;
    if (parse_number_with_unit(str, unit, value, computed, false)) {
        if (unit == SVGLength::Unit::NONE || unit == SVGLength::Unit::PERCENT) {
            return value;
        }
    }
    return def;
}

bool svg_length_absolute_unit(SVGLength::Unit u)
{
    return (u != SVGLength::EM && u != SVGLength::EX && u != SVGLength::PERCENT);
}

/**
 * N.B.\ This routine will sometimes return strings with `e' notation, so is unsuitable for CSS
 * lengths (which don't allow scientific `e' notation).
 */
std::string sp_svg_length_write_with_units(SVGLength const &length)
{
    Inkscape::SVGOStringStream os;
    if (length.unit == SVGLength::PERCENT) {
        os << 100*length.value << length.getUnit()->abbr.c_str();
    } else if (length.unit == SVGLength::PX) {
        os << length.value;
    } else {
        os << length.value << length.getUnit()->abbr.c_str();
    }
    return os.str();
}


void SVGLength::readOrUnset(gchar const *str, Unit u, double v, double c)
{
    if (!read(str)) {
        unset(u, v, c);
    }
}

namespace Inkscape {
char const *refX_named_to_percent(char const *str)
{
    if (str) {
        if (g_str_equal(str, "left")) {
            return "0%";
        } else if (g_str_equal(str, "center")) {
            return "50%";
        } else if (g_str_equal(str, "right")) {
            return "100%";
        }
    }
    return str;
}

char const *refY_named_to_percent(char const *str)
{
    if (str) {
        if (g_str_equal(str, "top")) {
            return "0%";
        } else if (g_str_equal(str, "center")) {
            return "50%";
        } else if (g_str_equal(str, "bottom")) {
            return "100%";
        }
    }
    return str;
}
} // namespace Inkscape

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
