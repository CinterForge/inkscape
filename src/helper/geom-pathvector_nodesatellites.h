// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * \brief PathVectorNodeSatellites a class to manage nodesatellites -per node extra data- in a pathvector
 *//*
 * Authors: see git history
 * Jabiertxof
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_PATHVECTORSATELLITES_H
#define SEEN_PATHVECTORSATELLITES_H

#include <2geom/path.h>
#include <2geom/pathvector.h>
#include <helper/geom-nodesatellite.h>

// First index is path, second is node in path.
typedef std::vector<std::vector<NodeSatellite>> NodeSatellites;

///@brief PathVectorNodeSatellites a class to manage nodesatellites in a pathvector. Used by fillet/chamfer LPE.
class PathVectorNodeSatellites
{
public:
    Geom::PathVector getPathVector() const;
    void setPathVector(Geom::PathVector pathv);
    NodeSatellites getNodeSatellites();
    void setNodeSatellites(NodeSatellites nodesatellites);
    size_t getTotalNodeSatellites();
    void setSelected(std::vector<size_t> selected);
    void updateSteps(size_t steps, bool apply_no_radius, bool apply_with_radius, bool only_selected);
    void updateAngle(double angle, bool apply_no_radius, bool apply_with_radius, bool only_selected);
    void updateAmount(double radius, bool apply_no_radius, bool apply_with_radius, bool only_selected, 
                      bool use_knot_distance, bool flexible);
    void convertUnit(Glib::ustring in, Glib::ustring to, bool apply_no_radius, bool apply_with_radius);
    void updateNodeSatelliteType(NodeSatelliteType nodesatellitetype, bool apply_no_radius, bool apply_with_radius,
                                 bool only_selected);
    std::pair<size_t, size_t> getIndexData(size_t index);
    void recalculateForNewPathVector(Geom::PathVector const pathv, NodeSatellite const S);

private:
    Geom::PathVector _pathvector;
    NodeSatellites _nodesatellites;
};

#endif //SEEN_PATHVECTORSATELLITES_H
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
