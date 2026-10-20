// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ralf Stephan <ralf@ark.in-berlin.de>
 *   Michael Kowalski
 *
 * Copyright (C) 2001-2005 Authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLOR_PREVIEW_H
#define SEEN_COLOR_PREVIEW_H

#include <gtkmm/drawingarea.h>
#include <cstdint>

namespace Gtk {
class Builder;
}

namespace Cairo {
class Context;
} // namespace Cairo

namespace Inkscape::UI::Widget {

/**
 * A color preview widget, used within a picker button and style indicator.
 * It can show RGBA color or Cairo pattern.
 *
 * RGBA colors are split in half to show solid color and transparency, if any.
 * RGBA colors are also manipulated to reduce intensity if the color preview is disabled.
 *
 * Patterns are shown "as is" on top of the checkerboard.
 * There's no separate "disabled" look for patterns.
 *
 * Outlined style can be used to surround a color patch with a contrasting border.
 * Border is dark-theme-aware.
 *
 * Indicators can be used to distinguish ad-hoc colors from swatches and spot colors.
 */
class ColorPreview final : public Gtk::DrawingArea {
public:
    ColorPreview(std::uint32_t rgba = 0);
    ColorPreview(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    // set preview color RGBA with opacity (alpha)
    void setRgba32(std::uint32_t rgba);
    // set arbitrary pattern-based preview
    void setPattern(Cairo::RefPtr<Cairo::Pattern> pattern);
    // simple color patch vs outlined color patch
    enum Style { Simple, Outlined };
    void setStyle(Style style);
    // add indicator on top of the preview: swatch or spot color
    enum Indicator { None = 0, Swatch = 1, SpotColor = 2, LinearGradient = 4, RadialGradient = 8 };
    void setIndicator(Indicator indicator);
    // add frame for a 'Simple' preview
    void set_frame(bool frame);
    // set border radius; -1 to auto
    void set_border_radius(int radius);
    // adjust size of checkerboard tiles
    void set_checkerboard_tile_size(unsigned size);
    // Update the fill indicator, showing this widget is the fill of the current item.
    void set_fill(bool on);
    // Update the stroke indicator, showing this widget is the stroke of the current item.
    void set_stroke(bool on);

    struct GradientStops {
        double offset;
        double red, green, blue, alpha;
    };
    // Set a linear gradient to show in the color preview.
    void set_gradient(std::vector<GradientStops> stops);
private:
    void construct();
    std::uint32_t _rgba; // requested RGBA color, used if there is no pattern given
    Cairo::RefPtr<Cairo::Pattern> _pattern; // pattern to show, if provided
    Style _style = Simple;
    Indicator _indicator = None;
    int _radius = -1;
    bool _frame = false;
    bool _is_fill = false;
    bool _is_stroke = false;
    void draw_func(Cairo::RefPtr<Cairo::Context> const &cr, int width, int height);
    int _checkerboard_tile_size = 6;
    std::vector<GradientStops> _gradient;
    Cairo::RefPtr<Cairo::LinearGradient> _linear_gradient;
    int _linear_gradient_width = 0;
    void create_gradient_preview(int width);
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_COLOR_PREVIEW_H

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
