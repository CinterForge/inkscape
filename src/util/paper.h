// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape Paper Sizes
 *
 * Authors:
 *
 * Copyright (C) 2013 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <vector>
#include <2geom/point.h>

#include "units.h"

#ifndef INKSCAPE_UTIL_PAPER_H
#define INKSCAPE_UTIL_PAPER_H

namespace Inkscape {

/**
 * Data class used to store common paper dimensions from pages.csv
 */ 
class PaperSize
{
public:
    PaperSize();
    PaperSize(std::string name, double width, double height, Inkscape::Util::Unit const *unit);
    PaperSize(const PaperSize &other) { assign(other); } 
    PaperSize &operator=(const PaperSize &other) { assign(other); return *this; }

    ~PaperSize() = default;

    std::string name;
    Geom::Point size;
    double width;
    double height;
    Inkscape::Util::Unit const *unit; /// pointer to object in UnitTable, do not delete

    std::string getDescription(bool landscape) const;
    static std::string toDimsString(double x, double y, Util::Unit const *unit);

    static const std::vector<PaperSize>& getPageSizes();
    static const PaperSize *findPaperSize(double width, double height, Inkscape::Util::Unit const *unit);

    static std::string toDescription(std::string name, double x, double y, Inkscape::Util::Unit const *unit);
private:
    void assign(const PaperSize &other);
    static std::string formatNumber(double val);
};

} // namespace Inkscape

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
