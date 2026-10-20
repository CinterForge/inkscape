// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG data parser
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Raph Levien <raph@acm.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 1999 Raph Levien
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <glib.h>
#include <2geom/transforms.h>
#include "svg.h"
#include "util/numeric/precision.h"
#include "preferences.h"

std::string
sp_svg_transform_write(Geom::Affine const &transform)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // this must be a bit grater than EPSILON
    double e = 1e-5 * transform.descrim();
    int prec = Inkscape::Util::get_default_numeric_precision();
    int min_exp = prefs->getInt("/options/svgoutput/minimumexponent", -8);

    // Special case: when all fields of the affine are zero,
    // the optimized transformation is scale(0)
    if (transform[0] == 0 && transform[1] == 0 && transform[2] == 0 &&
        transform[3] == 0 && transform[4] == 0 && transform[5] == 0)
    {
        return "scale(0)";
    }


    std::stringstream c(""); // string buffer

    if (transform.isIdentity()) {
        // We are more or less identity, so no transform attribute needed:
        return {};
    } else if (transform.isScale()) {
        // We are more or less a uniform scale
        c << "scale(";
        c << sp_svg_number_write_de(transform[0], prec, min_exp);
        if (Geom::are_near(transform[0], transform[3], e)) {
            c << ")";
        } else {
            c << ",";
            c << sp_svg_number_write_de(transform[3], prec, min_exp);
            c << ")";
        }
    } else if (transform.isTranslation()) {
        // We are more or less a pure translation
        c  << "translate(";
        c << sp_svg_number_write_de(transform[4], prec, min_exp);
        if (Geom::are_near(transform[5], 0.0, e)) {
            c << ")";
        } else {
            c << ",";
            c << sp_svg_number_write_de(transform[5], prec, min_exp);
            c << ")";
        }
    } else if (transform.isRotation()) {
        // We are more or less a pure rotation
        c << "rotate(";
        double angle = std::atan2(transform[1], transform[0]) * (180 / M_PI);
        c << sp_svg_number_write_de(angle, prec, min_exp);
        c << ")";
    } else if (transform.withoutTranslation().isRotation()) {
        // Solution found by Johan Engelen
        // Refer to the matrix in svg-affine-test.h

        // We are a rotation about a special axis
        c << "rotate(";
        double angle = std::atan2(transform[1], transform[0]) * (180 / M_PI);
        c << sp_svg_number_write_de(angle, prec, min_exp);
        c << ",";

        Geom::Affine const& m = transform;
        double tx = (m[2]*m[5]+m[4]-m[4]*m[3]) / (1-m[3]-m[0]+m[0]*m[3]-m[2]*m[1]);

        c << sp_svg_number_write_de(tx, prec, min_exp);
        c << ",";

        double ty = (m[1]*tx + m[5]) / (1 - m[3]);
        c << sp_svg_number_write_de(ty, prec, min_exp);
        c << ")";
    } else if (transform.isHShear()) {
        // We are more or less a pure skewX
        c << "skewX(";
        double angle = atan(transform[2]) * (180 / M_PI);
        c << sp_svg_number_write_de(angle, prec, min_exp);
        c << ")";
    } else if (transform.isVShear()) {
        // We are more or less a pure skewY
        c << "skewY(";
        double angle = atan(transform[1]) * (180 / M_PI);

        c << sp_svg_number_write_de(angle, prec, min_exp);
        c << ")";
    } else {
        c << "matrix(";
        c << sp_svg_number_write_de(transform[0], prec, min_exp);
        c << ",";
        c << sp_svg_number_write_de(transform[1], prec, min_exp);
        c << ",";
        c << sp_svg_number_write_de(transform[2], prec, min_exp);
        c << ",";
        c << sp_svg_number_write_de(transform[3], prec, min_exp);
        c << ",";
        c << sp_svg_number_write_de(transform[4], prec, min_exp);
        c << ",";
        c << sp_svg_number_write_de(transform[5], prec, min_exp);
        c << ")";
    }

    assert(c.str().length() <= 256);
    return c.str();

}

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
