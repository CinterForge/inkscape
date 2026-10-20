// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * The nodes at the ends of the path in the pen/pencil tools.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_DRAW_ANCHOR_H
#define SEEN_DRAW_ANCHOR_H

/** \file 
 * Drawing anchors. 
 */

#include <2geom/point.h>

#include "display/control/canvas-item-ptr.h"

namespace Inkscape {

class CanvasItemCtrl;

namespace UI {
namespace Tools {

class FreehandBase;

}
}
}

/// The drawing anchor.
/// \todo Make this a regular knot, this will allow setting statusbar tips.

// TODO Get rid of this class.

class SPDrawAnchor
{
public:
    Inkscape::UI::Tools::FreehandBase *dc;
    std::shared_ptr<Geom::PathVector> curve;
    bool start : 1;
    bool active : 1;
    Geom::Point dp;
    CanvasItemPtr<Inkscape::CanvasItemCtrl> ctrl;

    SPDrawAnchor(Inkscape::UI::Tools::FreehandBase *dc,
                 std::shared_ptr<Geom::PathVector> curve, bool start, Geom::Point delta);

    ~SPDrawAnchor();

    SPDrawAnchor *anchorTest(Geom::Point w, bool activate);
};

#endif /* !SEEN_DRAW_ANCHOR_H */

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
