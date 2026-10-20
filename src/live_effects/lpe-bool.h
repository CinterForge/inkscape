// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Boolean operation live path effect
 *
 * Copyright (C) 2016 Michael Soegtrop
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_BOOL_H
#define INKSCAPE_LPE_BOOL_H

#include "livarot/LivarotDefs.h"
#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/hidden.h"
#include "live_effects/parameter/originalsatellite.h"
#include "live_effects/parameter/parameter.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEBool : public Effect {
public:
    LPEBool(LivePathEffectObject *lpeobject);
    ~LPEBool() override;

    void doEffect(Geom::PathVector &curve) override;
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    void doBeforeEffect(SPLPEItem const *lpeitem) override;
    void transform_multiply(Geom::Affine const &postmul, bool set) override;
    void doAfterEffect(SPLPEItem const* lpeitem, Geom::PathVector *curve) override;
    void doOnVisibilityToggled(SPLPEItem const * /*lpeitem*/) override;
    void doOnRemove(SPLPEItem const * /*lpeitem*/) override;
    bool doOnOpen(SPLPEItem const *lpeitem) override;
    void add_filter();
    void fractureit(SPObject * operandit, Geom::PathVector unionpv);
    void divisionit(SPObject * operand_a, SPObject * operand_b, Geom::PathVector unionpv);
    Geom::PathVector get_union(SPObject *root, SPObject *object, bool prefear_original = false);
    Inkscape::XML::Node *dupleNode(SPObject * origin, Glib::ustring element_type);
    void remove_filter(SPObject *object);
    enum bool_op_ex
    {
        bool_op_ex_union = bool_op_union,
        bool_op_ex_inters = bool_op_inters,
        bool_op_ex_diff = bool_op_diff,
        bool_op_ex_symdiff = bool_op_symdiff,
        bool_op_ex_cut = bool_op_cut,
        bool_op_ex_cut_both,
        // bool_op_ex_slice = bool_op_slice,
        // bool_op_ex_slice_inside,  // like bool_op_slice, but leaves only the contour pieces inside of the cut path
        // bool_op_ex_slice_outside, // like bool_op_slice, but leaves only the contour pieces outside of the cut path
        bool_op_ex_count
    };

    inline friend BooleanOp to_bool_op(bool_op_ex val)
    {
        //assert(val <= bool_op_ex_slice);
        assert(val <= bool_op_ex_cut);
        return (BooleanOp) val;
    }

private:
    LPEBool(const LPEBool &) = delete;
    LPEBool &operator=(const LPEBool &) = delete;

    OriginalSatelliteParam operand_item;
    EnumParam<bool_op_ex> bool_operation;
    EnumParam<FillRule> fill_type_this;
    EnumParam<FillRule> fill_type_operand;
    BoolParam swap_operands;
    BoolParam rmv_inner;
    bool onremove = false;
    SPItem *operand = nullptr;
    SPGroup *division = nullptr;
    SPGroup *division_both = nullptr;
    SPGroup *division_other = nullptr;
    Glib::ustring operand_id = "";
    Glib::ustring division_id = "";
    Glib::ustring division_other_id = "";
    HiddenParam filter;
    Geom::PathVector _hp;
    Geom::Affine prev_affine;
    bool reverse = false; 
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
