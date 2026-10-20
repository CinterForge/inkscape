// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cairo display backend.
 * Copyright (C) 2022 PBS <pbs3141@gmail.com>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_CANVAS_CAIROGRAPHICS_H
#define INKSCAPE_UI_WIDGET_CANVAS_CAIROGRAPHICS_H

#include "graphics.h"

namespace Inkscape::UI::Widget {

struct CairoFragment
{
    Cairo::RefPtr<Cairo::ImageSurface> surface;
    Cairo::RefPtr<Cairo::ImageSurface> outline_surface;
};

class CairoGraphics : public Graphics
{
public:
    CairoGraphics(Prefs const &prefs, Stores const &stores, PageInfo const &pi);

    void set_scale_factor(int scale) override { scale_factor = scale; }
    void set_outlines_enabled(bool) override;
    void set_background_in_stores(bool enabled) override { background_in_stores = enabled; }
    void set_colours(uint32_t p, uint32_t d, uint32_t b) override { page = p; desk = d; shadow = b; }
    void set_shadow_size(float size) override {};
    
    void recreate_store(Geom::IntPoint const &dimensions) override;
    void shift_store(Fragment const &dest) override;
    void swap_stores() override;
    void fast_snapshot_combine() override;
    void snapshot_combine(Fragment const &dest) override;
    void invalidate_snapshot() override {}

    bool is_opengl() const override { return false; }
    void invalidated_glstate() override {}

    Cairo::RefPtr<Cairo::ImageSurface> request_tile_surface(Geom::IntRect const &rect, bool nogl) override;
    void draw_tile(Fragment const &fragment, Cairo::RefPtr<Cairo::ImageSurface> surface, Cairo::RefPtr<Cairo::ImageSurface> outline_surface) override;
    void junk_tile_surface(Cairo::RefPtr<Cairo::ImageSurface> surface) override {}

    void paint_widget(Fragment const &view, PaintArgs const &args, Cairo::RefPtr<Cairo::Context> const &cr) override;

private:
    // Drawn content.
    CairoFragment store, snapshot;

    // Dependency objects in canvas.
    Prefs const &prefs;
    Stores const &stores;
    PageInfo const &pi;

    // Backend-agnostic state.
    int scale_factor = 1;
    bool outlines_enabled = false;
    bool background_in_stores = false;
    uint32_t page, desk, shadow;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_CAIROGRAPHICS_H

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
