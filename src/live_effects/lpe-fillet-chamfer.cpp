// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-fillet-chamfer.h"

#include <2geom/elliptical-arc.h>

#include "helper/geom-curves.h"
#include "helper/geom.h"
#include "helper/geom-nodesatellite.h"
#include "object/sp-rect.h"
#include "object/sp-shape.h"
#include "ui/knot/knot-holder.h"
#include "ui/pack.h"
#include "ui/tools/tool-base.h"
#include "ui/util.h"
#include "ui/widget/spinbutton.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape::LivePathEffect {

static const Util::EnumData<Filletmethod> FilletmethodData[] = {
    { FM_AUTO, N_("Auto"), "auto" },
    { FM_ARC, N_("Force arc"), "arc" },
    { FM_BEZIER, N_("Force bezier"), "bezier" }
};
static const Util::EnumDataConverter<Filletmethod> FMConverter(FilletmethodData, FM_END);

LPEFilletChamfer::LPEFilletChamfer(LivePathEffectObject *lpeobject)
    : Effect(lpeobject),
      unit(_("Unit:"), _("Unit"), "unit", &wr, this, "px"),
      nodesatellites_param("NodeSatellite_param", "NodeSatellite_param",
                       "nodesatellites_param", &wr, this),
      method(_("Method:"), _("Method to calculate the fillet or chamfer"),
             "method", FMConverter, &wr, this, FM_AUTO),
      mode(_("Mode:"), _("Mode, e.g. fillet or chamfer"),
             "mode", &wr, this, "F", true),
      radius(_("Radius:"), _("Radius, in unit or %"), "radius", &wr,
             this, 0.0),
      chamfer_steps(_("Chamfer steps:"), _("Chamfer steps"), "chamfer_steps",
                    &wr, this, 1),
      chamfer_scale(_("Chamfer scale (%):"), _("Scale chamfer steps toward the single-step chamfer line"),
                   "chamfer_scale", &wr, this, 100.0),
      flexible(_("Radius in %"), _("Flexible radius size (%)"),
               "flexible", &wr, this, false),
      only_selected(_("Change only selected nodes"),
                    _("Change only selected nodes"), "only_selected", &wr, this,
                    false),
      use_knot_distance(_("Use knots distance instead radius"),
                        _("Use knots distance instead radius"),
                        "use_knot_distance", &wr, this, true),
      hide_knots(_("Hide knots"), _("Hide knots"), "hide_knots", &wr, this,
                 false),
      apply_no_radius(_("Apply changes if radius = 0"), _("Apply changes if radius = 0"), "apply_no_radius", &wr, this, true),
      apply_with_radius(_("Apply changes if radius > 0"), _("Apply changes if radius > 0"), "apply_with_radius", &wr, this, true),
      _pathvector_nodesatellites(nullptr)
{
    // fix legacy < 1.2:
    const gchar * satellites_param = getLPEObj()->getAttribute("satellites_param");
    if (satellites_param){
        getLPEObj()->setAttribute("nodesatellites_param", satellites_param);
    };
    registerParameter(&nodesatellites_param);
    registerParameter(&radius);
    registerParameter(&unit);
    registerParameter(&method);
    registerParameter(&mode);
    registerParameter(&chamfer_steps);
    registerParameter(&chamfer_scale);
    registerParameter(&flexible);
    registerParameter(&use_knot_distance);
    registerParameter(&apply_no_radius);
    registerParameter(&apply_with_radius);
    registerParameter(&only_selected);
    registerParameter(&hide_knots);

    radius.param_set_range(0.0, std::numeric_limits<double>::max());
    radius.param_set_increments(1, 1);
    radius.param_set_digits(4);
    chamfer_steps.param_set_range(1, std::numeric_limits<gint>::max());
    chamfer_steps.param_set_increments(1, 1);
    chamfer_steps.param_make_integer();
    chamfer_scale.param_set_range(0.0, 100.0);
    chamfer_scale.param_set_increments(1, 10);
    chamfer_scale.param_set_digits(2);
    _provides_knotholder_entities = true;
    _provides_path_adjustment = true;
    helperpath = false;
    previous_unit = Glib::ustring("");
}

