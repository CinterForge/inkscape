// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * LPE <offset> implementation
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Jabiertxo Arraiza
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 * Copyright (C) Jabierto Arraiza 2015 <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-offset.h"

#include "path/path-curve.h"
#include "helper/geom-pathstroke.h"
#include "helper/geom.h"
#include "live_effects/parameter/enum.h"
#include "object/sp-item-group.h"
#include "object/sp-lpe-item.h"
#include "object/sp-shape.h"
#include "ui/knot/knot-holder-entity.h"
#include "ui/knot/knot-holder.h"
#include "util/units.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

#include "ui/knot/knot.h"

namespace Inkscape {
namespace LivePathEffect {

namespace OfS {
    class KnotHolderEntityOffsetPoint : public LPEKnotHolderEntity<LPEOffset> {
    public:
        KnotHolderEntityOffsetPoint(LPEOffset * effect) : LPEKnotHolderEntity(effect) {}
        ~KnotHolderEntityOffsetPoint() override
        {
            _effect->_knotholder = nullptr;
        }
        void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
        void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
        Geom::Point knot_get() const override;
    private:
    };
} // OfS


static const Util::EnumData<unsigned> JoinTypeData[] = {
    // clang-format off
    {JOIN_BEVEL,       N_("Beveled"),    "bevel"},
    {JOIN_ROUND,       N_("Rounded"),    "round"},
    {JOIN_MITER,       N_("Miter"),      "miter"},
    {JOIN_MITER_CLIP,  N_("Miter Clip"), "miter-clip"},
    {JOIN_EXTRAPOLATE, N_("Extrapolated arc"), "extrp_arc"},
    {JOIN_EXTRAPOLATE1, N_("Extrapolated arc Alt1"), "extrp_arc1"},
    {JOIN_EXTRAPOLATE2, N_("Extrapolated arc Alt2"), "extrp_arc2"},
    {JOIN_EXTRAPOLATE3, N_("Extrapolated arc Alt3"), "extrp_arc3"},
    // clang-format on
};

static const Util::EnumDataConverter<unsigned> JoinTypeConverter(JoinTypeData, sizeof(JoinTypeData)/sizeof(*JoinTypeData));


LPEOffset::LPEOffset(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    unit(_("Unit"), _("Unit of measurement"), "unit", &wr, this, "mm"),
    offset(_("Offset:"), _("Offset"), "offset", &wr, this, 0.0),
    linejoin_type(_("Join:"), _("Determines the shape of the path's corners"),  "linejoin_type", JoinTypeConverter, &wr, this, JOIN_MITER),
    miter_limit(_("Miter limit:"), _("Maximum length of the miter join (in units of stroke width)"), "miter_limit", &wr, this, 4.0),
    attempt_force_join(_("Force miter"), _("Overrides the miter limit and forces a join."), "attempt_force_join", &wr, this, false),
    update_on_knot_move(_("Live update"), _("Update while moving handle"), "update_on_knot_move", &wr, this, true)
{
    show_orig_path = true;
    registerParameter(&linejoin_type);
    registerParameter(&unit);
    registerParameter(&offset);
    registerParameter(&miter_limit);
    registerParameter(&attempt_force_join);
    registerParameter(&update_on_knot_move);
    offset.param_set_increments(0.1, 0.1);
    offset.param_set_digits(6);
    offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    _knotholder = nullptr;
    _provides_knotholder_entities = true;
    apply_to_clippath_and_mask = true;
    prev_unit = unit.get_abbreviation();
    liveknot = false;
    fillrule = fill_nonZero;
}

LPEOffset::~LPEOffset() {
    modified_connection.disconnect();
    if (_knotholder) {
        _knotholder->clear();
        _knotholder = nullptr;
    }
};

bool LPEOffset::doOnOpen(SPLPEItem const *lpeitem)
{
    if (!is_load || is_applied) {
        return false;
    }
    Glib::ustring version = lpeversion.param_getSVGValue();
    if (version < "1.3") {
        lpeversion.param_setValue("1.3", true);
    }
    return false;
}

void
LPEOffset::doOnApply(SPLPEItem const* lpeitem)
{
    lpeversion.param_setValue("1.3", true);
}

Glib::ustring 
sp_get_fill_rule(SPObject *obj) {
    SPCSSAttr *css;
    css = sp_repr_css_attr (obj->getRepr() , "style");
    Glib::ustring  val = sp_repr_css_property (css, "fill-rule", "");
    sp_repr_css_attr_unref(css);
    return val;
} 

void
LPEOffset::modified(SPObject *obj, guint flags)
{
    // we check for style changes and apply LPE to get appropiate fill rule on change
    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG && obj) {
        // Get the used fillrule
        Glib::ustring fr = sp_get_fill_rule(obj);
        FillRule fillrule_chan = fill_nonZero;
        if (fr == "evenodd") {
            fillrule_chan = fill_oddEven;
        }
        if (fillrule != fillrule_chan && sp_lpe_item) {
            sp_lpe_item_update_patheffect(sp_lpe_item, true, true);
        }
    }
}

Geom::Point get_nearest_point(Geom::PathVector pathv, Geom::Point point)
{
    Geom::Point res = Geom::Point(Geom::infinity(), Geom::infinity());
    std::optional< Geom::PathVectorTime > pathvectortime = pathv.nearestTime(point);
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        res = pathv[(*pathvectortime).path_index].pointAt(pathtime.curve_index + pathtime.t);
    }
    return res;
}

void LPEOffset::transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    refresh_widgets = true;
    if (!postmul.isTranslation()) {
        Geom::Affine current_affine = sp_item_transform_repr(sp_lpe_item);
        offset.param_transform_multiply(postmul * current_affine.inverse(), true);
    }
    offset_pt *= postmul;
}

