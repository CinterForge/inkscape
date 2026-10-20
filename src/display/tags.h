// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DRAWINGITEM_TAGS_H
#define INKSCAPE_DRAWINGITEM_TAGS_H

#include "util/cast.h"

// Class hierarchy structure

#define DRAWINGITEM_HIERARCHY_DATA(X)\
X(DrawingItem,\
    X(DrawingShape)\
    X(DrawingImage)\
    X(DrawingGroup,\
        X(DrawingPattern)\
        X(DrawingText)\
    )\
    X(DrawingGlyphs)\
)

namespace Inkscape {

// Forward declarations

#define X(n, ...) class n; __VA_ARGS__
DRAWINGITEM_HIERARCHY_DATA(X)
#undef X

// Tag generation

enum class DrawingItemTag : int
{
    #define X(n, ...) n##_first, __VA_ARGS__ n##_tmp, n##_last = n##_tmp - 1,
    DRAWINGITEM_HIERARCHY_DATA(X)
    #undef X
};

} // namespace Inkscape

// Tag specialization

#define X(n, ...) template <> inline constexpr int first_tag<Inkscape::n> = static_cast<int>(Inkscape::DrawingItemTag::n##_first); __VA_ARGS__
DRAWINGITEM_HIERARCHY_DATA(X)
#undef X

#define X(n, ...) template <> inline constexpr int last_tag<Inkscape::n> = static_cast<int>(Inkscape::DrawingItemTag::n##_last); __VA_ARGS__
DRAWINGITEM_HIERARCHY_DATA(X)
#undef X

#undef DRAWINGITEM_HIERARCHY_DATA

#endif // INKSCAPE_DRAWINGITEM_TAGS_H

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
