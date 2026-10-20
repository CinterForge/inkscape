// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * PathVectorNodeSatellites a class to manage nodesatellites -per node extra data- in a pathvector
 *//*
 * Authors: see git history
 * Jabiertxof
 * Nathan Hurst
 * Johan Engelen
 * Josh Andler
 * suv
 * Mc-
 * Liam P. White
 * Krzysztof Kosiński
 * This code is in public domain
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <helper/geom-pathvector_nodesatellites.h>
#include <helper/geom.h>

#include "util/units.h"

Geom::PathVector PathVectorNodeSatellites::getPathVector() const
{
    return _pathvector;
}

void PathVectorNodeSatellites::setPathVector(Geom::PathVector pathv)
{
    _pathvector = pathv;
}

NodeSatellites PathVectorNodeSatellites::getNodeSatellites()
{
    return _nodesatellites;
}

void PathVectorNodeSatellites::setNodeSatellites(NodeSatellites nodesatellites)
{
    _nodesatellites = nodesatellites;
}

size_t PathVectorNodeSatellites::getTotalNodeSatellites()
{
    size_t counter = 0;
    for (auto &_nodesatellite : _nodesatellites) {
        counter += _nodesatellite.size();
    }
    return counter;
}

std::pair<size_t, size_t> PathVectorNodeSatellites::getIndexData(size_t index)
{
    size_t counter = 0;
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            if (index == counter) {
                return std::make_pair(i,j);
            }
            counter++;
        }
    }
    return std::make_pair(0,0);
}

void PathVectorNodeSatellites::setSelected(std::vector<size_t> selected)
{
    size_t counter = 0;
    for (auto &_nodesatellite : _nodesatellites) {
        for (auto &j : _nodesatellite) {
            if (find (selected.begin(), selected.end(), counter) != selected.end()) {
                j.setSelected(true);
            } else {
                j.setSelected(false);
            }
            counter++;
        }
    }
}

void PathVectorNodeSatellites::updateSteps(size_t steps, bool apply_no_radius, bool apply_with_radius,
                                           bool only_selected)
{
    for (auto &_nodesatellite : _nodesatellites) {
        for (auto &j : _nodesatellite) {
            if ((!apply_no_radius && j.amount == 0) ||
                (!apply_with_radius && j.amount != 0)) 
            {
                continue;
            }
            if (only_selected) {
                if (j.selected) {
                    j.steps = steps;
                }
            } else {
                j.steps = steps;
            }
        }
    }
}

void PathVectorNodeSatellites::updateAngle(double angle, bool apply_no_radius, bool apply_with_radius,
                                           bool only_selected)
{
    for (auto &_nodesatellite : _nodesatellites) {
        for (auto &j : _nodesatellite) {
            if ((!apply_no_radius && j.amount == 0) ||
                (!apply_with_radius && j.amount != 0))
            {
                continue;
            }
            if (only_selected) {
                if (j.selected) {
                    j.angle = angle;
                }
            } else {
                j.angle = angle;
            }
        }
    }
}

void PathVectorNodeSatellites::updateAmount(double radius, bool apply_no_radius, bool apply_with_radius,
                                            bool only_selected, bool use_knot_distance, bool flexible)
{
    double power = 0;
    if (!flexible) {
        power = radius;
    } else {
        power = radius / 100;
    }
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            std::optional<size_t> previous_index = std::nullopt;
            if (j == 0 && _pathvector[i].closed()) {
                previous_index = count_path_nodes(_pathvector[i]) - 1;
            } else if (!_pathvector[i].closed() || j != 0) {
                previous_index = j - 1;
            }
            if (!_pathvector[i].closed() && j == 0) {
                _nodesatellites[i][j].amount = 0;
                continue;
            }
            if (count_path_nodes(_pathvector[i]) == j) {
                continue;
            }
            if ((!apply_no_radius && _nodesatellites[i][j].amount == 0) ||
                (!apply_with_radius && _nodesatellites[i][j].amount != 0)) {
                continue;
            }

            if (_nodesatellites[i][j].selected || !only_selected) {
                if (!use_knot_distance && !flexible) {
                    if (previous_index) {
                        _nodesatellites[i][j].amount =
                            _nodesatellites[i][j].radToLen(power, _pathvector[i][*previous_index], _pathvector[i][j]);
                        if (power && !_nodesatellites[i][j].amount) {
                            g_warning("Seems a too high radius value");
                        }
                    } else {
                        _nodesatellites[i][j].amount = 0.0;
                    }
                } else {
                    _nodesatellites[i][j].amount = power;
                }
            }
        }
    }
}