void LPEFilletChamfer::doOnApply(SPLPEItem const *lpeItem)
{
    SPLPEItem *splpeitem = const_cast<SPLPEItem *>(lpeItem);
    auto shape = cast<SPShape>(splpeitem);
    if (!shape) {
        g_warning("LPE Fillet/Chamfer can only be applied to shapes (not groups).");
        SPLPEItem *item = const_cast<SPLPEItem *>(lpeItem);
        item->removeCurrentPathEffect(false);
        return;
    }
    auto rect = cast<SPRect>(splpeitem);
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers(*shape->curve());
    double power = radius;
    double a = 0;
    if (rect) {
        double a = rect->getVisibleRx();
        a = std::max(a, rect->getVisibleRy());
        if (a) {
            rect->setRx(true, 0);
            rect->setRy(true, 0);
            pathv = Geom::PathVector(Geom::Path(rect->getRect()));
            if (!Geom::are_near(a, 0)) {
                a /= getSPDoc()->getDocumentScale()[Geom::X];
                unit.param_set_value(getSPDoc()->getWidth().unit->abbr.c_str());
                flexible.param_setValue(false);
                radius.param_set_value(a);
                power = a;
            }
        }
    }


    NodeSatellites nodesatellites;
    if (!flexible && Geom::are_near(a, 0)) {
        auto trans = lpeItem->transform.inverse();
        power = Inkscape::Util::Quantity::convert(power, unit.get_abbreviation(), "px") / getSPDoc()->getDocumentScale()[Geom::X];
        power *= ((trans.expansionX() + trans.expansionY()) / 2.0);
    }

    NodeSatelliteType nodesatellite_type = FILLET;

    std::map<std::string, NodeSatelliteType> gchar_map_to_nodesatellite_type = boost::assign::map_list_of(
        "F", FILLET)("IF", INVERSE_FILLET)("C", CHAMFER)("IC", INVERSE_CHAMFER)("KO", INVALID_SATELLITE);

    auto mode_str = mode.param_getSVGValue();
    auto it = gchar_map_to_nodesatellite_type.find(mode_str.raw());

    if (it != gchar_map_to_nodesatellite_type.end()) {
        nodesatellite_type = it->second;
    }
    if (!_pathvector_nodesatellites) {
        _pathvector_nodesatellites = new PathVectorNodeSatellites();
    }
    NodeSatellite nodesatellite(nodesatellite_type);
    nodesatellite.setSteps(chamfer_steps);
    nodesatellite.setAngle(chamfer_scale);
    nodesatellite.setAmount(power);
    nodesatellite.setIsTime(flexible);
    nodesatellite.setHasMirror(true);
    nodesatellite.setHidden(hide_knots);
    _pathvector_nodesatellites->recalculateForNewPathVector(pathv, nodesatellite);
    nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites);
}

Gtk::Widget *LPEFilletChamfer::newWidget()
{
    // use manage here, because after deletion of Effect object, others might
    // still be pointing to this widget.
    auto const vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    vbox->set_margin(5);

    for (auto const param: param_vector) {
        if (!param->widget_is_visible) continue;

        auto const widg = param->param_newWidget();
        if (!widg) continue;

        if (param->param_key == "radius") {
            auto &scalar = dynamic_cast<UI::Widget::Scalar &>(*widg);
            scalar.signal_value_changed().connect(
                sigc::mem_fun(*this, &LPEFilletChamfer::updateAmount));
            scalar.getSpinButton().set_width_chars(6);
        } else if (param->param_key == "chamfer_steps") {
            auto &scalar = dynamic_cast<UI::Widget::Scalar &>(*widg);
            scalar.signal_value_changed().connect(
                sigc::mem_fun(*this, &LPEFilletChamfer::updateChamferSteps));
            scalar.getSpinButton().set_width_chars(3);
        } else if (param->param_key == "chamfer_scale") {
            auto &scalar = dynamic_cast<UI::Widget::Scalar &>(*widg);
            scalar.signal_value_changed().connect(
                sigc::mem_fun(*this, &LPEFilletChamfer::updateChamferScale));
            scalar.getSpinButton().set_width_chars(5);
        }

        UI::pack_start(*vbox, *widg, true, true, 2);

        if (auto const tip = param->param_getTooltip()) {
            widg->set_tooltip_markup(*tip);
        } else {
            widg->set_tooltip_text({});
            widg->set_has_tooltip(false);
        }
    }

    // Fillet and chamfer containers

    auto const fillet_container = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    Gtk::Button *fillet =  Gtk::make_managed<Gtk::Button>(Glib::ustring(_("Fillet")));
    fillet->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &LPEFilletChamfer::updateNodeSatelliteType), FILLET));

    UI::pack_start(*fillet_container, *fillet, true, true, 2);
    auto const inverse_fillet = Gtk::make_managed<Gtk::Button>(Glib::ustring(_("Inverse fillet")));
    inverse_fillet->signal_clicked().connect(sigc::bind(
        sigc::mem_fun(*this, &LPEFilletChamfer::updateNodeSatelliteType), INVERSE_FILLET));
    UI::pack_start(*fillet_container, *inverse_fillet, true, true, 2);

    auto const chamfer_container = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    auto const chamfer = Gtk::make_managed<Gtk::Button>(Glib::ustring(_("Chamfer")));
    chamfer->signal_clicked().connect(
        sigc::bind(sigc::mem_fun(*this, &LPEFilletChamfer::updateNodeSatelliteType), CHAMFER));

    UI::pack_start(*chamfer_container, *chamfer, true, true, 2);
    auto const inverse_chamfer = Gtk::make_managed<Gtk::Button>(Glib::ustring(_("Inverse chamfer")));
    inverse_chamfer->signal_clicked().connect(sigc::bind(
        sigc::mem_fun(*this, &LPEFilletChamfer::updateNodeSatelliteType), INVERSE_CHAMFER));
    UI::pack_start(*chamfer_container, *inverse_chamfer, true, true, 2);

    UI::pack_start(*vbox, *fillet_container, true, true, 2);
    UI::pack_start(*vbox, *chamfer_container, true, true, 2);
    return vbox;
}

