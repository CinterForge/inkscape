// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_SNAPSHOT_UTILS_H
#define INKSCAPE_UI_SNAPSHOT_UTILS_H

#include <graphene.h>
#include <gsk/gsk.h>
#include <gdk/gdk.h>
#include <2geom/generic-rect.h>
#include <2geom/point.h>

#include "colors/color.h"
#include "util/delete-with.h"

class SPGradient;

namespace Inkscape::UI {

constexpr graphene_point_t to_gtk(Geom::Point const &point)
{
    float const x = point.x();
    float const y = point.y();
    return GRAPHENE_POINT_INIT(x, y);
}

constexpr graphene_point_t to_gtk(Geom::IntPoint const &point)
{
    float const x = point.x();
    float const y = point.y();
    return GRAPHENE_POINT_INIT(x, y);
}

template <typename T>
constexpr graphene_rect_t to_gtk(Geom::GenericRect<T> const &rect)
{
    float const x = rect.left();
    float const y = rect.top();
    float const w = rect.width();
    float const h = rect.height();
    return GRAPHENE_RECT_INIT(x, y, w, h);
}

constexpr GdkRGBA to_gtk(uint32_t rgba)
{
    return {
        .red   = ((rgba & 0xff000000) >> 24) / 255.0f,
        .green = ((rgba & 0x00ff0000) >> 16) / 255.0f,
        .blue  = ((rgba & 0x0000ff00) >>  8) / 255.0f,
        .alpha = ((rgba & 0x000000ff)      ) / 255.0f
    };
}

inline GdkRGBA to_gtk(Colors::Color const &colour)
{
    return to_gtk(colour.toRGBA());
}

/// Class which holds an object and decays to a raw pointer to it. Used for passing arguments to GTK functions.
template <typename T>
struct Dangler
{
    T content;
    operator T const *() const { return &content; }
};

template <typename T>
Dangler(T &&t) -> Dangler<T>;

/// Wrap a C++ type to make it suitable for passing to a GTK function which expects a pointer to a C type.
template <typename T>
inline auto pass_in(T const &t)
{
    return Dangler{to_gtk(t)};
}

/// Smart pointer for owning render nodes.
using RenderNodePtr = std::unique_ptr<GskRenderNode, Util::Deleter<gsk_render_node_unref>>;

/// Create a render node of checkerboard pattern filling the given rectangle.
RenderNodePtr create_checkerboard_node(Geom::IntRect const &rect,
                                       int size = 6,
                                       GdkRGBA const &col1 = to_gtk(0xc4c4c4ff),
                                       GdkRGBA const &col2 = to_gtk(0xb0b0b0ff));

/// Return a preview of a SPGradient as a vector of colour stops.
std::vector<GskColorStop> createPreviewStops(SPGradient *gradient);

} // namespace Inkscape::UI

#endif // INKSCAPE_UI_SNAPSHOT_UTILS_H
