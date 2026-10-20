// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_LIVAROT_SWEEP_EVENT_H
#define INKSCAPE_LIVAROT_SWEEP_EVENT_H
/** \file
 * Intersection events.
 */

#include <2geom/point.h>
class SweepTree;


/**
 * An intersection event structure to record any intersections that are
 * detected (predicted) during the sweepline.
 */
class SweepEvent
{
public:
    SweepTree *sweep[2];   /*!< Nodes associated with the left and right edge of the intersection. */

    Geom::Point posx;      /*!< Point of the intersection. */
    double tl;             /*!< Time value of the intersection on the left edge (tl).*/
    double tr;             /*!< Time value of the intersection on the right edge (tr). */

    int ind;               /*!< Index in the binary heap. */

    SweepEvent();   // not used.
    virtual ~SweepEvent();  // not used.

    /**
     * Initialize the sweep event.
     *
     * @param iLeft The left node of the intersection.
     * @param iRight The right node of the intersection.
     * @param iPt The intersection point.
     * @param itl The time value of the intersection on the left edge.
     * @param itr The time value of the intersection on the right edge.
     */
    void MakeNew (SweepTree * iLeft, SweepTree * iRight, Geom::Point const &iPt,
                  double itl, double itr);

    /// Void a SweepEvent structure.

    /**
     * Empty the sweep event data.
     *
     * Also reset event pointers of any SweepTree nodes that might point to this event.
     */
    void MakeDelete ();
};


#endif /* !INKSCAPE_LIVAROT_SWEEP_EVENT_H */

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