void PathVectorNodeSatellites::convertUnit(Glib::ustring in, Glib::ustring to, bool apply_no_radius,
                                           bool apply_with_radius)
{
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            if (!_pathvector[i].closed() && j == 0) {
                _nodesatellites[i][j].amount = 0;
                continue;
            }
            if (count_path_nodes(_pathvector[i]) == j) {
                continue;
            }
            if ((!apply_no_radius && _nodesatellites[i][j].amount == 0) ||
                (!apply_with_radius && _nodesatellites[i][j].amount != 0)) {
                continue;
            }
            _nodesatellites[i][j].amount =
                Inkscape::Util::Quantity::convert(_nodesatellites[i][j].amount, in.c_str(), to.c_str());
        }
    }
}

void PathVectorNodeSatellites::updateNodeSatelliteType(NodeSatelliteType nodesatellitetype, bool apply_no_radius,
                                                       bool apply_with_radius, bool only_selected)
{
    for (size_t i = 0; i < _nodesatellites.size(); ++i) {
        for (size_t j = 0; j < _nodesatellites[i].size(); ++j) {
            if ((!apply_no_radius && _nodesatellites[i][j].amount == 0) ||
                (!apply_with_radius && _nodesatellites[i][j].amount != 0)) {
                continue;
            }
            if (count_path_nodes(_pathvector[i]) == j) {
                if (!only_selected) {
                    _nodesatellites[i][j].nodesatellite_type = nodesatellitetype;
                }
                continue;
            }
            if (only_selected) {
                if (_nodesatellites[i][j].selected) {
                    _nodesatellites[i][j].nodesatellite_type = nodesatellitetype;
                }
            } else {
                _nodesatellites[i][j].nodesatellite_type = nodesatellitetype;
            }
        }
    }
}

/*
 * Copy NodeSatellite data from the old path to the new path by matching nodes in the new path to
 * nodes in the old path geometrically. If more than one node matches, take the first match.
 *
 * Empty sub-paths ("M 0,0" or "M 0,0 z") have no curves or nodes and have zero length
 * NodeSatellite vectors.
 *
 * Closed sub-paths have the same number of nodes as curves, but if the closing path is almost
 * degenerate (less then Geom::Epsilon in length), it is removed. This follows Inkscape's behavior
 * when editting paths.
 *
 * Open sub-paths have one more node that number of curves; the last (as well as the first node) is
 * not used but we need to include it in NodeSatellite data for backwards compatibility.
 *
 * Inputs: new path (new_pathv), default NodeSatellite (S).
 */
void PathVectorNodeSatellites::recalculateForNewPathVector(Geom::PathVector const new_pathvector, NodeSatellite const S)
{
    NodeSatellites new_nodesatellites;

    // Loop over new paths
    size_t new_paths_size = new_pathvector.size();
    for (size_t i_np = 0; i_np < new_paths_size; ++i_np) {  // OLD i

        std::vector<NodeSatellite> new_nodesatellite_vector;

        // Find number of curves.
        size_t new_curves_size = count_path_curves(new_pathvector[i_np]);

        // Loop over curves nodes
        for (size_t i_nc = 0; i_nc < new_curves_size; ++i_nc) { // OLD j

            // Search for old NodeSatellite match.
            bool found = false;

            // Loop over old paths (there may not be any old paths, e.g. for stars!).
            for (size_t i_op = 0; i_op < _pathvector.size(); ++i_op) { // OLD k

                // Check we have data corresponding to path:
                if (i_op >= _nodesatellites.size()) {
                    // No use continuing, no data!
                    break;
                }

                // Loop over old curves
                size_t old_curves_size = count_path_curves(_pathvector[i_op]);
                for (size_t i_oc = 0; i_oc < old_curves_size; ++i_oc) { // OLD l

                    // Check we have data corresponding to node
                    if (i_oc >= _nodesatellites[i_op].size()) {
                        // No use continuing, no data!
                        break;
                    }

                    if (Geom::are_near(_pathvector[i_op][i_oc].initialPoint(), new_pathvector[i_np][i_nc].initialPoint(), 0.001)) { // epsilon is not big enough.
                        new_nodesatellite_vector.push_back(_nodesatellites[i_op][i_oc]);
                        found = true;
                        break;
                    }
                } // Loop over old curves.

                if (found) {
                    break;
                }
            } // Loop over old paths.

            if (!found) {
                bool const push_satellite = _pathvector.empty() &&
                                            i_np < _nodesatellites.size() &&
                                            i_nc < _nodesatellites[i_np].size();
                new_nodesatellite_vector.push_back(push_satellite ? _nodesatellites[i_np][i_nc] : S);
            }

        } // Loop over new curves.

        // Add entry for final node of non-empty open paths, this is not used but matches previous behavior.
        if (!new_pathvector[i_np].empty() &&
            !new_pathvector[i_np].closed()) {
            new_nodesatellite_vector.push_back(S);
        }

        new_nodesatellites.push_back(new_nodesatellite_vector);

    } // Loop over new paths.

    setPathVector(new_pathvector);
    setNodeSatellites(new_nodesatellites);
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