void LPEFilletChamfer::updateAmount()
{
    if (!_pathvector_nodesatellites) { // empty item
        return;
    }
    setSelected(_pathvector_nodesatellites);
    double power = radius;
    if (!flexible) {
        power = Inkscape::Util::Quantity::convert(power, unit.get_abbreviation(), "px") / getSPDoc()->getDocumentScale()[Geom::X];
        std::vector<SPLPEItem *> lpeitems = getCurrrentLPEItems();
        if (lpeitems.size() == 1) {
            sp_lpe_item = lpeitems[0];
            auto trans = sp_lpe_item->transform.inverse();
            power *= ((trans.expansionX() + trans.expansionY()) / 2.0);
        }
    }
    _pathvector_nodesatellites->updateAmount(power, apply_no_radius, apply_with_radius, only_selected,
                                             use_knot_distance, flexible);
    nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites);
}

void LPEFilletChamfer::updateChamferSteps()
{
    if (!_pathvector_nodesatellites) { // empty item
        return;
    }
    setSelected(_pathvector_nodesatellites);
    _pathvector_nodesatellites->updateSteps(chamfer_steps, apply_no_radius, apply_with_radius, only_selected);
    nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites);
}

void LPEFilletChamfer::updateChamferScale()
{
    if (!_pathvector_nodesatellites) { // empty item
        return;
    }
    setSelected(_pathvector_nodesatellites);
    _pathvector_nodesatellites->updateAngle(chamfer_scale, apply_no_radius, apply_with_radius, only_selected);
    nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites);
}

void LPEFilletChamfer::updateNodeSatelliteType(NodeSatelliteType nodesatellitetype)
{
    if (!_pathvector_nodesatellites) { // empty item
        return;
    }
    std::map<NodeSatelliteType, gchar const *> nodesatellite_type_to_gchar_map = boost::assign::map_list_of(
        FILLET, "F")(INVERSE_FILLET, "IF")(CHAMFER, "C")(INVERSE_CHAMFER, "IC")(INVALID_SATELLITE, "KO");
    mode.param_setValue((Glib::ustring)nodesatellite_type_to_gchar_map.at(nodesatellitetype));
    setSelected(_pathvector_nodesatellites);
    _pathvector_nodesatellites->updateNodeSatelliteType(nodesatellitetype, apply_no_radius, apply_with_radius,
                                                        only_selected);
    nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites);
}

