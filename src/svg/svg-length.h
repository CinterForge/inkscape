// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Carl Hetherington <inkscape@carlh.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_SVG_LENGTH_H
#define SEEN_SP_SVG_LENGTH_H

#include <string>
#include <optional>

namespace Inkscape::Util {
class Unit;
}

/**
 *  SVG length type
 */
class SVGLength {
public:
    SVGLength();

    enum Unit {
        NONE,
        PX,
        PT,
        PC,
        MM,
        CM,
        INCH,
        EM,
        EX,
        PERCENT,
        LAST_UNIT = PERCENT
    };

    // The object's value is valid / exists in SVG.
    bool _set;

    // If this was found through chaining hrefs (in gradients)
    bool _found = false;

    // The unit of value.
    Unit unit;

    // The value of this SVGLength as found in the SVG.
    double value;

    // The value in pixels (value * pixels/unit).
    double computed;

    double operator=(double v) {
        _set = true;
        unit = NONE;
        value = computed = v;
        return v;
    }

    bool operator==(const SVGLength& rhs) const {
        if (rhs.unit == unit) {
            if (unit == SVGLength::PERCENT)
                return value == rhs.value;
            return computed == rhs.computed;
        }
        return false;
    }
    bool operator!=(const SVGLength& rhs) const {
        return !(*this == rhs);
    }

    operator bool() const { return _set; }

    bool read(char const *str);
    void readOrUnset(char const *str, Unit u = NONE, double v = 0, double c = 0);
    bool readAbsolute(char const *str);
    Inkscape::Util::Unit const *getUnit() const;
    bool isAbsolute();

    std::string write() const;
    std::string toString(const std::string &unit, double doc_scale, std::optional<unsigned int> precision = {}, bool add_unit = true) const;
    double toValue(const std::string &out_unit) const;
    bool fromString(const std::string &input, const std::string &unit, std::optional<double> scale = {});

    // To set 'v' use '='
    void set(Unit u, double v); // Sets computed value based on u and v.
    void set(Unit u, double v, double c); // Sets all three values.
    void unset(Unit u = NONE, double v = 0, double c = 0);
    void unset(SVGLength const &l);
    void scale(double scale); // Scales length (value, computed), leaving unit alone.
    void update(double em, double ex, double scale); // Updates computed value
};

bool svg_length_absolute_unit(SVGLength::Unit unit);

namespace Inkscape {
char const *refX_named_to_percent(char const *str);
char const *refY_named_to_percent(char const *str);
} // namespace Inkscape

#endif // SEEN_SP_SVG_LENGTH_H

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
