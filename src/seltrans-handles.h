// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_SELTRANS_HANDLES_H
#define SEEN_SP_SELTRANS_HANDLES_H

/*
 * Seltrans knots
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include <glibmm/ustring.h>

#include "enums.h"  // SPAnchorType

typedef unsigned int guint32;

namespace Inkscape {
    class SelTrans;
}

enum SPSelTransType {
    HANDLE_STRETCH,
    HANDLE_SCALE,
    HANDLE_SKEW,
    HANDLE_ROTATE,
    HANDLE_CENTER,
    HANDLE_SIDE_ALIGN,
    HANDLE_CORNER_ALIGN,
    HANDLE_CENTER_ALIGN
};

// Offset for moving from Left click to Shift Click
const int ALIGN_SHIFT_OFFSET = 9;

// Offset for handle number (handles used by alignment don't start at zero, see seltrans-handles.cpp).
const int ALIGN_OFFSET = -13;

// Which handle does what in the alignment (clicking)
// clang-format off
const std::vector<Glib::ustring> AlignArguments = {
    // Left Click
    "selection top",
    "selection right",
    "selection bottom",
    "selection left",
    "selection vcenter",
    "selection top left",
    "selection top right",
    "selection bottom right",
    "selection bottom left",

    // Shift click
    "selection anchor bottom",
    "selection anchor left",
    "selection anchor top",
    "selection anchor right",
    "selection hcenter",
    "selection anchor bottom right",
    "selection anchor bottom left",
    "selection anchor top left",
    "selection anchor top right"
};
// clang-format on

struct SPSelTransHandle;

struct SPSelTransHandle {
    SPSelTransType type;
    SPAnchorType anchor;
    unsigned int control;
    gdouble x, y;
};
// These are 4 * each handle type + 1 for center
int const NUMHANDS = 26;
extern SPSelTransHandle const hands[NUMHANDS];

#endif // SEEN_SP_SELTRANS_HANDLES_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
