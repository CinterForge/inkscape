// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_DISPLAY_DRAWING_PAINT_SERVER_H
#define INKSCAPE_DISPLAY_DRAWING_PAINT_SERVER_H
/**
 * @file
 * Representation of paint servers used when rendering.
 */
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <array>
#include <vector>
#include <cairo.h>
#include <2geom/rect.h>
#include <2geom/affine.h>
#include "object/sp-paint-server-data.h"

class SPGradient;
class SPPaintServer;

namespace Inkscape {
namespace Colors {
class Color;
}

/**
 * A DrawingPaintServer is a lightweight copy of the resources needed to paint using a paint server.
 *
 * It is built by an SPPaintServer, stored in the DrawingItem tree, and used when rendering.
 *
 * The pattern information is stored in a rendering-backend agnostic way, and the object remains
 * valid even after the original SPPaintServer is modified or destroyed.
 */
class DrawingPaintServer
{
public:
    virtual ~DrawingPaintServer() = 0;

    /// Produce a pattern that can be used for painting with Cairo.
    virtual cairo_pattern_t *create_pattern(cairo_t *ct, Geom::OptRect const &bbox, double opacity) const = 0;

    /// Return whether this paint server could benefit from dithering.
    virtual bool ditherable() const { return false; }

    /// Return whether create_pattern() uses its cairo_t argument. Such pattern cannot be cached, but recreated each time.
    /// Fixme: The only reson this exists is to work around https://gitlab.freedesktop.org/cairo/cairo/-/issues/146.
    virtual bool uses_cairo_ctx() const { return false; }
};

// Todo: Remove, merging with existing implementation for solid colours.
/**
 * A simple solid color, storing an RGB color and an opacity.
 */
class DrawingSolidColor final
    : public DrawingPaintServer
{
public:
    DrawingSolidColor(Colors::Color color);
    cairo_pattern_t *create_pattern(cairo_t *, Geom::OptRect const &, double opacity) const override;

private:
    Colors::Color color;
};

/**
 * The base class for all gradients.
 */
class DrawingGradient
    : public DrawingPaintServer
{
protected:
    DrawingGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform)
        : spread(spread)
        , units(units)
        , transform(transform) {}

    bool ditherable() const override { return true; }

    /// Perform some common initialization steps on the given Cairo pattern.
    void common_setup(cairo_pattern_t *pat, Geom::OptRect const &bbox, double opacity) const;

    SPGradientSpread spread;
    SPGradientUnits units;
    Geom::Affine transform;
};

/**
 * A linear gradient.
 */
class DrawingLinearGradient final
    : public DrawingGradient
{
public:
    DrawingLinearGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform,
                          SPGradientVector const *vector)
        : DrawingGradient(spread, units, transform)
        , x1(vector->geom[0])
        , y1(vector->geom[1])
        , x2(vector->geom[2])
        , y2(vector->geom[3])
        , stops(vector->stops) {}

    cairo_pattern_t *create_pattern(cairo_t*, Geom::OptRect const &bbox, double opacity) const override;

private:
    float x1, y1, x2, y2;
    std::vector<SPGradientStop> stops;
};

/**
 * A radial gradient.
 */
class DrawingRadialGradient final
    : public DrawingGradient
{
public:
    DrawingRadialGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform,
                          SPGradientVector const *vector)
        : DrawingGradient(spread, units, transform)
        , cx(vector->geom[0])
        , cy(vector->geom[1])
        , r(vector->geom[2])
        , fx(vector->geom[3])
        , fy(vector->geom[4])
        , fr(vector->geom[5])
        , stops(vector->stops) {}

    cairo_pattern_t *create_pattern(cairo_t *ct, Geom::OptRect const &bbox, double opacity) const override;

    bool uses_cairo_ctx() const override { return true; }

private:
    float cx, cy, r, fx, fy, fr;
    std::vector<SPGradientStop> stops;
};

/**
 * A mesh gradient.
 */
class DrawingMeshGradient final
    : public DrawingGradient
{
public:
    DrawingMeshGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform,
                        SPGradientMesh const *mesh)
        : DrawingGradient(spread, units, transform)
        , patchdata(mesh->patches) {}

    cairo_pattern_t *create_pattern(cairo_t*, Geom::OptRect const &bbox, double opacity) const override;

private:
    std::vector<std::vector<SPGradientPatch>> patchdata;
};

std::unique_ptr<Inkscape::DrawingPaintServer> create_drawing_paintserver(SPPaintServer *ps);

} // namespace Inkscape

#endif // INKSCAPE_DISPLAY_DRAWING_PAINT_SERVER_H
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