void LPEFilletChamfer::setSelected(PathVectorNodeSatellites *_pathvector_nodesatellites)
{
    if (!_pathvector_nodesatellites) { // empty item
        return;
    }
    std::vector<SPLPEItem *> lpeitems = getCurrrentLPEItems();
    if (lpeitems.size() == 1) {
        sp_lpe_item = lpeitems[0];
        if (!_pathvector_nodesatellites) {
            sp_lpe_item_update_patheffect(sp_lpe_item, false, false);
        } else {
            Geom::PathVector const pathv = _pathvector_nodesatellites->getPathVector();
            NodeSatellites nodesatellites = _pathvector_nodesatellites->getNodeSatellites();
            for (size_t i = 0; i < nodesatellites.size(); ++i) {
                for (size_t j = 0; j < nodesatellites[i].size(); ++j) {
                    Geom::Curve const &curve_in = pathv[i][j];
                    if (only_selected && isNodePointSelected(curve_in.initialPoint()) ){
                        nodesatellites[i][j].setSelected(true);
                    } else {
                        nodesatellites[i][j].setSelected(false);
                    }
                }
            }
            _pathvector_nodesatellites->setNodeSatellites(nodesatellites);
        }
    }
}

void LPEFilletChamfer::doBeforeEffect(SPLPEItem const *lpeItem)
{
    if (!pathvector_before_effect.empty()) {
        bool const legacy_without_chamfer_scale = is_load && !getLPEObj()->getAttribute("chamfer_scale");
        //fillet chamfer specific calls
        nodesatellites_param.setUseDistance(use_knot_distance);
        nodesatellites_param.setCurrentZoom(current_zoom);
        //mandatory call
        nodesatellites_param.setEffectType(effectType());
        Geom::PathVector const pathv = pathv_to_linear_and_cubic_beziers(pathvector_before_effect);
        NodeSatellites nodesatellites = nodesatellites_param.data();
        if (nodesatellites.empty()) {
            doOnApply(lpeItem); // dont want _impl to not update versioning
            nodesatellites = nodesatellites_param.data();
        }

        for (size_t i = 0; i < nodesatellites.size(); ++i) {

            if (i >= pathv.size()) {
                // We have more nodesatellites vectors than paths. This could happen if two paths
                // are merged.
                break;
            }

            size_t curves_in_path = count_path_curves(pathv[i]);
            for (size_t j = 0; j < nodesatellites[i].size(); ++j) {
                if (j >= curves_in_path) {
                    // We have more nodesatellite points than curves in path. This could happen if
                    // a curve (node) is removed.
                    break;
                }

                Geom::Curve const &curve_in = pathv[i][j];
                if (nodesatellites[i][j].is_time != flexible) {
                    nodesatellites[i][j].is_time = flexible;
                    double amount = nodesatellites[i][j].amount;
                    if (nodesatellites[i][j].is_time) {
                        double time = timeAtArcLength(amount, curve_in);
                        nodesatellites[i][j].amount = time;
                    } else {
                        double size = arcLengthAt(amount, curve_in);
                        nodesatellites[i][j].amount = size;
                     }
                }

                nodesatellites[i][j].hidden = hide_knots;
                if (legacy_without_chamfer_scale) {
                    nodesatellites[i][j].angle = 100.0;
                }
                if (only_selected && isNodePointSelected(curve_in.initialPoint()) ){
                    nodesatellites[i][j].setSelected(true);
                }
            }

            // Set "amount" to zero for nodes at start and end of open path.
            if (!pathv[i].closed()) {
                if (nodesatellites[i].size() > 0) {
                    nodesatellites[i].front().amount = 0;
                    nodesatellites[i].back().amount = 0;
                }
            }
        }

        if (!_pathvector_nodesatellites) {
            _pathvector_nodesatellites = new PathVectorNodeSatellites();
        }

        if (is_load || _adjust_path) {
            double power = radius;
            if (!flexible) {
                power = Inkscape::Util::Quantity::convert(power, unit.get_abbreviation(), "px") / getSPDoc()->getDocumentScale()[Geom::X];
            }
            _adjust_path = false; // not wait till effect finish
            NodeSatelliteType nodesatellite_type = FILLET;
            std::map<std::string, NodeSatelliteType> gchar_map_to_nodesatellite_type = boost::assign::map_list_of(
                "F", FILLET)("IF", INVERSE_FILLET)("C", CHAMFER)("IC", INVERSE_CHAMFER)("KO", INVALID_SATELLITE);
            auto mode_str = mode.param_getSVGValue();
            auto it = gchar_map_to_nodesatellite_type.find(mode_str.raw());
            if (it != gchar_map_to_nodesatellite_type.end()) {
                nodesatellite_type = it->second;
            }
            NodeSatellite nodesatellite(nodesatellite_type);
            nodesatellite.setSteps(chamfer_steps);
            nodesatellite.setAngle(chamfer_scale);
            nodesatellite.setAmount(power);
            nodesatellite.setIsTime(flexible);
            nodesatellite.setHasMirror(true);
            nodesatellite.setHidden(hide_knots);
            _pathvector_nodesatellites->setNodeSatellites(nodesatellites);
            _pathvector_nodesatellites->recalculateForNewPathVector(pathv, nodesatellite);
            nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites, true);
            nodesatellites_param.reloadKnots();
        } else {
            _pathvector_nodesatellites->setPathVector(pathv);
            _pathvector_nodesatellites->setNodeSatellites(nodesatellites);
            nodesatellites_param.setPathVectorNodeSatellites(_pathvector_nodesatellites, false);
        }
    } else {
        g_warning("LPE Fillet can only be applied to shapes (not groups).");
    }
}

