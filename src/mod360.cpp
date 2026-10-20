// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "mod360.h"

#include <cmath>
#include <2geom/angle.h>

/** Returns \a x wrapped around to between 0 and less than 360,
    or 0 if \a x isn't finite.
**/
double mod360(double const x)
{
    double m = fmod(x, 360.0);
    if (std::isnan(m)) {
        m = 0.0;
    } else if (m < 0) {
        m += 360.0;
    }
    if (m < 0.0 || m >= 360.0) {
        m = 0.0;
    }
    return m;
}

/** Returns \a x wrapped around to between -180 and less than 180,
    or 0 if \a x isn't finite.
**/
double mod360symm(double const x)
{
    double m = mod360(x);
    
    return m < 180.0 ? m : m - 360.0;   
}

double radians_to_degree_mod360(double rad) {
    return Geom::Angle::from_radians(rad).degrees();
}

double degree_to_radians_mod2pi(double degrees) {
    return Geom::Angle::from_degrees(degrees).radians();
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
