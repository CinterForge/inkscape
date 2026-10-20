// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Rewrite of GridCanvasItem.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "canvas-item-grid.h"

#include <cmath>
#include <cairomm/enums.h>
#include <2geom/line.h>

#include "colors/color.h"
#include "helper/geom.h"
#include "helper/mathfns.h"

enum Dim3
{
    X,
    Y,
    Z
};

static int calculate_scaling_factor(double length, int major)
{
    int multiply = 1;
    int step = std::max(major, 1);
    int watchdog = 0;

    while (length * multiply < 8.0 && watchdog < 100) {
        multiply *= step;
        // First pass, go up to the major line spacing, then keep increasing by two.
        step = 2;
        watchdog++;
    }

    return multiply;
}

namespace Inkscape {

/**
 * Create a null control grid.
 */
CanvasItemGrid::CanvasItemGrid(CanvasItemGroup *group)
    : CanvasItem(group)
    , _origin(0, 0)
    , _spacing(1, 1)
    , _minor_color(0x0)
    , _major_color(0x0)
    , _major_line_interval(5)
    , _dotted(false)
{
    _no_emp_when_zoomed_out = Preferences::get()->getBool("/options/grids/no_emphasize_when_zoomedout");
    _pref_tracker =
        Preferences::PreferencesObserver::create("/options/grids/no_emphasize_when_zoomedout",
                                                 [this](auto &entry) { set_no_emp_when_zoomed_out(entry.getBool()); });

    request_update();
}

/**
 * Returns true if point p (in canvas units) is within tolerance (canvas units) distance of grid.
 */
bool CanvasItemGrid::contains(Geom::Point const &p, double tolerance)
{
    return false; // We're not pickable!
}

// Find the signed distance of a point to a line. The distance is negative if
// the point lies to the left of the line considering the line's versor.
static double signed_distance(Geom::Point const &point, Geom::Line const &line)
{
    return Geom::cross(point - line.initialPoint(), line.versor());
}

void CanvasItemGrid::set_origin(Geom::Point const &point)
{
    defer([=, this] {
        if (_origin == point) {
            return;
        }
        _origin = point;
        request_update();
    });
}

void CanvasItemGrid::set_major_color(uint32_t color)
{
    defer([=, this] {
        if (_major_color == color) {
            return;
        }
        _major_color = color;
        request_update();
    });
}

void CanvasItemGrid::set_minor_color(uint32_t color)
{
    defer([=, this] {
        if (_minor_color == color) {
            return;
        }
        _minor_color = color;
        request_update();
    });
}

void CanvasItemGrid::set_dotted(bool dotted)
{
    defer([=, this] {
        if (_dotted == dotted) {
            return;
        }
        _dotted = dotted;
        request_update();
    });
}

void CanvasItemGrid::set_spacing(Geom::Point const &point)
{
    defer([=, this] {
        if (_spacing == point) {
            return;
        }
        _spacing = point;
        request_update();
    });
}

void CanvasItemGrid::set_major_line_interval(int n)
{
    if (n < 1) {
        return;
    }
    defer([=, this] {
        if (_major_line_interval == n) {
            return;
        }
        _major_line_interval = n;
        request_update();
    });
}

void CanvasItemGrid::set_no_emp_when_zoomed_out(bool noemp)
{
    if (_no_emp_when_zoomed_out != noemp) {
        _no_emp_when_zoomed_out = noemp;
        request_redraw();
    }
}

static std::pair<int, int> calculate_line_range(Geom::IntRect const &screen_rect, Geom::Point origin,
                                                Geom::Point direction, Geom::Point normal)
{
    // Find the minimum and maximum distances of the buffer corners from axis.
    double min = Geom::infinity();
    double max = -Geom::infinity();
    for (int c = 0; c < 4; ++c) {
        // We need signed distance... lib2geom offers only positive distance.
        // And the end goal is line index instead of real distance. As explained further things cancel out
        // thus allowing to skip some redundant math.
        double distance = Geom::dot(screen_rect.corner(c) - origin, normal);

        min = std::min(min, distance);
        max = std::max(max, distance);
    }

    // distance/spacing = |a|sin(angle) / |norm| = |a||norm|sin(angle) / |norm|^2 = dot(a, norm) / |norm|^2
    double spacing_sq = normal.lengthSq();
    int start = std::ceil(min / spacing_sq);
    int stop = std::ceil(max / spacing_sq);

    return {start, stop};
}

static std::optional<Geom::Line> get_grid_line(Geom::IntRect const &screen_rect, Geom::Point origin,
                                               Geom::Point direction, Geom::Point normal, int index, int line_thickness,
                                               int scale_factor)
{
    auto grid_line = Geom::Line::from_origin_and_vector(origin + index * normal, direction);
    auto segment = grid_line.clip(screen_rect);
    if (!segment.has_value()) {
        return {};
    }
    Geom::Point x[2] = {segment->initialPoint(), segment->finalPoint()};
    if (line_thickness >= 0) {
        x[0] = CanvasItem::align_to_pixels05(x[0], line_thickness, scale_factor);
        x[1] = CanvasItem::align_to_pixels05(x[1], line_thickness, scale_factor);
    }
    if (Geom::dot(x[1] - x[0], direction) < 0) {
        // keep the direction consistent with original direction vector
        std::swap(x[0], x[1]);
    }
    return Geom::Line(x[0], x[1]);
}

static void add_line(Inkscape::CanvasItemBuffer &buf, Geom::Line const &line)
{
    buf.cr->move_to(line.origin().x(), line.origin().y());
    buf.cr->line_to(line.finalPoint().x(), line.finalPoint().y());
}

/** ====== Rectangular Grid  ====== **/

CanvasItemGridXY::CanvasItemGridXY(Inkscape::CanvasItemGroup *group)
    : CanvasItemGrid(group)
{
    _name = "CanvasItemGridXY";
}

void CanvasItemGridXY::_update(bool)
{
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());

