// SPDX-License-Identifier: GPL-2.0-or-later

#include "handle-preview.h"
#include "display/control/canvas-item-ctrl.h"
#include "display/control/canvas-item-group.h"
#include "display/control/canvas-item.h"
#include "ui/widget/canvas.h"

namespace Inkscape {

Cairo::RefPtr<Cairo::ImageSurface> draw_handles_preview(int device_scale) {
    constexpr int step = 34; // selected to make handles fit at highest size
    constexpr auto types = std::to_array({
        CANVAS_ITEM_CTRL_TYPE_ADJ_SKEW,
        CANVAS_ITEM_CTRL_TYPE_ADJ_ROTATE,
        CANVAS_ITEM_CTRL_TYPE_POINTER, // pointy, triangular handle
        CANVAS_ITEM_CTRL_TYPE_MARKER, // X mark
        CANVAS_ITEM_CTRL_TYPE_NODE_AUTO, 
        CANVAS_ITEM_CTRL_TYPE_NODE_CUSP,
        CANVAS_ITEM_CTRL_TYPE_NODE_SMOOTH,
    });
    auto h = static_cast<int>(1.5 * step);
    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, (types.size() + 1) * step * device_scale, h * device_scale);
    cairo_surface_set_device_scale(surface->cobj(), device_scale, device_scale);
    auto buf = CanvasItemBuffer{
        .rect = Geom::IntRect(0, 0, surface->get_width(), surface->get_height()),
        .device_scale = device_scale,
        .cr = Cairo::Context::create(surface),
        .outline_pass = false
    };

    auto canvas = std::make_unique<UI::Widget::Canvas>();
    canvas->set_visible();
    auto root = canvas->get_canvas_item_root();

    int i = 1;
    for (auto type : types) {
        auto position = Geom::IntPoint{step * i++, h / 2};
        auto handle = new CanvasItemCtrl(root, type, position);

        if (type == CANVAS_ITEM_CTRL_TYPE_ADJ_SKEW) handle->set_hover();
        if (type == CANVAS_ITEM_CTRL_TYPE_NODE_CUSP || type == CANVAS_ITEM_CTRL_TYPE_NODE_SMOOTH) handle->set_selected();
        if (type == CANVAS_ITEM_CTRL_TYPE_POINTER) handle->set_angle(M_PI);

        handle->set_size(Inkscape::HandleSize::NORMAL);
    }

    root->update(true);
    root->render(buf);

    return surface;
}

} // namespace
