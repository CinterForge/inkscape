// SPDX-License-Identifier: GPL-2.0-or-later
//
// Stand-alone selection transformation operations
//

#ifndef INKSCAPE_TRANSFORM_OBJECT_H
#define INKSCAPE_TRANSFORM_OBJECT_H

// Transformation helpers.
// These functions operate on the current selection.

namespace Geom { class Affine; }
namespace Inkscape { class Selection; }

enum class SkewUnits {
    Percent,      // values are in percent (e.g., 10 = 10%)
    AngleRadians, // values are angles in radians which are converted with tan()
    Absolute      // values are unitless skew factors directly applied
};

// Move by (x, y). If relative == false, (x, y) are absolute top-left coordinates of bbox.
// If apply_separately == true, objects are shifted individually in a chained manner when relative==true,
// otherwise each item is positioned relative to its own bbox.
void transform_move(Inkscape::Selection* selection,
                    double x, double y,
                    bool relative,
                    bool apply_separately,
                    double yaxisdir);

// Scale to new width/height if is_percent == false, or by percent if true.
// If apply_separately == true, scaling is applied per-item around each item's center.
void transform_scale(Inkscape::Selection* selection,
                     double sx, double sy,
                     bool is_percent,
                     bool apply_separately,
                     bool transform_stroke,
                     bool preserve);

// Rotate by angle_degrees around selection center (or item centers when apply_separately == true).
void transform_rotate(Inkscape::Selection* selection,
                      double angle_degrees,
                      bool apply_separately);

// Skew by hx, hy according to units. yaxisdir is typically desktop->yaxisdir().
void transform_skew(Inkscape::Selection* selection,
                    double hx, double hy,
                    SkewUnits units,
                    bool apply_separately,
                    double yaxisdir);

// Apply affine matrix. If replace_matrix == true, replace each item's transform; otherwise post-multiply.
void transform_apply_matrix(Inkscape::Selection* selection,
                            const Geom::Affine& affine,
                            bool replace_matrix);

#endif //INKSCAPE_TRANSFORM_OBJECT_H