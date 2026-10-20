// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_BBOXSORT_H
#define SEEN_BBOXSORT_H

/** @file
 * @brief Simple helper class for sorting objects based on their bounding boxes.
 */
/* Authors:
 *   MenTaLguY
 *   Dmitry Kirsanov
 *   Krzysztof Kosiński
 *
 * Copyright (C) 2007-2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/*
 * Previously part of the Align and Distribute dialog.
 */

class BBoxSort {

public:
    BBoxSort(SPItem *item, Geom::Rect const &bounds, Geom::Dim2 orientation, double begin, double end)
        : item(item)
        , bbox(bounds)
    {
        anchor = begin * bbox.min()[orientation] + end * bbox.max()[orientation];
    }

    BBoxSort(const BBoxSort &rhs) = default; // Should really be vector of pointers to avoid copying class when sorting.
    ~BBoxSort() = default;

    double anchor = 0.0;
    SPItem* item = nullptr;
    Geom::Rect bbox;
};

static bool operator< (const BBoxSort &a, const BBoxSort &b) {
    return a.anchor < b.anchor;
}

#endif // SEEN_BBOXSORT_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
