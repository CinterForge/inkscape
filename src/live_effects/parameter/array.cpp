// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/array.h"

#include <utility>
#include <2geom/coord.h>
#include <2geom/point.h>

#include "live_effects/effect.h"
#include "live_effects/lpeobject.h"
#include "svg/svg.h"

namespace Inkscape::LivePathEffect {

template <>
double
ArrayParam<double>::readsvg(char const * const str)
{
    double newx = Geom::infinity();
    sp_svg_number_read_d(str, &newx);
    return newx;
}

template <>
float
ArrayParam<float>::readsvg(char const * const str)
{
    float newx = Geom::infinity();
    sp_svg_number_read_f(str, &newx);
    return newx;
}

template <>
Glib::ustring
ArrayParam<Glib::ustring>::readsvg(char const * const str)
{
    if (str) {
        return Glib::ustring(str);
    }
    return Glib::ustring("");
}

template <>
Geom::Point
ArrayParam<Geom::Point>::readsvg(char const * const str)
{
    auto const strarray = g_strsplit(str, ",", 2);
    double newx, newy;
    unsigned int success = sp_svg_number_read_d(strarray[0], &newx);
    success += sp_svg_number_read_d(strarray[1], &newy);
    g_strfreev (strarray);
    if (success == 2) {
        return Geom::Point(newx, newy);
    }
    return Geom::Point(Geom::infinity(),Geom::infinity());
}

template <>
std::shared_ptr<SatelliteReference>
ArrayParam<std::shared_ptr<SatelliteReference>>::readsvg(char const * const str)
{
    std::shared_ptr<SatelliteReference> satellitereference = nullptr;
    if (!str) {
        return satellitereference;
    }

    auto const strarray = g_strsplit(str, ",", 2);
    if (strarray[0] != nullptr && g_strstrip(strarray[0])[0] == '#') {
        try {
            bool active = strarray[1] != nullptr;
            satellitereference = std::make_shared<SatelliteReference>(param_effect->getLPEObj(), active);
            satellitereference->attach(Inkscape::URI(g_strstrip(strarray[0])));
            if (active) {
                satellitereference->setActive(strncmp(strarray[1], "1", 1) == 0);
            }
        } catch (Inkscape::BadURIException &e) {
            g_warning("%s (%s)", e.what(), strarray[0]);
            satellitereference->detach();
        }
    }
    g_strfreev(strarray);
    return satellitereference;
}

template <>
std::vector<NodeSatellite> ArrayParam<std::vector<NodeSatellite>>::readsvg(char const * const str)
{
    std::vector<NodeSatellite> subpath_nodesatellites;
    if (!str) {
        return subpath_nodesatellites;
    }
    auto const strarray = g_strsplit(str, "@", 0);
    auto iter = strarray;
    while (*iter != nullptr) {
        auto const strsubarray = g_strsplit(*iter, ",", 8);
        if (*strsubarray[7]) {//steps always > 0
            NodeSatellite nodesatellite{};
            nodesatellite.setNodeSatellitesType(g_strstrip(strsubarray[0]));
            nodesatellite.is_time = strncmp(strsubarray[1], "1", 1) == 0;
            nodesatellite.selected = strncmp(strsubarray[2], "1", 1) == 0;
            nodesatellite.has_mirror = strncmp(strsubarray[3], "1", 1) == 0;
            nodesatellite.hidden = strncmp(strsubarray[4], "1", 1) == 0;
            double amount,angle;
            float stepsTmp;
            sp_svg_number_read_d(strsubarray[5], &amount);
            sp_svg_number_read_d(strsubarray[6], &angle);
            sp_svg_number_read_f(g_strstrip(strsubarray[7]), &stepsTmp);
            unsigned int steps = (unsigned int)stepsTmp;
            nodesatellite.amount = amount;
            nodesatellite.angle = angle;
            nodesatellite.steps = steps;
            subpath_nodesatellites.push_back(std::move(nodesatellite));
        }
        g_strfreev (strsubarray);
        iter++;
    }
    g_strfreev (strarray);
    return subpath_nodesatellites;
}

} // namespace Inkscape::LivePathEffect

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
