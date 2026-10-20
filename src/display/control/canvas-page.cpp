// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape pages implementation
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-page.h"
#include "canvas-item-rect.h"
#include "canvas-item-text.h"
#include "colors/utils.h"

namespace Inkscape {

Color const CanvasPage::DEFAULT_BACKGROUND_COLOR{0xffffffff};
Color const CanvasPage::DEFAULT_BORDER_COLOR{0x0000001a};
Color const CanvasPage::DEFAULT_CANVAS_COLOR{0xffffffff};
Color const CanvasPage::DEFAULT_MARGIN_COLOR{0x1699d751};
Color const CanvasPage::DEFAULT_BLEED_COLOR{0xbe310e31};
Color const CanvasPage::DEFAULT_SHADOW_COLOR{0x000000a0};
Color const CanvasPage::DEFAULT_SHADOW_COLOR_LIGHT{0xffffffa0};

CanvasPage::CanvasPage()
    : _background_color(DEFAULT_BACKGROUND_COLOR)
    , _border_color(DEFAULT_BORDER_COLOR)
    , _canvas_color(DEFAULT_CANVAS_COLOR)
    , _margin_color(DEFAULT_MARGIN_COLOR)
    , _bleed_color(DEFAULT_BLEED_COLOR)
    , _shadow_color(DEFAULT_SHADOW_COLOR) {};
CanvasPage::~CanvasPage() = default;

/**
 * Add the page canvas to the given canvas item groups (canvas view is implicit)
 */
void CanvasPage::add(Geom::Rect size, CanvasItemGroup *background_group, CanvasItemGroup *border_group)
{
    // Foreground 'border'
    if (auto item = new CanvasItemRect(border_group, size)) {
        item->set_name("foreground");
        item->set_is_page(true);
        item->set_pixel_alignment(RectLineAlignment::Outside);
        canvas_items.emplace_back(item);
    }

    // Background rectangle 'fill'
    if (auto item = new CanvasItemRect(background_group, size)) {
        item->set_name("background");
        item->set_is_page(true);
        item->set_dashed(false);
        item->set_inverted(false);
        item->set_stroke(0x00000000);
        item->set_pixel_alignment(RectLineAlignment::Outside);
        item->set_infill_shift(true);
        canvas_items.emplace_back(item);
    }

    if (auto item = new CanvasItemRect(border_group, size)) {
        item->set_name("margin");
        item->set_dashed(false);
        item->set_inverted(false);
        item->set_stroke(_margin_color.toRGBA());
        canvas_items.emplace_back(item);
    }

    if (auto item = new CanvasItemRect(border_group, size)) {
        item->set_name("bleed");
        item->set_dashed(false);
        item->set_inverted(false);
        item->set_stroke(_bleed_color.toRGBA());
        canvas_items.emplace_back(item);
    }

    if (auto label = new CanvasItemText(border_group, Geom::Point(0, 0), "{Page Label}")) {
        canvas_items.emplace_back(label);
    }
}
/**
 * Hide the page in the given canvas widget.
 */
void CanvasPage::remove(UI::Widget::Canvas *canvas)
{
    g_assert(canvas != nullptr);
    for (auto it = canvas_items.begin(); it != canvas_items.end();) {
        if (canvas == (*it)->get_canvas()) {
            it = canvas_items.erase(it);
        } else {
            ++it;
        }
    }
}

void CanvasPage::show()
{
    for (auto &item : canvas_items) {
        item->set_visible(true);
    }
}

void CanvasPage::hide()
{
    for (auto &item : canvas_items) {
        item->set_visible(false);
    }
}

void CanvasPage::set_guides_visible(bool show) {
    for (auto& item: canvas_items) {
        if (item->get_name() == "margin" || item->get_name() == "bleed") {
            item->set_visible(show);
        }
    }
}

/**
 * Update the visual representation of a page on screen.
 *
 * @param size - The size of the page in desktop units
 * @param txt - An optional label for the page
 * @param outline - Disable normal rendering and show as an outline.
 */
void CanvasPage::update(Geom::Rect size, Geom::OptRect margin, Geom::OptRect bleed, const char *txt, bool outline, bool is_yaxisdown)
{
    // Put these in the preferences?
    bool border_on_top = _border_on_top;
    guint32 shadow_color = _shadow_color.toRGBA();
    guint32 select_color = 0x000000cc;
    guint32 border_color = _border_color.toRGBA();
    guint32 margin_color = _margin_color.toRGBA();
    guint32 bleed_color = _bleed_color.toRGBA();

    // This is used when showing the viewport as *not a page* it's mostly
    // never used as the first page is normally the viewport too.
    if (outline) {
        border_on_top = false;
        _shadow_size = 0;
        border_color = select_color;
    }

    for (auto &item : canvas_items) {
        if (auto rect = dynamic_cast<CanvasItemRect *>(item.get())) {
            if (rect->get_name() == "margin") {
                rect->set_stroke(margin_color);
                bool vis = margin && *margin != size;
                rect->set_visible(vis);
                if (vis) {
                    rect->set_rect(*margin);
                }
                continue;
            }
            if (rect->get_name() == "bleed") {
                rect->set_stroke(bleed_color);
                bool vis = bleed && *bleed != size;
                rect->set_visible(vis);
                if (vis) {
                    rect->set_rect(*bleed);
                }
                continue;
            }
            // background or foreground, other cases handled above
            rect->set_rect(size);

            bool is_foreground = (rect->get_name() == "foreground");
            // This will put the border on the background OR foreground layer as needed.
            if (is_foreground == border_on_top) {
                rect->set_visible(true);
                rect->set_stroke(is_selected ? select_color : border_color);
            } else {
                rect->set_visible(false);
                rect->set_stroke(0x0);
            }
            // This undoes the hide for the background rect, and additionally gives it a fill and shadow.
            if (!is_foreground) {
                rect->set_visible(true);
/*
                if (_checkerboard) {
                    // draw checkerboard pattern, ignore alpha (background color doesn't support it)
                    rect->set_background_checkerboard(_background_color, false);
                }
                else {
                    // Background color does not support transparency; draw opaque pages
                    rect->set_background(_background_color | 0xff);
                }
*/
                rect->set_fill(_background_color.toRGBA());
                rect->set_shadow(shadow_color, _shadow_size, true);
            } else {
                rect->set_fill(0x0);
                rect->set_shadow(0x0, 0);
            }
        } else if (auto label = dynamic_cast<CanvasItemText *>(item.get())) {
            _updateTextItem(label, size, txt ? txt : "", is_yaxisdown);
        }
    }
}

/**
 * Update the page's textual label.
 */
void CanvasPage::_updateTextItem(CanvasItemText *label, Geom::Rect page, std::string txt, bool is_yaxisdown)
{
    // Default style for the label
    int fontsize = 10.0;
    uint32_t foreground = 0xffffffff;
    uint32_t background = 0x00000099;
    uint32_t selected = 0x0e5bf199;
    Geom::Point anchor(0.0, 1.0);
    Geom::Point coord = page.corner(0);
    double radius = 0.2;

    // Change the colors for whiter/lighter backgrounds
    if (Colors::get_perceptual_lightness(_canvas_color) < 0.5) {
        foreground = 0x000000ff;
        background = 0xffffff99;
        selected = 0x50afe7ff;
    }

    if (_label_style == "below") {
        radius = 1.0;
        fontsize = 14.0;
        anchor = Geom::Point(0.5, -0.2);
        coord = Geom::Point(page.midpoint().x(), is_yaxisdown ? page.bottom() : page.top());

        if (!txt.empty()) {
            std::string bullet = is_selected ? " \u2022 " : "   ";
            txt = bullet + txt + bullet;
        }
    }

    label->set_fontsize(fontsize);
    label->set_fill(foreground);
    label->set_background(is_selected ? selected : background);
    label->set_bg_radius(radius);
    label->set_anchor(anchor);
    label->set_coord(coord);
    label->set_visible(!txt.empty());
    label->set_text(std::move(txt));
    label->set_border(4.0);
}

bool CanvasPage::setOnTop(bool on_top)
{
    if (on_top != _border_on_top) {
        _border_on_top = on_top;
        return true;
    }
    return false;
}

bool CanvasPage::setShadow(int shadow)
{
    if (_shadow_size != shadow) {
        _shadow_size = shadow;
        return true;
    }
    return false;
}

bool CanvasPage::setPageColor(Color const &border, Color const &bg, Color const &canvas, Color const &margin,
                              Color const &bleed, Color const &shadow)
{
    if (std::tie(border, bg, canvas, margin, bleed, shadow) !=
        std::tie(_border_color, _background_color, _canvas_color, _margin_color, _bleed_color, _shadow_color)) {
        _border_color = border;
        _background_color = bg;
        _canvas_color = canvas;
        _margin_color = margin;
        _bleed_color = bleed;
        _shadow_color = shadow;
        return true;
    }
    return false;
}

bool CanvasPage::setLabelStyle(const std::string &style)
{
    if (style != _label_style) {
        _label_style = style;
        return true;
    }
    return false;
}

} // namespace Inkscape

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