Geom::Point LPEOffset::get_default_point(Geom::PathVector pathv)
{
    Geom::Point origin = Geom::Point(Geom::infinity(), Geom::infinity());
    Geom::OptRect bbox = pathv.boundsFast();
    if (bbox) {
        origin = Geom::Point((*bbox).midpoint()[Geom::X], (*bbox).top());
        origin = get_nearest_point(pathv, origin);
    }
    return origin;
}

double
LPEOffset::sp_get_offset()
{
    double ret_offset = 0;
    Geom::Point res = Geom::Point(Geom::infinity(), Geom::infinity());
    std::optional< Geom::PathVectorTime > pathvectortime = mix_pathv_all.nearestTime(offset_pt);
    if (pathvectortime) {
        Geom::PathTime pathtime = pathvectortime->asPathTime();
        Geom::Path npath = mix_pathv_all[(*pathvectortime).path_index];
        res = npath.pointAt(pathtime.curve_index + pathtime.t);
        if (npath.closed()) {
            int winding_value = mix_pathv_all.winding(offset_pt);
            bool inset = false;
            if (winding_value % 2 != 0) {
                inset = true;
            }
            
            ret_offset = Geom::distance(offset_pt, res);
            if (inset) {
                ret_offset *= -1;
            }
        } else {
            ret_offset = Geom::distance(offset_pt, res);
            if (!sign) {
                ret_offset *= -1;
            }
        }
    }
    return Inkscape::Util::Quantity::convert(ret_offset, "px", unit.get_abbreviation()) * this->scale;
}

void
LPEOffset::addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(helper_path);
}

void
LPEOffset::doBeforeEffect (SPLPEItem const* lpeitem)
{
    auto obj = sp_lpe_item;
    if (is_load && obj) {
        modified_connection = obj->connectModified(sigc::mem_fun(*this, &LPEOffset::modified));
    }
    original_bbox(lpeitem);
    auto group = cast<SPGroup>(sp_lpe_item);
    if (group) {
        mix_pathv_all.clear();
    }
    this->scale = lpeitem->i2doc_affine().descrim();
    if (!is_load && prev_unit != unit.get_abbreviation()) {
        offset.param_set_undo(false);
        offset.param_set_value(Inkscape::Util::Quantity::convert(offset, prev_unit, unit.get_abbreviation()));
    } else {
        offset.param_set_undo(true);
    }
    prev_unit = unit.get_abbreviation();
}

void LPEOffset::doAfterEffect(SPLPEItem const *, Geom::PathVector *)
{
    if (offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        if (_knotholder && !_knotholder->entity.empty()) {
            _knotholder->entity.front()->knot_get();
        }
    }
    if (is_load) {
        offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    }
    if (_knotholder && !_knotholder->entity.empty() && sp_lpe_item && !liveknot) {
        Geom::PathVector out;
        // we don do this on groups, editing is joining ito so no need to update knot
        auto shape = cast<SPShape>(sp_lpe_item);
        if (shape) {
            out = *cast<SPShape>(sp_lpe_item)->curve();
            offset_pt = get_nearest_point(out, offset_pt);
            _knotholder->entity.front()->knot_get();
        }
    }
}

