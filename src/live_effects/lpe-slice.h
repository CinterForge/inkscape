// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_SLICE_H
#define INKSCAPE_LPE_SLICE_H

/** \file
 * LPE <mirror_symmetry> implementation: mirrors a path with respect to a given line.
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *   Jabiertxof
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilin Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/lpegroupbbox.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/point.h"
#include "live_effects/parameter/satellitearray.h"

namespace Inkscape {
namespace LivePathEffect {


class LPESlice : public Effect, GroupBBoxEffect {
public:
    LPESlice(LivePathEffectObject *lpeobject);
    ~LPESlice() override;
    void doOnApply (SPLPEItem const* lpeitem) override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    void doAfterEffect (SPLPEItem const* lpeitem, Geom::PathVector *curve) override;
    Geom::PathVector doEffect_path (Geom::PathVector const & path_in) override;
    void doOnRemove(SPLPEItem const* /*lpeitem*/) override;
    void doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/) override;
    Gtk::Widget *newWidget() override;
    bool doOnOpen(SPLPEItem const *lpeitem) override;
    void cloneStyle(SPObject *orig, SPObject *dest);
    bool split(SPItem *item, Geom::PathVector *curve, std::vector<std::pair<Geom::Line, size_t>> slicer, size_t splitindex,
               bool &creation);
    bool splititem(SPItem *item, Geom::PathVector *curve, std::pair<Geom::Line, size_t> slicer, bool toggle,
                   bool is_original = false, Geom::Affine tpass = Geom::identity(), bool top = true);
    bool haschildslice(SPItem *item);
    std::vector<std::pair<Geom::Line, size_t> > getSplitLines();
    void cloneD(SPObject *orig, SPObject *dest, bool is_original); 
    Inkscape::XML::Node *createPathBase(SPObject *elemref);
    Geom::PathVector cutter(Geom::PathVector const & path_in);
    void originalDtoD(SPShape const *shape, Geom::PathVector *curve);
    void originalDtoD(SPItem *item);
    void resetStyles();
    void centerVert();
    void centerHoriz();
    SPObject *container;

protected:
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;

private:
    SatelliteArrayParam lpesatellites;
    BoolParam allow_transforms;
    PointParam start_point;
    PointParam end_point;
    PointParam center_point;
    Geom::Point previous_center;
    bool reset;
    bool blockreset;
    bool center_vert;
    bool center_horiz;
    bool allow_transforms_prev;
    size_t objindex = 0;
    bool legacy = false;
    LPESlice(const LPESlice&) = delete;
    LPESlice& operator=(const LPESlice&) = delete;
};

} //namespace LivePathEffect
} //namespace Inkscape

#endif
