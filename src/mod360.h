// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_MOD360_H
#define SEEN_MOD360_H

// degree modulo 360; output from 0 to 360 inclusive
double mod360(double const x);
// degree module 360; output from -180 to 180
double mod360symm (double const x);

// radians -> degree modulo 360
double radians_to_degree_mod360(double rad);

// degrees -> radians modulo 2 Pi
double degree_to_radians_mod2pi(double degree);

#endif /* !SEEN_MOD360_H */

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
