// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-attach-path.h"

#include <cmath>

#include <2geom/path-sink.h>

#include "object/sp-lpe-item.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEAttachPath::LPEAttachPath(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    start_path(_("Start path:"), _("Path to attach to the start of this path"), "startpath", &wr, this),
    start_path_position(_("Start path position:"), _("Position to attach path start to"), "startposition", &wr, this, 0.0),
    start_path_curve_start(_("Start path curve start:"), _("Starting curve"), "startcurvestart", &wr, this, Geom::Point(20,0)/*, true*/),
    start_path_curve_end(_("Start path curve end:"), _("Ending curve"), "startcurveend", &wr, this, Geom::Point(20,0)/*, true*/),
    end_path(_("End path:"), _("Path to attach to the end of this path"), "endpath", &wr, this),
    end_path_position(_("End path position:"), _("Position to attach path end to"), "endposition", &wr, this, 0.0),
    end_path_curve_start(_("End path curve start:"), _("Starting curve"), "endcurvestart", &wr, this, Geom::Point(20,0)/*, true*/),
    end_path_curve_end(_("End path curve end:"), _("Ending curve"), "endcurveend", &wr, this, Geom::Point(20,0)/*, true*/)
{
    registerParameter(&start_path);
    registerParameter(&start_path_position);
    registerParameter(&start_path_curve_start);
    registerParameter(&start_path_curve_end);
    
    registerParameter(&end_path);
    registerParameter(&end_path_position);
    registerParameter(&end_path_curve_start);
    registerParameter(&end_path_curve_end);

    //perceived_path = true;
    show_orig_path = true;
    curve_start_previous_origin = start_path_curve_end.getOrigin();
    curve_end_previous_origin = end_path_curve_end.getOrigin();
    start_path.setUpdating(true);
    end_path.setUpdating(true);
}

LPEAttachPath::~LPEAttachPath() = default;

void LPEAttachPath::resetDefaults(SPItem const * /*item*/)
{
    Effect::resetDefaults(nullptr);
    curve_start_previous_origin = start_path_curve_end.getOrigin();
    curve_end_previous_origin = end_path_curve_end.getOrigin();
}