    // Queue redraw of grid area
    ow = _origin * affine();
    sw[0] = Geom::Point(_spacing[0], 0) * affine().withoutTranslation();
    sw[1] = Geom::Point(0, _spacing[1]) * affine().withoutTranslation();

    // Find suitable grid spacing for display
    for (int dim : {0, 1}) {
        int const scaling_factor = calculate_scaling_factor(sw[dim].length(), _major_line_interval);
        sw[dim] *= scaling_factor;
        scaled[dim] = scaling_factor > 1;
    }

    request_redraw();
}

void CanvasItemGridXY::_render(Inkscape::CanvasItemBuffer &buf) const
{
    // no_emphasize_when_zoomedout determines color (minor or major) when only major grid lines/dots shown.
    uint32_t empcolor = ((scaled[Geom::X] || scaled[Geom::Y]) && _no_emp_when_zoomed_out) ? _minor_color : _major_color;
    uint32_t color = _minor_color;

    buf.cr->save();
    buf.cr->translate(-buf.rect.left(), -buf.rect.top());
    buf.cr->set_line_width(1.0);
    buf.cr->set_line_cap(Cairo::Context::LineCap::SQUARE);

    int physical_thickness = buf.device_scale * 1;

    // Add a 2px margin to the buffer rectangle to avoid missing intersections (in case of rounding errors, and due to
    // adding 0.5 by pixel alignment code)
    auto const buf_rect_with_margin = expandedBy(buf.rect, 2);

    for (int dim : {0, 1}) {
        int const nrm = dim ^ 0x1;

        // Construct an axis line through origin with direction normal to grid spacing.
        Geom::Point dir = sw[dim];
        Geom::Point normal = sw[nrm];
        Geom::Line orth = Geom::Line::from_origin_and_vector(ow, normal);

        double dash = dir.length(); // Total length of dash pattern.

        auto [start, stop] = calculate_line_range(buf_rect_with_margin, ow, dir, normal);

        // Dash alignment with pixel grid for the purpose of aligning with center of orthogonal lines
        // and to avoid blurry half pixels at the ends.
        double dash_pixel_center_offset = 0;
        if (physical_thickness & 1) {
            dash_pixel_center_offset = Geom::dot(dir.normalized(), Geom::Point(0.5, 0.5) / buf.device_scale);
        }

        std::vector<double> min_dashes = {1.0, dash - 1.0};
        std::vector<double> maj_dashes = {3.0, dash - 3.0};
        double min_dash_offset = -0.5 + dash_pixel_center_offset;
        double maj_dash_offset = -1.5 + dash_pixel_center_offset;

        // Loop over grid lines that intersected buf rectangle.
        for (int j = start; j < stop; ++j) {
            auto line = get_grid_line(buf_rect_with_margin, ow, dir, normal, j, physical_thickness, buf.device_scale);

            if (!line.has_value()) {
                std::cerr << "CanvasItemGridXY::render: Grid line doesn't intersect!" << std::endl;
                continue;
            }
            // Set up line.
            add_line(buf, line.value());

            // Determine whether to draw with the emphasis color.
            bool const noemp = !scaled[dim] && j % _major_line_interval != 0;
            uint32_t col = noemp ? color : empcolor;

            // Set dash pattern and color.
            if (_dotted) {
                // alpha needs to be larger than in the line case to maintain a similar
                // visual impact but setting it to the maximal value makes the dots
                // dominant in some cases. Solution, increase the alpha by a factor of
                // 4. This then allows some user adjustment.
                uint32_t coldot = (col & 0xff) << 2;
                if (coldot > 0xff) {
                    coldot = 0xff;
                }
                coldot += (col & 0xffffff00);
                col = coldot;

                // Dash pattern must use spacing  from orthogonal direction.
                // Offset is to center dash on orthogonal lines.
                double offset = std::fmod(signed_distance(line->initialPoint(), orth), dash);
                if (Geom::cross(dir, normal) > 0) {
                    offset = -offset;
                }

                if (noemp) {
                    // Minor lines
                    offset += min_dash_offset;
                    buf.cr->set_dash(min_dashes, -offset);
                } else {
                    // Major lines
                    offset += maj_dash_offset;
                    buf.cr->set_dash(maj_dashes, -offset);
                }
                buf.cr->set_line_cap(Cairo::Context::LineCap::BUTT);
            }
            buf.cr->set_source_rgba(SP_RGBA32_R_F(col), SP_RGBA32_G_F(col), SP_RGBA32_B_F(col), SP_RGBA32_A_F(col));

            buf.cr->stroke();
        }
    }

    buf.cr->restore();
}

