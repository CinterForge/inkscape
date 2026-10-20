// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_WIDGET_CANVAS_FRAGMENT_H
#define INKSCAPE_UI_WIDGET_CANVAS_FRAGMENT_H

#include <2geom/int-rect.h>
#include <2geom/affine.h>

namespace Inkscape::UI::Widget {

/// A "fragment" is a rectangle of drawn content at a specfic place.
struct Fragment
{
    // The matrix the geometry was transformed with when the content was drawn.
    Geom::Affine affine;

    // The rectangle of world space where the fragment was drawn.
    Geom::IntRect rect;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_FRAGMENT_H

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
