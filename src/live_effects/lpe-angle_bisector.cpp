// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Johan Engelen <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Copyright (C) Authors 2007-2012
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-angle_bisector.h"

#include <glibmm/i18n.h>

#include <2geom/sbasis-to-bezier.h>

#include "object/sp-lpe-item.h"
#include "ui/knot/knot-holder.h"
#include "ui/knot/knot-holder-entity.h"


namespace Inkscape {
namespace LivePathEffect {

namespace AB {

class KnotHolderEntityLeftEnd : public LPEKnotHolderEntity<LPEAngleBisector> {
public:
    KnotHolderEntityLeftEnd(LPEAngleBisector* effect) : LPEKnotHolderEntity(effect) {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    Geom::Point knot_get() const override;
};

class KnotHolderEntityRightEnd : public LPEKnotHolderEntity<LPEAngleBisector> {
public:
    KnotHolderEntityRightEnd(LPEAngleBisector* effect) : LPEKnotHolderEntity(effect) {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    Geom::Point knot_get() const override;
};

} // namespace AB

LPEAngleBisector::LPEAngleBisector(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    length_left(_("Length left:"), _("Specifies the left end of the bisector"), "length-left", &wr, this, 0),
    length_right(_("Length right:"), _("Specifies the right end of the bisector"), "length-right", &wr, this, 250)
{
    show_orig_path = true;
    _provides_knotholder_entities = true;

    registerParameter( &length_left );
    registerParameter( &length_right );
}

LPEAngleBisector::~LPEAngleBisector() = default;

Geom::PathVector
LPEAngleBisector::doEffect_path (Geom::PathVector const & path_in)
{
    using namespace Geom;

    // we assume that the path has >= 3 nodes
    ptA = path_in[0].pointAt(1);
    Point B = path_in[0].initialPoint();
    Point C = path_in[0].pointAt(2);

    double angle = angle_between(B - ptA, C - ptA);

    dir = unit_vector(B - ptA) * Rotate(angle/2);

    Geom::Point D = ptA - dir * length_left;
    Geom::Point E = ptA + dir * length_right;

    Piecewise<D2<SBasis> > output = Piecewise<D2<SBasis> >(D2<SBasis>(SBasis(D[X], E[X]), SBasis(D[Y], E[Y])));

    return path_from_piecewise(output, LPE_CONVERSION_TOLERANCE);
}

void
LPEAngleBisector::addKnotHolderEntities(KnotHolder *knotholder, SPDesktop *desktop, SPItem *item) {
    {
        KnotHolderEntity *e = new AB::KnotHolderEntityLeftEnd(this);
        e->create(desktop, item, knotholder, Inkscape::CANVAS_ITEM_CTRL_TYPE_LPE, "LPE:LeftEnd",
                  _("Adjust the \"left\" end of the bisector"));
        knotholder->add(e);
    }
    {
        KnotHolderEntity *e = new AB::KnotHolderEntityRightEnd(this);
        e->create(desktop, item, knotholder, Inkscape::CANVAS_ITEM_CTRL_TYPE_LPE, "LPE:RightEnd",
                  _("Adjust the \"right\" end of the bisector"));
        knotholder->add(e);
    }
};

namespace AB {

void
KnotHolderEntityLeftEnd::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, guint state)
{
    Geom::Point const s = snap_knot_position(p, state);

    double lambda = Geom::nearest_time(s, _effect->ptA, _effect->dir);
    _effect->length_left.param_set_value(-lambda);

    sp_lpe_item_update_patheffect (cast<SPLPEItem>(item), false, true);
}

void
KnotHolderEntityRightEnd::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, guint state)
{
    Geom::Point const s = snap_knot_position(p, state);

    double lambda = Geom::nearest_time(s, _effect->ptA, _effect->dir);
    _effect->length_right.param_set_value(lambda);

    sp_lpe_item_update_patheffect (cast<SPLPEItem>(item), false, true);
}

Geom::Point
KnotHolderEntityLeftEnd::knot_get() const
{
    return _effect->ptA - _effect->dir * _effect->length_left;
}

Geom::Point
KnotHolderEntityRightEnd::knot_get() const
{
    return _effect->ptA + _effect->dir * _effect->length_right;
}

} // namespace AB

/* ######################## */

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