/** ========= Axonometric Grids ======== */

/*
 * Current limits are: one axis (y-axis) is always either vertical or calculated from
 * the other two axes to form an intersection with the generated grid. The other two
 * axes are bound to a certain range of angles. The z-axis always has an angle smaller
 * than 90 degrees (measured from horizontal, 0 degrees being a line extending to the
 * right). The x-axis will always have an angle between 0 and 90 degrees.
 */
CanvasItemGridAxonom::CanvasItemGridAxonom(Inkscape::CanvasItemGroup *group)
    : CanvasItemGrid(group)
{
    _name = "CanvasItemGridAxonom";
    angle_y_vertical = true;

    angle_deg[X] = 30.0;
    angle_deg[Y] = 30.0;
    angle_deg[Z] = 0.0;

    angle_rad[X] = Geom::rad_from_deg(angle_deg[X]);
    angle_rad[Y] = Geom::rad_from_deg(angle_deg[Y]);
    angle_rad[Z] = Geom::rad_from_deg(angle_deg[Z]);
}

void CanvasItemGridAxonom::_update(bool)
{
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());

    ow = _origin * affine();

    double tan_x = std::tan(angle_rad[X]);
    double tan_z = std::tan(angle_rad[Z]);

    double len = _spacing.y();
    double len_x = len * std::cos(angle_rad[X]);
    double len_z = len * std::cos(angle_rad[Z]);

    direction[X] = Geom::Point(1.0, tan_x);
    direction[Z] = Geom::Point(1.0, -tan_z);
    normal[X] = rot90(direction[X]).normalized() * len_x;
    normal[Z] = rot90(direction[Z]).normalized() * len_z;

    if (angle_y_vertical) {
        direction[Y] = Geom::Point(0, 1);
        normal[Y] = Geom::Point(len / (tan_x + tan_z), 0);
    } else {
        double len_y = len * std::cos(angle_rad[Y]);
        double tan_y = std::tan(angle_rad[Y]);
        direction[Y] = Geom::Point(1.0, tan_y);
        normal[Y] = rot90(direction[Y]).normalized() * len_y * 0.5;
    }

    auto scaled_y = len * affine().descrim();
    int const scaling_factor = calculate_scaling_factor(scaled_y, _major_line_interval);
    scaled = scaling_factor > 1;

    auto vector_transform = affine().withoutTranslation() * Geom::Scale(scaling_factor);
    for (auto axis : {X, Y, Z}) {
        direction[axis] *= vector_transform;
        normal[axis] *= vector_transform;
    }

    if (_major_line_interval == 0) {
        scaled = true;
    }

    request_redraw();
}