void 
LPEAttachPath::doBeforeEffect (SPLPEItem const* lpeitem)
{
    if (is_load) {
        start_path.setUpdating(false);
        start_path.start_listening(start_path.getObject());
        start_path.connect_selection_changed();
        end_path.setUpdating(false);
        end_path.start_listening(end_path.getObject());
        end_path.connect_selection_changed();
        if (auto item = end_path.getObject()) {
            item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        if (auto item = start_path.getObject()) {
            item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
}


bool 
LPEAttachPath::doOnOpen(SPLPEItem const *lpeitem)
{
    if (!is_load || is_applied) {
        return false;
    }
    start_path.setUpdating(false);
    start_path.start_listening(start_path.getObject());
    start_path.connect_selection_changed();
    end_path.setUpdating(false);
    end_path.start_listening(end_path.getObject());
    end_path.connect_selection_changed();
    return false;
}

void LPEAttachPath::doEffect(Geom::PathVector &curve)
{
    if (sp_lpe_item && !curve.empty()) {
        auto p = Geom::Path(curve.initialPoint());
        
        bool set_start_end = start_path_curve_end.getOrigin() != curve_start_previous_origin;
        bool set_end_end = end_path_curve_end.getOrigin() != curve_end_previous_origin;
        
        if (start_path.linksToPath() && start_path.getObject()) {

            Geom::PathVector linked_pathv = start_path.get_pathvector();
            Geom::Affine linkedtransform = start_path.getObject()->getRelativeTransform(sp_lpe_item);

            if ( !linked_pathv.empty() )
            {
                Geom::Path transformedpath = linked_pathv.front() * linkedtransform;
                start_path_curve_start.setOrigin(curve.initialPoint());

                std::vector<Geom::Point> derivs = curve.front().front().pointAndDerivatives(0, 3);
                
                for (unsigned deriv_n = 1; deriv_n < derivs.size(); deriv_n++) {
                    Geom::Coord length = derivs[deriv_n].length();
                    if ( ! Geom::are_near(length, 0) && !start_path.getObject()->_tmpsuccessor) {
                        if (set_start_end) {
                            start_path_position.param_set_value(transformedpath.nearestTime(start_path_curve_end.getOrigin()).asFlatTime());
                        }
                        if (start_path_position > transformedpath.size()) {
                            start_path_position.param_set_value(transformedpath.size());
                        } else if (start_path_position < 0) {
                            start_path_position.param_set_value(0);
                        }
                        Geom::Curve const *c = start_path_position >= transformedpath.size() ?
							&transformedpath.back() : &transformedpath.at((int)start_path_position);

                        std::vector<Geom::Point> derivs_2 = c->pointAndDerivatives(start_path_position >= transformedpath.size() ? 1 : (start_path_position - (int)start_path_position), 3);
                        for (unsigned deriv_n_2 = 1; deriv_n_2 < derivs_2.size(); deriv_n_2++) {
                            Geom::Coord length_2 = derivs[deriv_n_2].length();
                            if ( ! Geom::are_near(length_2, 0) ) {
                                start_path_curve_end.setOrigin(derivs_2[0]);
                                curve_start_previous_origin = start_path_curve_end.getOrigin();

                                double startangle = atan2(start_path_curve_start.getVector().y(), start_path_curve_start.getVector().x());
                                double endangle = atan2(start_path_curve_end.getVector().y(), start_path_curve_end.getVector().x());
                                double startderiv = atan2(derivs[deriv_n].y(), derivs[deriv_n].x());
                                double endderiv = atan2(derivs_2[deriv_n_2].y(), derivs_2[deriv_n_2].x());
                                Geom::Point pt1 = Geom::Point(start_path_curve_start.getVector().length() * cos(startangle + startderiv), start_path_curve_start.getVector().length() * sin(startangle + startderiv));
                                Geom::Point pt2 = Geom::Point(start_path_curve_end.getVector().length() * cos(endangle + endderiv), start_path_curve_end.getVector().length() * sin(endangle + endderiv));
                                p = Geom::Path(derivs_2[0]);
                                p.appendNew<Geom::CubicBezier>(-pt2 + derivs_2[0], -pt1 + curve.initialPoint(), curve.initialPoint());
                                break;

                            }
                        }
                        break;
                    }
                }
            }
        }
        
        p.append(curve.front());
        
        if (end_path.linksToPath() && end_path.getObject()) {

            Geom::PathVector linked_pathv = end_path.get_pathvector();
            Geom::Affine linkedtransform = end_path.getObject()->getRelativeTransform(sp_lpe_item);

            if ( !linked_pathv.empty() )
            {
                Geom::Path transformedpath = linked_pathv.front() * linkedtransform;
                Geom::Curve *last_seg_reverse = curve.front().back().reverse();
                
                end_path_curve_start.setOrigin(last_seg_reverse->initialPoint());

                std::vector<Geom::Point> derivs = last_seg_reverse->pointAndDerivatives(0, 3);
                for (unsigned deriv_n = 1; deriv_n < derivs.size(); deriv_n++) {
                    Geom::Coord length = derivs[deriv_n].length();
                    if ( ! Geom::are_near(length, 0) && !end_path.getObject()->_tmpsuccessor) {
                        if (set_end_end) {
                            end_path_position.param_set_value(transformedpath.nearestTime(end_path_curve_end.getOrigin()).asFlatTime());
                        }
                        
                        if (end_path_position > transformedpath.size()) {
                            end_path_position.param_set_value(transformedpath.size());
                        } else if (end_path_position < 0) {
                            end_path_position.param_set_value(0);
                        }
                        const Geom::Curve *c = end_path_position >= transformedpath.size() ?
							&transformedpath.back() : &transformedpath.at((int)end_path_position);

                        std::vector<Geom::Point> derivs_2 = c->pointAndDerivatives(end_path_position >= transformedpath.size() ? 1 : (end_path_position - (int)end_path_position), 3);
                        for (unsigned deriv_n_2 = 1; deriv_n_2 < derivs_2.size(); deriv_n_2++) {
                            Geom::Coord length_2 = derivs[deriv_n_2].length();
                            if ( ! Geom::are_near(length_2, 0) ) {
                                
                                end_path_curve_end.setOrigin(derivs_2[0]);
                                curve_end_previous_origin = end_path_curve_end.getOrigin();
                                
                                double startangle = atan2(end_path_curve_start.getVector().y(), end_path_curve_start.getVector().x());
                                double endangle = atan2(end_path_curve_end.getVector().y(), end_path_curve_end.getVector().x());
                                double startderiv = atan2(derivs[deriv_n].y(), derivs[deriv_n].x());
                                double endderiv = atan2(derivs_2[deriv_n_2].y(), derivs_2[deriv_n_2].x());
                                Geom::Point pt1 = Geom::Point(end_path_curve_start.getVector().length() * cos(startangle + startderiv), end_path_curve_start.getVector().length() * sin(startangle + startderiv));
                                Geom::Point pt2 = Geom::Point(end_path_curve_end.getVector().length() * cos(endangle + endderiv), end_path_curve_end.getVector().length() * sin(endangle + endderiv));
                                p.appendNew<Geom::CubicBezier>(-pt1 + curve.front().finalPoint(), -pt2 + derivs_2[0], derivs_2[0]);

                                break;

                            }
                        }
                        break;
                    }
                }
                delete last_seg_reverse;
            }
        }
        curve = std::move(p);
    }
}

} // namespace LivePathEffect
} /* namespace Inkscape */

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