Geom::PathVector 
LPEOffset::doEffect_path(Geom::PathVector const & path_in)
{
    SPItem *item = current_shape;
    SPDocument *document = getSPDoc();
    if (!item || !document) {
        return path_in;
    }
    if (Geom::are_near(offset,0.0)) {
        // this is to keep reference to multiple pathvectors like in a group. Used by knot position in LPE Offset
        mix_pathv_all.insert(mix_pathv_all.end(), path_in.begin(), path_in.end());
        if (is_load && offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
            offset_pt = get_default_point(path_in);
            if (_knotholder && !_knotholder->entity.empty()) {
                _knotholder->entity.front()->knot_get();
            }
        }
        // allow offset 0 to get flattened version
    }
    // Get the used fillrule
    Glib::ustring fr = sp_get_fill_rule(item);
    fillrule = fill_nonZero;
    if (fr == "evenodd") {
        fillrule = fill_oddEven;
    }
    // ouline operations faster on live editing knot, on release it, get updated to -1
    // todo study remove/change value. Use text paths to test if is needed
    double tolerance = -1;
    if (liveknot) {
        tolerance = 3;
    }
    // Get the doc units offset
    double to_offset = Inkscape::Util::Quantity::convert(offset, unit.get_abbreviation(), "px") / this->scale;
    Geom::PathVector inputpv = path_in;
    Geom::PathVector prev_mix_pathv_all;
    Geom::PathVector prev_helper_path;
    bool is_group = is<SPGroup>(sp_lpe_item);
    // Pathvector used outside this function to calculate the offset
    if (!is_group) {
        mix_pathv_all.clear();
        helper_path.clear();
    }
    LineJoinType join = static_cast<LineJoinType>(linejoin_type.get_value());
    double miterlimit = attempt_force_join ? std::numeric_limits<double>::max() : miter_limit;
    Geom::Point point = offset_pt;
    if (is_group) {
        point = Geom::Point(Geom::infinity(), Geom::infinity());
    }
    Geom::PathVector out = do_offset(inputpv, to_offset, tolerance, miterlimit, fillrule,  join,  point, helper_path, mix_pathv_all);
    // we are fastering outline on expand
    if (!prev_mix_pathv_all.empty()) {
        mix_pathv_all = prev_mix_pathv_all;
        helper_path = prev_helper_path;
    }
    return out;
}

void LPEOffset::addKnotHolderEntities(KnotHolder *knotholder, SPItem *item)
{
    _knotholder = knotholder;
    KnotHolderEntity * knot_entity = new OfS::KnotHolderEntityOffsetPoint(this);
    knot_entity->create(nullptr, item, knotholder, Inkscape::CANVAS_ITEM_CTRL_TYPE_LPE,
                         "LPEOffset", _("Offset point"));
    knot_entity->knot->updateCtrl();
    offset_pt = Geom::Point(Geom::infinity(), Geom::infinity());
    _knotholder->add(knot_entity);
}

namespace OfS {

void KnotHolderEntityOffsetPoint::knot_set(Geom::Point const &p, Geom::Point const& /*origin*/, guint state)
{
    using namespace Geom;
    _effect->offset_pt = snap_knot_position(p, state);
    double offset = _effect->sp_get_offset();
    if (_effect->update_on_knot_move) {
        _effect->liveknot = true;
        _effect->offset.param_set_value(offset);
        sp_lpe_item_update_patheffect (cast<SPLPEItem>(item), false, false);
    } else {
        _effect->liveknot = false;
    }
}

void KnotHolderEntityOffsetPoint::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state)
{
    _effect->liveknot = false;
    using namespace Geom;
    double offset = _effect->sp_get_offset();
    _effect->offset.param_set_value(offset);
    _effect->makeUndoDone(RC_("Undo", "Move handle"));
}

Geom::Point KnotHolderEntityOffsetPoint::knot_get() const
{
    if (!_effect->update_on_knot_move) {
        return _effect->offset_pt;
    }
    if (_effect->offset_pt == Geom::Point(Geom::infinity(), Geom::infinity())) {
        _effect->offset_pt = _effect->get_default_point(_effect->pathvector_after_effect);
    }

    return _effect->offset_pt;
}

} // namespace OfS
} //namespace LivePathEffect
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
