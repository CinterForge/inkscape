// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Jabiertxof:Thanks to all people help me
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_FILLET_CHAMFER_H
#define INKSCAPE_LPE_FILLET_CHAMFER_H

#include "helper/geom-nodesatellite.h"
#include "helper/geom-pathvector_nodesatellites.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/hidden.h"
#include "live_effects/parameter/nodesatellitesarray.h"
#include "live_effects/parameter/unit.h"

namespace Inkscape {
namespace LivePathEffect {

enum Filletmethod {
    FM_AUTO,
    FM_ARC,
    FM_BEZIER,
    FM_END
};

class LPEFilletChamfer : public Effect {
public:
    LPEFilletChamfer(LivePathEffectObject *lpeobject);
    void doBeforeEffect(SPLPEItem const *lpeItem) override;
    Geom::PathVector doEffect_path(Geom::PathVector const &path_in) override;
    void doOnApply(SPLPEItem const *lpeItem) override;
    Gtk::Widget *newWidget() override;
    void adjustForNewPath() override;
    Geom::Ray getRay(Geom::Point start, Geom::Point end, Geom::Curve *curve, bool reverse);
    void addChamferSteps(Geom::Path &tmp_path, Geom::Path path_chamfer, Geom::Point end_arc_point, size_t steps, double scale);
    void addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec) override;
    void updateNodeSatelliteType(NodeSatelliteType nodesatellitetype);
    void setSelected(PathVectorNodeSatellites *_pathvector_nodesatellites);
    //void convertUnit();
    void updateChamferSteps();
    void updateChamferScale();
    void updateAmount();
    bool helperpath;
    NodeSatelliteArrayParam nodesatellites_param;

private:
    UnitParam unit;
    EnumParam<Filletmethod> method;
    ScalarParam radius;
    ScalarParam chamfer_steps;
    ScalarParam chamfer_scale;
    BoolParam flexible;
    HiddenParam mode;
    BoolParam only_selected;
    BoolParam use_knot_distance;
    BoolParam hide_knots;
    BoolParam apply_no_radius;
    BoolParam apply_with_radius;
    PathVectorNodeSatellites *_pathvector_nodesatellites;
    Geom::PathVector _hp;
    Glib::ustring previous_unit;
    LPEFilletChamfer(const LPEFilletChamfer &);
    LPEFilletChamfer &operator=(const LPEFilletChamfer &);

};

} //namespace LivePathEffect
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
