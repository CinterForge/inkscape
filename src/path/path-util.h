// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path utilities.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef PATH_UTIL_H
#define PATH_UTIL_H

#include <memory>
#include <optional>

#include <2geom/forward.h>
#include <2geom/path.h>

#include "livarot/Path.h"

class SPItem;

/**
 * Creates a Livarot Path object from the Geom::PathVector.
 *
 * @param pathv A reference to the pathvector to convert.
 *
 * @return A pointer to the livarot Path object.
 */
std::unique_ptr<Path> Path_for_pathvector(Geom::PathVector const &pathv);

/**
 * Creates a Livarot Path object from an SPItem. The Geom::PathVector extracted from
 * the item would be before applying LPEs for SPPath and after applying LPEs for other shapes.
 *
 * @param item A pointer to the SPItem object.
 * @param doTransformation See the same parameter in pathvector_for_curve().
 * @param transformFull See the same parameter in pathvector_for_curve().
 *
 * @return A pointer to the livarot Path object.
 */
std::unique_ptr<Path> Path_for_item(SPItem *item, bool doTransformation, bool transformFull = true);

/**
 * Creates a Livarot Path object from the SPItem. This function ensures that the Geom::PathVector
 * extracted is the one before applying the LPE stack.
 *
 * @param item A pointer to the SPItem object.
 * @param doTransformation See the same parameter in pathvector_for_curve().
 * @param transformFull See the same parameter in pathvector_for_curve().
 *
 * @return A pointer to the livarot Path object.
 */
std::unique_ptr<Path> Path_for_item_before_LPE(SPItem *item, bool doTransformation, bool transformFull = true);

/**
 * Gets a Geom::PathVector from the SPCurve object.
 *
 * TODO: see if calling this method can be optimized. All the pathvector copying might be slow.
 *
 * @param item A pointer to the original SPItem. This is required to get the transformation
 * information.
 * @param curve A non-null pointer to the SPCurve.
 * @param doTransformation If set to true, the transformation in the SPItem is applied.
 * @param transformFull If doTransformation and transformFull are both set to true, the
 * i2doc_affine transformation is applied, which includes the all the transformations of the
 * ancestors and even the document viewport. If doTransformation is true but transformFull is false
 * only the item's own transformation gets applied.
 *
 * @return The Geom::PathVector.
 */
Geom::PathVector pathvector_for_curve(SPItem *item, Geom::PathVector const &curve, bool doTransformation, bool transformFull);

/**
 * Gets an SPCurve from the SPItem.
 *
 * An SPCurve is basically a wrapper around a Geom::PathVector. This function extracts
 * an SPCurve object from the SPItem. The behavior is a little ill-defined though. It
 * returns the path before applying LPE stack if the item is an SPPath and the path
 * after applying the LPE stack for all other types.
 *
 * For SPText, SPFlowtext and SPImage LPEs don't matter anyways (AFAIK). So, curve_for_item
 * and curve_for_item_before_LPE behave identically. For SPShape objects, there is a difference.
 * curve_for_item returns path before LPE only if it's SPPath and path after LPE otherwise.
 * curve_for_item_before_LPE returns path before LPE all the time. See the inheritance diagram
 * for these classes if this doesn't make sense.
 *
 * @param item Pointer to the SPItem object.
 *
 * @returns The extracted SPCurve
 */
std::optional<Geom::PathVector> curve_for_item(SPItem *item);

/**
 * Gets an SPCurve from the SPItem before any LPE.
 *
 * An SPCurve is basically a wrapper around a Geom::PathVector. This function extracts
 * an SPCurve object from the SPItem but it ensures that if LPEs are supported on that
 * SPItem the path before the LPE is returned.
 *
 * For SPText, SPFlowtext and SPImage LPEs don't matter anyways (AFAIK). So, curve_for_item
 * and curve_for_item_before_LPE behave identically. For SPShape objects, there is a difference.
 * curve_for_item returns path before LPE only if it's SPPath and path after LPE otherwise.
 * curve_for_item_before_LPE returns path before LPE all the time. See the inheritance diagram
 * for these classes if this doesn't make sense.
 *
 * @param item Pointer to the SPItem object.
 *
 * @returns The extracted SPCurve
 */
std::optional<Geom::PathVector> curve_for_item_before_LPE(SPItem *item);

/**
 * Get the nearest position given a Livarot Path and a point.
 *
 * I've not properly read the internals of the functions that get called for this function.
 * However, if the name is correct, it seems the function returns the point in time and the
 * piece where the path gets closest to the given point.
 *
 * TODO: Confirm this by checking the internals of the function that gets called.
 *
 * @param path A pointer to the Livarot Path object.
 * @param p The point to which you wanna find the nearest point on the Path.
 * @param seg If it is set to 0, all segments are considered. If set to any other number, only
 * that segment will be considered.
 *
 * @return The point at time t.
 */
std::optional<Path::cut_position> get_nearest_position_on_Path(Path *path, Geom::Point p, unsigned seg = 0);

/**
 * Gets the point at a particular time in a particular piece in a path description.
 *
 * @param path Pointer to the Livarot Path object.
 * @param piece The index of the path description. If this index is invalid, a point
 * at the origin will be returned.
 * @param t The time on the path description. Should be between 0 and 1.
 *
 * @return The cut position. This structure has both the piece index and the time for the nearest
 * point.
 */
Geom::Point get_point_on_Path(Path *path, int piece, double t);

/**
 * Intersect two clips, treating empty optionals as no clip.
 */
std::optional<Geom::PathVector> intersect_clips(std::optional<Geom::PathVector> &&a, std::optional<Geom::PathVector> &&b);

#endif // PATH_UTIL_H

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
