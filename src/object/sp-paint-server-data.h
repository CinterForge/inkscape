// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Data for paint server objects useful for renderers.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_PAINT_SERVER_DATA_H
#define SEEN_SP_PAINT_SERVER_DATA_H

#include <vector>
#include <2geom/point.h>
#include "colors/color.h"

enum class PaintServerType
{
    INVALID,
    SOLID_COLOR,
    HATCH_PATTERN,
    LINEAR_GRADIENT,
    RADIAL_GRADIENT,
    MESH_GRADIENT
};

enum SPGradientSpread {
    SP_GRADIENT_SPREAD_PAD,
    SP_GRADIENT_SPREAD_REFLECT,
    SP_GRADIENT_SPREAD_REPEAT,
    SP_GRADIENT_SPREAD_UNDEFINED = INT_MAX
};

enum SPGradientUnits {
    SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX,
    SP_GRADIENT_UNITS_USERSPACEONUSE
};

/**
 * Differs from SPStop in that SPStop mirrors the \<stop\> element in the document, whereas
 * SPGradientStop shows more the effective stop color.
 *
 * For example, SPGradientStop has no currentColor option: currentColor refers to the color
 * property value of the gradient where currentColor appears, so we interpret currentColor before
 * copying from SPStop to SPGradientStop.
 */
struct SPGradientStop {
    std::optional<Inkscape::Colors::Color> color;
    double offset;
};

/**
 * The effective gradient vector, after copying stops from the referenced gradient if necessary.
 */
struct SPGradientVector {
    bool built;
    std::vector<SPGradientStop> stops;

    // For linear geometry is x1,y1,x2,y2; for radial it's cx,cy,r,fx,fy,fr
    std::vector<double> geom;
};

/**
 * The data store for a single Mesh gradient patch (i.e. the mesh equiv of a stop)
 */
struct SPGradientPatch
{
    Geom::Point points[4][4];
    char pathtype[4];
    bool tensorIsSet[4];
    Geom::Point tensorpoints[4];
    std::optional<Inkscape::Colors::Color> color[4];
};

/**
 * The effective gradient patch set (mesh), once processed correctly
 */
struct SPGradientMesh
{
    bool built;

    std::vector<std::vector<SPGradientPatch>> patches;
};

#endif /* !SEEN_SP_PAINT_SERVER_DATA_H */

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
