// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_BUFFER_H
#define SEEN_CANVAS_ITEM_BUFFER_H

/**
 * Buffer for rendering canvas items.
 */

/*
 * Author:
 *   See git history.
 *
 * Copyright (C) 2020 Authors
 *
 * Rewrite of SPCanvasBuf.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 */

#include <2geom/rect.h>
#include <cairomm/context.h>

namespace Inkscape {

/**
 * Class used when rendering canvas items.
 */
struct CanvasItemBuffer
{
    Geom::IntRect rect;
    int device_scale; // For high DPI monitors.
    Cairo::RefPtr<Cairo::Context> cr;
    bool outline_pass;
};

} // namespace Inkscape

#endif // SEEN_CANVAS_ITEM_BUFFER_H

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
