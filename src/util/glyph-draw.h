// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 8/12/25.
//

#ifndef GLYPH_DRAW_H
#define GLYPH_DRAW_H

#include <gdkmm/rgba.h>
#include <cairomm/cairomm.h>

#include "2geom/int-rect.h"

class FontInstance;

namespace Inkscape::Util {

struct draw_glyph_params {
    // font to use
    FontInstance* font = nullptr;
    // draw at requested size (or 0 for auto-fit)
    double font_size = 0;
    // index of the glyph to draw
    uint32_t glyph_index = 0;
    // where to draw to
    const Cairo::RefPtr<Cairo::Context> ctx;
    // available area
    Geom::IntRect rect;
    // colors to use
    Gdk::RGBA glyph_color;
    Gdk::RGBA line_color;
    Gdk::RGBA background_color;
    // draw baseline, ascender and descender lines
    bool draw_metrics = false;
    // fill background with color
    bool draw_background = false;
};

// Draw requested glyph
void draw_glyph(const draw_glyph_params& params);

} // namespace

#endif //GLYPH_DRAW_H