// Shared update function for when angles change, to recalculate the derived Y angle if necessary and queue redraw.
void CanvasItemGridAxonom::update_derived_angle_y()
{
    if (!angle_y_vertical) {
        angle_rad[Y] = Util::derived_angle_y(angle_rad[X], angle_rad[Z]);
        angle_deg[Y] = Geom::deg_from_rad(angle_rad[Y]);
    }
    request_update();
}

void CanvasItemGridAxonom::set_angle_y_vertical(bool vertical)
{
    defer([=, this] {
        if (angle_y_vertical == vertical) {
            return;
        }

        angle_y_vertical = vertical;
        update_derived_angle_y();
    });
}

// expects value given to be in degrees
void CanvasItemGridAxonom::set_angle_x(double deg)
{
    defer([=, this] {
        angle_deg[X] = std::clamp(deg, 0.0, 89.0); // setting to 90 and values close cause extreme slowdowns
        angle_rad[X] = Geom::rad_from_deg(angle_deg[X]);
        update_derived_angle_y();
    });
}

// expects value given to be in degrees
void CanvasItemGridAxonom::set_angle_z(double deg)
{
    defer([=, this] {
        angle_deg[Z] = std::clamp(deg, 0.0, 89.0); // setting to 90 and values close cause extreme slowdowns
        angle_rad[Z] = Geom::rad_from_deg(angle_deg[Z]);
        update_derived_angle_y();
    });
}

/**
 * This function calls Cairo to render a line on a particular canvas buffer.
 * Coordinates are interpreted as SCREENcoordinates
 */
void CanvasItemGridAxonom::_render(Inkscape::CanvasItemBuffer &buf) const
{
    // Set correct coloring, depending preference (when zoomed out, always major coloring or minor coloring)
    uint32_t empcolor = (scaled && _no_emp_when_zoomed_out) ? _minor_color : _major_color;
    uint32_t color = _minor_color;

    buf.cr->save();
    buf.cr->translate(-buf.rect.left(), -buf.rect.top());
    buf.cr->set_line_width(1.0);
    buf.cr->set_line_cap(Cairo::Context::LineCap::SQUARE);
    int const phsyical_thickness = 1 * buf.device_scale;

    // Add a 2px margin to the buffer rectangle to avoid missing intersections (in case of rounding errors, and due to
    // adding 0.5 by pixel alignment code)
    auto const buf_rect_with_margin = expandedBy(buf.rect, 2);

    // render the three separate line groups representing the main-axes
    for (Dim3 axis : {X, Y, Z}) {
        auto dir = direction[axis];
        auto norm = normal[axis];
        if (!dir.isFinite() || !norm.isFinite()) {
            continue;
        }
        auto [start, stop] = calculate_line_range(buf_rect_with_margin, ow, dir, norm);
        for (auto j = start; j < stop; j++) {
            auto line = get_grid_line(buf_rect_with_margin, ow, dir, norm, j, phsyical_thickness, buf.device_scale);
            if (!line.has_value()) {
                continue;
            }

            add_line(buf, line.value());
            bool const noemp = !scaled && j % _major_line_interval != 0;
            auto rgba = noemp ? color : empcolor;
            buf.cr->set_source_rgba(SP_RGBA32_R_F(rgba), SP_RGBA32_G_F(rgba), SP_RGBA32_B_F(rgba), SP_RGBA32_A_F(rgba));
            buf.cr->stroke();
        }
    }

    buf.cr->restore();
}

