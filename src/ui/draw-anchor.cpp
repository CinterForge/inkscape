// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Anchors implementation.
 */

/*
 * Authors:
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "ui/draw-anchor.h"

#include "desktop.h"
#include "display/control/canvas-item-ctrl.h"
#include "tools/freehand-base.h"

/**
 * Creates an anchor object and initializes it.
 */
SPDrawAnchor::SPDrawAnchor(Inkscape::UI::Tools::FreehandBase *dc, std::shared_ptr<Geom::PathVector> curve, bool start, Geom::Point delta)
    : dc(dc), curve(std::move(curve)), start(start), active(FALSE), dp(delta),
      ctrl(
        make_canvasitem<Inkscape::CanvasItemCtrl>(
          dc->getDesktop()->getCanvasControls(),
          Inkscape::CANVAS_ITEM_CTRL_TYPE_ANCHOR
        )
      )
{
    ctrl->set_name("CanvasItemCtrl:DrawAnchor");
    ctrl->set_position(delta);
    ctrl->set_pickable(false); // We do our own checking. (TODO: Should be fixed!)
}

SPDrawAnchor::~SPDrawAnchor()
{
    // Clear references in FreehandBase.
    if (dc->sa == this) {
        dc->sa = nullptr;
    }
    if (dc->ea == this) {
        dc->ea = nullptr;
    }
}

/**
 * Test if point is near anchor, if so fill anchor on canvas and return
 * pointer to it or NULL.
 */
SPDrawAnchor *SPDrawAnchor::anchorTest(Geom::Point w, bool activate)
{
    if (activate && ctrl->contains(w)) {
        if (!active) {
            ctrl->set_hover();
            ctrl->set_size(Inkscape::HandleSize::LARGE);
            active = TRUE;
        }
        return this;
    }

    if (active) {
        ctrl->set_normal();
        ctrl->set_size(Inkscape::HandleSize::NORMAL);
        active = FALSE;
    }

    return nullptr;
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
