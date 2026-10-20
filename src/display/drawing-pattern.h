// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Canvas belonging to SVG pattern.
 *//*
 * Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DISPLAY_DRAWING_PATTERN_H
#define INKSCAPE_DISPLAY_DRAWING_PATTERN_H

#include <mutex>
#include <cairomm/surface.h>
#include "drawing-group.h"

using cairo_pattern_t = struct _cairo_pattern;

namespace Inkscape {

/**
 * @brief Drawing tree node used for rendering paints.
 *
 * DrawingPattern is used for rendering patterns and hatches.
 *
 * It renders its children to a cairo_pattern_t structure that can be
 * applied as source for fill or stroke operations.
 */
class DrawingPattern
    : public DrawingGroup
{
public:
    DrawingPattern(Drawing &drawing);
    int tag() const override { return tag_of<decltype(*this)>; }

    /**
     * Set the transformation from pattern to user coordinate systems.
     * @see SPPattern description for explanation of coordinate systems.
     */
    void setPatternToUserTransform(Geom::Affine const &);

    /**
     * Set the tile rect position and dimensions in content coordinate system
     */
    void setTileRect(Geom::Rect const &);

    /**
     * Turn on overflow rendering.
     *
     * Overflow is implemented as repeated rendering of pattern contents. In every step
     * a translation transform is applied.
     */
    void setOverflow(Geom::Affine const &initial_transform, int steps, Geom::Affine const &step_transform);

    /**
     * Render the pattern.
     *
     * Returns cairo_pattern_t structure that can be set as source surface.
     */
    cairo_pattern_t *renderPattern(RenderContext &rc, Geom::IntRect const &area, float opacity, int device_scale) const;

protected:
    ~DrawingPattern() override = default;

    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset) override;

    void _dropPatternCache() override;

    std::unique_ptr<Geom::Affine> _pattern_to_user;

    // Set by overflow.
    Geom::Affine _overflow_initial_transform;
    Geom::Affine _overflow_step_transform;
    int _overflow_steps;

    Geom::OptRect _tile_rect;

    // Set on update.
    Geom::IntPoint _pattern_resolution;

    struct Surface
    {
        Surface(Geom::IntRect const &rect, int device_scale);
        Geom::IntRect rect;
        Cairo::RefPtr<Cairo::ImageSurface> surface;
    };

    mutable std::mutex mutables;

    // Parts of the pattern tile that have been rendered. Read/written on render, cleared on update.
    mutable std::vector<Surface> surfaces;
};

} // namespace Inkscape

#endif // INKSCAPE_DISPLAY_DRAWING_PATTERN_H

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
