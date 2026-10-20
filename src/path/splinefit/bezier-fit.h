// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include "2geom/point.h"

struct InputPoint : Geom::Point {
    InputPoint() {}
    InputPoint(const Geom::Point& pt) : Point(pt) {}
    InputPoint(const Geom::Point& pt, double t) : Point(pt), t(t) {}
    InputPoint(const Geom::Point& pt, const Geom::Point& front, const Geom::Point& back, double t)
     : Point(pt), front(front), back(back), t(t), have_slope(true) {}

    Geom::Point front;
    Geom::Point back;
    double t = 0;
    bool have_slope = false;
};

// Fit cubic Bezier to input points; use slope of the first and last points to find a fit
int bezier_fit(Geom::Point bezier[4], const std::vector<InputPoint>& data);
