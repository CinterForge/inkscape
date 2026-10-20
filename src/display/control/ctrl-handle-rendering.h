// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_DISPLAY_CONTROL_CTRL_HANDLE_RENDERING_H
#define INKSCAPE_DISPLAY_CONTROL_CTRL_HANDLE_RENDERING_H
/**
 * Control handle rendering/caching.
 */
/*
 * Authors:
 *   Sanidhya Singh
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdint>
#include <memory>
#include <compare>

#include "canvas-item-enums.h"

namespace Cairo { class ImageSurface; }

namespace Inkscape::Handles {

struct RenderParams
{
    CanvasItemCtrlShape shape;
    uint32_t fill;
    uint32_t stroke;
    uint32_t outline;
    float stroke_width;
    float outline_width;
    float size;
    double angle;
    int device_scale;
    int size_parity; // -1 - disabled, 0, 1 desired parity in physical pixels

    auto operator<=>(RenderParams const &) const = default;
};

std::shared_ptr<Cairo::ImageSurface const> draw(RenderParams const &params);

} // namespace Inkscape::Handles

template <> struct std::hash<Inkscape::Handles::RenderParams>
{
    size_t operator()(Inkscape::Handles::RenderParams const &tuple) const;
};

#endif // INKSCAPE_DISPLAY_CONTROL_CTRL_HANDLE_RENDERING_H

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
