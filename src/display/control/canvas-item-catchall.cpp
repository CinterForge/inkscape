// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A class to catch events after everyone else has had a go.
 */

/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of SPCanvasAcetate.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-catchall.h"

namespace Inkscape {

/**
 * Create an null control catchall.
 */
CanvasItemCatchall::CanvasItemCatchall(CanvasItemGroup *group)
    : CanvasItem(group)
{
    _name = "CanvasItemCatchall";
    _pickable = true; // Duh! That's the purpose of this class!
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of catchall.
 */
bool CanvasItemCatchall::contains(Geom::Point const &p, double tolerance)
{
    return true; // We contain every place!
}

/**
 * Update and redraw control catchall.
 */
void CanvasItemCatchall::_update(bool)
{
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());
}

/**
 * Render catchall to screen via Cairo.
 */
void CanvasItemCatchall::_render(Inkscape::CanvasItemBuffer &buf) const
{
    // Do nothing! (Needed as CanvasItem is abstract.)
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
