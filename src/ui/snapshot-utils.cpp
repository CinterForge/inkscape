// SPDX-License-Identifier: GPL-2.0-or-later
#include "snapshot-utils.h"

#include <gtk/gtk.h>
#include <2geom/int-rect.h>

#include "object/sp-gradient.h"

namespace Inkscape::UI {

RenderNodePtr create_checkerboard_node(Geom::IntRect const &rect, int size, GdkRGBA const &col1, GdkRGBA const &col2)
{
    auto child = gtk_snapshot_new();
    gtk_snapshot_append_color(child, &col1, pass_in(Geom::IntRect::from_xywh(0, 0, 2 * size, 2 * size)));
    gtk_snapshot_append_color(child, &col2, pass_in(Geom::IntRect::from_xywh(0, 0, size, size)));
    gtk_snapshot_append_color(child, &col2, pass_in(Geom::IntRect::from_xywh(size, size, size, size)));
    auto node = gtk_snapshot_free_to_node(child);
    auto repeat = gsk_repeat_node_new(pass_in(rect), node, pass_in(Geom::IntRect::from_xywh(0, 0, 2 * size, 2 * size)));
    gsk_render_node_unref(node);
    return RenderNodePtr{repeat};
}

std::vector<GskColorStop> createPreviewStops(SPGradient *gradient)
{
    std::vector<GskColorStop> result;

    gradient->forEachPreviewPatternStop([&] (double offset, Colors::Color const &col) {
        result.push_back(GskColorStop{
            .offset = static_cast<float>(offset),
            .color = to_gtk(col)
        });
    });

    return result;
}

} // namespace Inkscape::UI