CanvasItemGridTiles::CanvasItemGridTiles(Inkscape::CanvasItemGroup *group)
    : CanvasItemGrid(group)
{
    _name = "CanvasItemGridTiles";
}

void CanvasItemGridTiles::set_gap_size(Geom::Point gap_size)
{
    defer([=, this] {
        if (_gap == gap_size) {
            return;
        }
        _gap = gap_size;
        request_update();
    });
}

void CanvasItemGridTiles::set_margin_size(Geom::Point margin_size)
{
    defer([=, this] {
        if (_margin == margin_size) {
            return;
        }
        _margin = margin_size;
        request_update();
    });
}

void CanvasItemGridTiles::_update(bool)
{
    _bounds = Geom::Rect(-Geom::infinity(), -Geom::infinity(), Geom::infinity(), Geom::infinity());

    // Queue redraw of grid area
    _world_origin = _origin * affine();
    auto tile = _spacing;
    auto pitch = tile + _gap;

    auto vector_transform = affine().withoutTranslation();

    _world_pitch[0] = Geom::Point(pitch.x(), 0) * vector_transform;
    _world_pitch[1] = Geom::Point(0, pitch.y()) * vector_transform;

    Geom::Rect rect = Geom::Rect::from_xywh(_gap * 0.5, tile);
    Geom::Rect margin_rect = rect.expandedBy(_margin);
    for (int i = 0; i < 4; i++) {
        _world_corners[i] = rect.corner(i) * vector_transform;
        _world_margin_corners[i] = margin_rect.corner(i) * vector_transform;
    }

    request_redraw();
}

void CanvasItemGridTiles::_render(Inkscape::CanvasItemBuffer &buf) const
{
    // minute tiles bring no real value, they look like noise, skip them
    double const MIN_SIZE_SQ = 3 * 3;
    if (_world_pitch[0].lengthSq() < MIN_SIZE_SQ || _world_pitch[1].lengthSq() < MIN_SIZE_SQ) {
        return;
    }

    buf.cr->save();
    buf.cr->translate(-buf.rect.left(), -buf.rect.top());
    buf.cr->set_line_width(1.0);
    buf.cr->set_line_cap(Cairo::Context::LineCap::BUTT);
    int physical_thickness = 1 * buf.device_scale;

    // Add a 2px margin to the buffer rectangle to avoid missing intersections (in case of rounding errors, and due to
    // adding 0.5 by pixel alignment code)
    auto const buf_rect_with_margin = expandedBy(buf.rect, 2);

    auto range_x = calculate_line_range(buf_rect_with_margin, _world_origin, _world_pitch[1], _world_pitch[0]);
    auto range_y = calculate_line_range(buf_rect_with_margin, _world_origin, _world_pitch[0], _world_pitch[1]);

    auto draw_rectangles = [&](Geom::Point const(&corners)[4], uint32_t color) {
        // Range start is first grid line inside the rect.
        // Since the tile goes between column x and column x+1 need to -1 for seeing the partial tiles.
        for (auto x = range_x.first - 1; x < range_x.second; x++) {
            for (auto y = range_y.first - 1; y < range_y.second; y++) {
                auto grid_corner = _world_origin + x * _world_pitch[0] + y * _world_pitch[1];

                auto aligned_corner = align_to_pixels05(grid_corner + corners[3], physical_thickness, buf.device_scale);
                buf.cr->move_to(aligned_corner.x(), aligned_corner.y());
                for (auto corner : corners) {
                    aligned_corner = align_to_pixels05(grid_corner + corner, physical_thickness, buf.device_scale);
                    buf.cr->line_to(aligned_corner.x(), aligned_corner.y());
                }
            }
        }

        uint32_t rgba = color;
        buf.cr->set_source_rgba(SP_RGBA32_R_F(rgba), SP_RGBA32_G_F(rgba), SP_RGBA32_B_F(rgba), SP_RGBA32_A_F(rgba));
        buf.cr->stroke();
    };

    draw_rectangles(_world_corners, _major_color);

    auto world_margin = _world_corners[0] - _world_margin_corners[0];
    if (world_margin.lengthSq() > 0.25) {
        draw_rectangles(_world_margin_corners, _minor_color);
    }
    buf.cr->restore();
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
