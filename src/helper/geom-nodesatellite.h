// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * NodeSatellite -- a per node holder of data.
 *//*
 * Authors:
 * see git history
 * Jabier Arraiza Cenoz<jabier.arraiza@marker.es>
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SATELLITE_H
#define SEEN_SATELLITE_H

#include <map>
#include <boost/assign.hpp>
#include <2geom/sbasis-geometric.h>
#include "util/enums.h"

enum NodeSatelliteType
{
    FILLET = 0,       // Fillet
    INVERSE_FILLET,   // Inverse Fillet
    CHAMFER,          // Chamfer
    INVERSE_CHAMFER,  // Inverse Chamfer
    INVALID_SATELLITE // Invalid NodeSatellite
};
/**
 * @brief NodeSatellite a per node holder of data.
 */

class NodeSatellite
{
public:
    NodeSatellite();
    NodeSatellite(NodeSatelliteType nodesatellite_type);

    virtual ~NodeSatellite();
    void setIsTime(bool set_is_time)
    {
        is_time = set_is_time;
    }
    void setSelected(bool set_selected)
    {
        selected = set_selected;
    }
    void setHasMirror(bool set_has_mirror)
    {
        has_mirror = set_has_mirror;
    }
    void setHidden(bool set_hidden)
    {
        hidden = set_hidden;
    }
    void setAmount(double set_amount)
    {
        amount = set_amount;
    }
    void setAngle(double set_angle)
    {
        angle = set_angle;
    }
    void setSteps(size_t set_steps)
    {
        steps = set_steps;
    }
    double lenToRad(double const A, Geom::Curve const &curve_in, Geom::Curve const &curve_out,
                    NodeSatellite const previousNodeSatellite) const;
    double radToLen(double const A, Geom::Curve const &curve_in,
                    Geom::Curve const &curve_out) const;

    double time(Geom::Curve const &curve_in, bool inverse = false) const;
    double time(double A, bool inverse, Geom::Curve const &curve_in) const;
    double arcDistance(Geom::Curve const &curve_in) const;

    void setPosition(Geom::Point const p, Geom::Curve const &curve_in, bool inverse = false);
    Geom::Point getPosition(Geom::Curve const &curve_in, bool inverse = false) const;

    void setNodeSatellitesType(gchar const *A);
    gchar const *getNodeSatellitesTypeGchar() const;
    NodeSatelliteType nodesatellite_type;
    // The value stored could be a time value of the nodesatellite in the curve or a length of distance to the node from
    // the nodesatellite "is_time" tells us if it's a time or length value
    bool is_time;
    bool selected;
    bool has_mirror;
    bool hidden;
    // in "amount" we store the time or distance used in the nodesatellite
    double amount;
    double angle;
    size_t steps;
};

double timeAtArcLength(double const A, Geom::Curve const &curve_in);
double arcLengthAt(double const A, Geom::Curve const &curve_in);

#endif // SEEN_SATELLITE_H

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
