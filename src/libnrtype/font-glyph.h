// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Struct describing a single glyph in a font.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef LIBNRTYPE_FONT_GLYPH_H
#define LIBNRTYPE_FONT_GLYPH_H

#include <memory>
#include <2geom/pathvector.h>

// The info for a glyph in a font. It's totally resolution- and fontsize-independent.
struct FontGlyph
{
    double h_advance = 1.0;
    double v_advance = 1.0;
    Geom::PathVector pathvector;
    Geom::Rect bbox_exact;                        // Exact bounding box, from font.
    Geom::Rect bbox_pick = {0.0, -0.2, 1.0, 0.8}; // Expanded bounding box (initialize to em box). (y point down.)
    Geom::Rect bbox_draw = {0.0, -0.2, 1.0, 0.8}; // Expanded bounding box (initialize to em box).
};

#endif // LIBNRTYPE_FONT_GLYPH_H

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