void
LPEFilletChamfer::adjustForNewPath() {
    _adjust_path = true;
}

void
LPEFilletChamfer::addCanvasIndicators(SPLPEItem const */*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

void
LPEFilletChamfer::addChamferSteps(Geom::Path &tmp_path, Geom::Path path_chamfer, Geom::Point end_arc_point, size_t steps, double scale)
{
    setSelected(_pathvector_nodesatellites);
    Geom::Point const start_arc_point = tmp_path.finalPoint();

    if (steps <= 1) {
        tmp_path.appendNew<Geom::LineSegment>(end_arc_point);
        return;
    }

    double clamped_scale = scale;
    if (clamped_scale < 0.0) {
        clamped_scale = 0.0;
    } else if (clamped_scale > 100.0) {
        clamped_scale = 100.0;
    }
    double const scale_factor = clamped_scale / 100.0;

    Geom::Point const line = end_arc_point - start_arc_point;
    double const line_length = Geom::L2(line);
    bool const can_project = line_length > Geom::EPSILON;
    Geom::Point normal;
    if (can_project) {
        normal = Geom::Point(-line[Geom::Y], line[Geom::X]) / line_length;
    }

    double path_subdivision = 1.0 / steps;
    for (size_t i = 1; i < steps; i++) {
        Geom::Point chamfer_step = path_chamfer.pointAt(path_subdivision * i);
        if (can_project && scale_factor < 1.0) {
            double const signed_distance = Geom::dot(chamfer_step - start_arc_point, normal);
            chamfer_step -= normal * (signed_distance * (1.0 - scale_factor));
        }
        tmp_path.appendNew<Geom::LineSegment>(chamfer_step);
    }
    tmp_path.appendNew<Geom::LineSegment>(end_arc_point);
}

/*
 * Convert the original path to the LPE path.
 */
Geom::PathVector
LPEFilletChamfer::doEffect_path(Geom::PathVector const &path_in)
{
    if (!_pathvector_nodesatellites) { // Empty item pathv with lpe
        std::cerr << "LPEFilletChamfer::doEffect_path:  No node satellites! "
                  << path_in << std::endl;
        return path_in;
    }

    const double GAP_HELPER = 0.00001;
    const double K = (4.0 / 3.0) * (sqrt(2.0) - 1.0);

    Geom::PathVector const pathv = _pathvector_nodesatellites->getPathVector();
    NodeSatellites nodesatellites = _pathvector_nodesatellites->getNodeSatellites();
    Geom::PathVector path_out;
    std::size_t path = -1; // Used to keep path and nodesatellites in sync.

    for (const auto &path_it : pathv) {

        ++path; // Must increment before skipping empty paths.

        // Skip empty paths ("M 0,0" or "M 0,0 z"). (But must include them in the indexing.)
        if (path_it.empty()) {
            continue;
        }

        Geom::Path::const_iterator curve_it1 = path_it.begin();
        Geom::Path::const_iterator curve_endit = path_it.end_default();

        // Treat almost degenerate closing lines as degenerate (allows for rounding errors).
        // Matches count_path_curves() and count_path_nodes().
        if (path_it.closed()) {
            auto const &closingline = path_it.closingSegment(); // Always a Geom::LineSegment.
            if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
                curve_endit = path_it.end_open();
            }
        }

        Geom::Path tmp_path;
        double time0 = 0;
        std::size_t curve = -1;
        size_t tcurves = count_path_curves(pathv[path]);
        if (tcurves < 2) {
            path_out.push_back(pathv[path]);
            continue;
        }
        while (curve_it1 != curve_endit) {

            ++curve;
            size_t next_index = curve + 1;

            // Loop around closed paths.
            if (curve == tcurves - 1 && pathv[path].closed()) {
                next_index = 0;
            }

            // Append last section of last curve for open paths.
            if (curve == tcurves - 1 && !pathv[path].closed()) {
                // At end of path and path is open.

                if (time0 != 1) {
                    // Previous nodesatellite not at 100% amount (thus we still are using part of
                    // the original path).
                    Geom::Curve *last_curve = curve_it1->portion(time0, 1);
                    last_curve->setInitial(tmp_path.finalPoint()); // Seamless connection.
                    tmp_path.append(*last_curve);
                }
                ++curve_it1;
                break; // We're done!
            }

            Geom::Curve const &curve_it2 = pathv.at(path).at(next_index);
            NodeSatellite nodesatellite = nodesatellites.at(path).at(next_index);

            // Start of path.
            if (!curve) { // curve == 0
                if (!path_it.closed()) {
                    time0 = 0;
                } else {
                    time0 = nodesatellites[path][0].time(*curve_it1);
                }
            }

            // Find section of original path that we keep (T0 to T1),
            // and section that is replaced (T1 to T2).
            double s = nodesatellite.arcDistance(curve_it2);
            double time1 = nodesatellite.time(s, true, (*curve_it1));
            double time2 = nodesatellite.time(curve_it2);
            if (time1 <= time0) {
                time1 = time0;
            }
            if (time2 > 1) {
                time2 = 1;
            }

            // Part of curve we are keeping.
            Geom::Curve *knot_curve_1 = curve_it1->portion(time0, time1);

            // Part of next curve we may be keeping.
            Geom::Curve *knot_curve_2 = curve_it2.portion(time2, 1);

            // Ensure seemless connections.
            if (curve > 0) {
                knot_curve_1->setInitial(tmp_path.finalPoint());
            } else {
                tmp_path.start((*curve_it1).pointAt(time0));
            }

            // Start and end points of section we are replacing.
            Geom::Point start_arc_point = knot_curve_1->finalPoint();
            Geom::Point end_arc_point = curve_it2.pointAt(time2);

            // Add a gap helper (why?)
            if (time2 == 1) {
                end_arc_point = curve_it2.pointAt(time2 - GAP_HELPER);
            }
            if (time1 == time0) {
                start_arc_point = curve_it1->pointAt(time1 + GAP_HELPER);
            }

            // Calculate points used corner section.
            Geom::Point curveit1 = curve_it1->finalPoint();
            Geom::Point curveit2 = curve_it2.initialPoint();
            double k1 = distance(start_arc_point, curveit1) * K;
            double k2 = distance(curve_it2.initialPoint(), end_arc_point) * K;

            // What happens if input curves are quadratic?
            Geom::CubicBezier const *cubic_1 = dynamic_cast<Geom::CubicBezier const *>(&*knot_curve_1);
            Geom::CubicBezier const *cubic_2 = dynamic_cast<Geom::CubicBezier const *>(&*knot_curve_2);
            Geom::Ray ray_1(start_arc_point, curveit1);
            Geom::Ray ray_2(curveit2, end_arc_point);
            if (cubic_1) {
                ray_1.setPoints((*cubic_1)[2], start_arc_point);
            }
            if (cubic_2) {
                ray_2.setPoints(end_arc_point, (*cubic_2)[1]);
            }
            bool ccw_toggle = cross(curveit1 - start_arc_point, end_arc_point - start_arc_point) < 0;
            double angle = angle_between(ray_1, ray_2, ccw_toggle);
            double handle_angle_1 = ray_1.angle() - angle;
            double handle_angle_2 = ray_2.angle() + angle;
            if (ccw_toggle) {
                handle_angle_1 = ray_1.angle() + angle;
                handle_angle_2 = ray_2.angle() - angle;
            }
            Geom::Point handle_1 = Geom::Point::polar(ray_1.angle(), k1) + start_arc_point;
            Geom::Point handle_2 = end_arc_point - Geom::Point::polar(ray_2.angle(), k2);
            Geom::Point inverse_handle_1 = Geom::Point::polar(handle_angle_1, k1) + start_arc_point;
            Geom::Point inverse_handle_2 = end_arc_point - Geom::Point::polar(handle_angle_2, k2);
            if (time0 == 1) {
                handle_1 = start_arc_point;
                inverse_handle_1 = start_arc_point;
            }

            // Remove gap helper.
            if (time2 == 1) {
                end_arc_point = curve_it2.pointAt(time2);
            }
            if (time1 == time0) {
                start_arc_point = curve_it1->pointAt(time0);
            }

            // Can part of this test be moved earlier?
            if (time1 != 1 && // We have a section to replace
                !Geom::are_near(angle, Geom::rad_from_deg(360)) && // It's not a straight connection
                !curve_it1->isDegenerate() && // Curves are not degenerate.
                !curve_it2.isDegenerate())
            {
                // Add section we're keeping.
                if (time1 != time0 || (time1 == 1 && time0 == 1)) {
                    if (!knot_curve_1->isDegenerate()) {
                        tmp_path.append(*knot_curve_1);
                    }
                }

                // More calculations of points for constructing corner section.
                NodeSatelliteType type = nodesatellite.nodesatellite_type;
                size_t steps = nodesatellite.steps;
                if (!steps) steps = 1;
                Geom::Line const x_line(Geom::Point(0, 0), Geom::Point(1, 0));
                Geom::Line const angled_line(start_arc_point, end_arc_point);
                double arc_angle = Geom::angle_between(x_line, angled_line);
                double radius = Geom::distance(start_arc_point, middle_point(start_arc_point, end_arc_point)) /
                                               sin(angle / 2.0);
                Geom::Coord rx = radius;
                Geom::Coord ry = rx;
                bool eliptical = (is_straight_curve(*curve_it1) &&
                                  is_straight_curve(curve_it2) && method != FM_BEZIER) ||
                                  method == FM_ARC;

                // Add corner.
                switch (type) {
                case CHAMFER:
                    {
                        Geom::Path path_chamfer;
                        path_chamfer.start(tmp_path.finalPoint());
                        if (eliptical) {
                            ccw_toggle = !ccw_toggle;
                            path_chamfer.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, false, ccw_toggle, end_arc_point);
                        } else {
                            path_chamfer.appendNew<Geom::CubicBezier>(handle_1, handle_2, end_arc_point);
                        }
                        addChamferSteps(tmp_path, path_chamfer, end_arc_point, steps, nodesatellite.angle);
                    }
                    break;
                case INVERSE_CHAMFER:
                    {
                        Geom::Path path_chamfer;
                        path_chamfer.start(tmp_path.finalPoint());
                        if (eliptical) {
                            path_chamfer.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, false, ccw_toggle, end_arc_point);
                        } else {
                            path_chamfer.appendNew<Geom::CubicBezier>(inverse_handle_1, inverse_handle_2, end_arc_point);
                        }
                        addChamferSteps(tmp_path, path_chamfer, end_arc_point, steps, nodesatellite.angle);
                    }
                    break;
                case INVERSE_FILLET:
                    {
                        if (eliptical) {
                            tmp_path.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, false, ccw_toggle, end_arc_point);
                        } else {
                            tmp_path.appendNew<Geom::CubicBezier>(inverse_handle_1, inverse_handle_2, end_arc_point);
                        }
                    }
                    break;
                default: // Fillet
                    {
                        if (eliptical) {
                            ccw_toggle = !ccw_toggle;
                            tmp_path.appendNew<Geom::EllipticalArc>(rx, ry, arc_angle, false, ccw_toggle, end_arc_point);
                        } else {
                            tmp_path.appendNew<Geom::CubicBezier>(handle_1, handle_2, end_arc_point);
                        }
                    }
                    break;
                }
            } else {
                // We didn't have a section to replace...
                if (!knot_curve_1->isDegenerate()) {
                    tmp_path.append(*knot_curve_1);
                }
            }

            // Move to next curve.
            ++curve_it1;
            time0 = time2;
        } // Loop over curves.

        if (path_it.closed()) {
            tmp_path.close();
        }

        path_out.push_back(tmp_path);
    } // Loop over paths (sub-paths).

    if (helperpath) {
        _hp = path_out;
        return pathvector_after_effect; // Original path vector
    }

    _hp.clear();
    return path_out;
}

} // namespace Inkscape::LivePathEffect

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offset:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
