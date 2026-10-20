// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path utilities.
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "path-curve.h"
#include "path-util.h"
#include "path/path-boolop.h"
#include "text-editing.h"
#include "livarot/Path.h"

#include "object/sp-flowtext.h"
#include "object/sp-image.h"
#include "object/sp-path.h"
#include "object/sp-text.h"

std::unique_ptr<Path> Path_for_pathvector(Geom::PathVector const &pathv)
{
    auto dest = std::make_unique<Path>();
    dest->LoadPathVector(pathv);
    return dest;
}

std::unique_ptr<Path> Path_for_item(SPItem *item, bool doTransformation, bool transformFull)
{
    auto curve = curve_for_item(item);

    if (!curve) {
        return nullptr;
    }

    auto pathv = pathvector_for_curve(item, *curve, doTransformation, transformFull);

    return Path_for_pathvector(pathv);
}

std::unique_ptr<Path> Path_for_item_before_LPE(SPItem *item, bool doTransformation, bool transformFull)
{
    auto curve = curve_for_item_before_LPE(item);

    if (!curve) {
        return nullptr;
    }
    
    auto pathv = pathvector_for_curve(item, *curve, doTransformation, transformFull);
    
    return Path_for_pathvector(pathv);
}

Geom::PathVector pathvector_for_curve(SPItem *item, Geom::PathVector const &curve, bool doTransformation, bool transformFull)
{
    auto result = curve;
    
    if (doTransformation) {
        if (transformFull) {
            result *= item->i2doc_affine();
        } else {
            result *= item->transform;
        }
    }

    return result;
}

std::optional<Geom::PathVector> curve_for_item(SPItem *item)
{
    if (!item) {
        return {};
    }
    
    if (auto path = cast<SPPath>(item)) {
        return ptr_to_opt(path->curveForEdit());
    } else if (auto shape = cast<SPShape>(item)) {
        return ptr_to_opt(shape->curve());
    } else if (is<SPText>(item) || is<SPFlowtext>(item)) {
        return te_get_layout(item)->convertToCurves();
    } else if (auto image = cast<SPImage>(item)) {
        return ptr_to_opt(image->get_curve());
    }
    
    return {};
}

std::optional<Geom::PathVector> curve_for_item_before_LPE(SPItem *item)
{
    if (!item) {
        return {};
    }

    if (auto shape = cast<SPShape>(item)) {
        return ptr_to_opt(shape->curveForEdit());
    } else if (is<SPText>(item) || is<SPFlowtext>(item)) {
        return te_get_layout(item)->convertToCurves();
    } else if (auto image = cast<SPImage>(item)) {
        return ptr_to_opt(image->get_curve());
    }
    
    return {};
}

std::optional<Path::cut_position> get_nearest_position_on_Path(Path *path, Geom::Point p, unsigned seg)
{
    if (!path) {
        return {};
    }
    // Get nearest position on path.
    return path->PointToCurvilignPosition(p, seg);
}

Geom::Point get_point_on_Path(Path *path, int piece, double t)
{
    Geom::Point p;
    path->PointAt(piece, t, p);
    return p;
}

std::optional<Geom::PathVector> intersect_clips(std::optional<Geom::PathVector> &&a, std::optional<Geom::PathVector> &&b)
{
    if (!a) return std::move(b);
    if (!b) return std::move(a);
    if (a->empty()) return std::move(a);
    if (b->empty()) return std::move(b);
    return sp_pathvector_boolop(*a, *b, bool_op_inters, fill_nonZero, fill_nonZero);
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
