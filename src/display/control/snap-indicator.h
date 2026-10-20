// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_DISPLAY_SNAP_INDICATOR_H
#define INKSCAPE_DISPLAY_SNAP_INDICATOR_H

/**
 * @file
 * Provides a class that shows a temporary indicator on the canvas of where the snap was, and what kind of snap
 */
/*
 * Authors:
 *   Johan Engelen
 *   Diederik van Lierop
 *
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Diederik van Lierop 2010 <mail@diedenrezi.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "snap-enums.h"
#include "snapped-point.h"
#include "display/control/canvas-item-curve.h"

#include <glib.h>
#include <glibmm/i18n.h>

class SPDesktop;

namespace Inkscape {
namespace Display {

class TemporaryItem;

class SnapIndicator  {
public:
    SnapIndicator(SPDesktop *desktop);
    virtual ~SnapIndicator();

    void set_new_snaptarget(Inkscape::SnappedPoint const &p, bool pre_snap = false);
    void remove_snaptarget(bool only_if_presnap = false);

    void set_new_snapsource(Inkscape::SnapCandidatePoint const &p);
    void remove_snapsource();

    void set_new_debugging_point(Geom::Point const &p);
    void remove_debugging_points();

protected:
    TemporaryItem *_snaptarget;
    TemporaryItem *_snaptarget_tooltip;
    TemporaryItem *_snaptarget_bbox;
    TemporaryItem *_snapsource;

    std::list<TemporaryItem *> _alignment_snap_indicators;
    std::list<TemporaryItem *> _distribution_snap_indicators;
    std::list<TemporaryItem *> _debugging_points;
    bool _snaptarget_is_presnap;
    SPDesktop *_desktop;

private:
    SnapIndicator(const SnapIndicator&) = delete;
    SnapIndicator& operator=(const SnapIndicator&) = delete;

    void make_distribution_indicators(SnappedPoint const &p, double fontsize, double scale);
    void make_alignment_indicator(Geom::Point const &p1, Geom::Point const &p2, guint32 color, double fontsize, double scale);
    guint32 get_guide_color(SnapTargetType t);
    Inkscape::CanvasItemCurve* make_stub_line_h(Geom::Point const &p);
    Inkscape::CanvasItemCurve* make_stub_line_v(Geom::Point const &p);
};

} //namespace Display
} //namespace Inkscape

#endif

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
