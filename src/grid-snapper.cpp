// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Grid Snapper for Rectangular and Axonometric Grids
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2022 Authors
 * Copyright (C) Johan Engelen 2006-2007 <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "grid-snapper.h"

#include <cmath>

#include "desktop.h"
#include "helper/mathfns.h"
#include "object/sp-grid.h"
#include "object/sp-namedview.h"
#include "preferences.h"

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

// Project a vector onto the given axis.
static auto proj(Geom::Point const &p, int dim)
{
    return dim == 0
         ? Geom::Point(p.x(), 0.0)
         : Geom::Point(0.0, p.y());
}

// Return the unit vector along the given axis.
static auto basis(int dim)
{
    return dim == 0
         ? Geom::Point(1.0, 0.0)
         : Geom::Point(0.0, 1.0);
}

namespace Inkscape {

GridSnapper::GridSnapper(SPGrid const *grid, SnapManager *sm, Geom::Coord const d)
    : LineSnapper(sm, d)
    , _grid(grid)
{
}

/**
 *  \return Snap tolerance (desktop coordinates); depends on current zoom so that it's always the same in screen pixels
 */
Geom::Coord GridSnapper::getSnapperTolerance() const
{
    return _snapmanager->getSnapperTolerance(_snapmanager->snapprefs.getGridTolerance());
}

bool GridSnapper::getSnapperAlwaysSnap(SnapSourceType const &/*source*/) const
{
    return Preferences::get()->getBool("/options/snap/grid/always", false);
}

LineSnapper::LineList GridSnapper::_getSnapLines(Geom::Point const &p) const
{
    if (!_snapmanager->getNamedView() || !ThisSnapperMightSnap()) {
        return {};
    }

    switch (_grid->getType()) {
        case GridType::RECTANGULAR: return get_snap_lines(p, 0);
        case GridType::AXONOMETRIC: return getSnapLinesAxonom(p);
        case GridType::MODULAR: return get_snap_lines(p, 4);
        default: g_assert_not_reached(); return {};
    }
}

void GridSnapper::_addSnappedLine(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance,  SnapSourceType const &source, long source_num, Geom::Point const &normal_to_line, Geom::Point const &point_on_line) const
{
    isr.grid_lines.emplace_back(snapped_point, snapped_distance, source, source_num, SNAPTARGET_GRID, getSnapperTolerance(), getSnapperAlwaysSnap(source), normal_to_line, point_on_line);
}

void GridSnapper::_addSnappedPoint(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const
{
    isr.points.emplace_back(snapped_point, source, source_num, SNAPTARGET_GRID, snapped_distance, getSnapperTolerance(), getSnapperAlwaysSnap(source), constrained_snap, true);
}

void GridSnapper::_addSnappedLinePerpendicularly(IntermSnapResults &isr, Geom::Point const &snapped_point, Geom::Coord const &snapped_distance, SnapSourceType const &source, long source_num, bool constrained_snap) const
{
    isr.points.emplace_back(snapped_point, source, source_num, SNAPTARGET_GRID_PERPENDICULAR, snapped_distance, getSnapperTolerance(), getSnapperAlwaysSnap(source), constrained_snap, true);
}

/**
 *  \return true if this Snapper will snap at least one kind of point.
 */
bool GridSnapper::ThisSnapperMightSnap() const
{
    return _snap_enabled && _snapmanager->snapprefs.isTargetSnappable(SNAPTARGET_GRID);
}

LineSnapper::LineList GridSnapper::get_snap_lines(const Geom::Point& p, int limit) const {
    LineList s;

    const auto desktop = _snapmanager->getDesktop();
    const int start = limit > 0 ? 0 : -1;

    for (int index = start; index < limit; ++index) {
        const auto [origin, spacing] = _grid->getEffectiveOriginAndSpacing(index);
        // empty spacing - skip this element
        if (spacing.isZero()) continue;;

        for (int i = 0; i < 2; ++i) {
            double scaled_spacing = spacing[i];

            if (getSnapVisibleOnly() && desktop) {
                // Only snap to visible grid lines.
                auto const sw = proj(spacing, i) * desktop->d2w().withoutTranslation();
                int const mult = calculate_scaling_factor(sw.length(), _grid->getMajorLineInterval());
                scaled_spacing *= mult;
            }

            s.emplace_back(basis(i), basis(i) * Util::round_to_upper_multiple_plus(p[i], scaled_spacing, origin[i]));
            s.emplace_back(basis(i), basis(i) * Util::round_to_lower_multiple_plus(p[i], scaled_spacing, origin[i]));
        }
    }

    return s;
}

LineSnapper::LineList GridSnapper::getSnapLinesAxonom(Geom::Point const &p) const
{
    LineList s;

    auto const *desktop = _snapmanager->getDesktop();
    auto const [origin, spacing] = _grid->getEffectiveOriginAndSpacing();

    bool is_y_vertical = _grid->isAngleYVertical();

    double angle_y_rad = Util::derived_angle_y(
            Geom::rad_from_deg(_grid->getAngleX()),
            Geom::rad_from_deg(_grid->getAngleZ())
        );

    double ta_x = tan(Geom::rad_from_deg(_grid->getAngleX()));
    double ta_z = tan(Geom::rad_from_deg(_grid->getAngleZ()));
    double ta_y = tan(angle_y_rad);

    if (desktop && desktop->yaxisdown()) {
        std::swap(ta_x, ta_z);
    }

    double spacing_h = spacing.y() / (ta_x + ta_z);
    double spacing_v = spacing.y();

    if (getSnapVisibleOnly() && desktop) {
        // Only snap to visible grid lines.
        auto const lyw = spacing.y() * desktop->d2w().descrim();
        int const mult = calculate_scaling_factor(lyw, _grid->getMajorLineInterval());
        spacing_h *= mult;
        spacing_v *= mult;
    }

    // In an axonometric grid, any point will be surrounded by 6 grid lines:
    // - y axis:
    //   - if vertical: 2 vertical grid lines, one left and one right from the point
    //   - else: 2 angled y grid lines, one above and one below the point
    // - 2 angled z grid lines, one above and one below the point
    // - 2 angled x grid lines, one above and one below the point

    // Calculate the x coordinate of the vertical grid lines
    Geom::Coord x_max = Util::round_to_upper_multiple_plus(p[Geom::X], spacing_h, origin[Geom::X]);
    Geom::Coord x_min = Util::round_to_lower_multiple_plus(p[Geom::X], spacing_h, origin[Geom::X]);

    // Calculate the y coordinate of the intersection of the angled grid lines with the y-axis
    double y_proj_along_x = p[Geom::Y] + ta_x * (p[Geom::X] - origin[Geom::X]);
    double y_proj_along_z = p[Geom::Y] - ta_z * (p[Geom::X] - origin[Geom::X]);
    double y_proj_along_y = p[Geom::Y] - ta_y * (p[Geom::X] - origin[Geom::X]);
    double y_proj_along_x_max = Util::round_to_upper_multiple_plus(y_proj_along_x, spacing_v, origin[Geom::Y]);
    double y_proj_along_x_min = Util::round_to_lower_multiple_plus(y_proj_along_x, spacing_v, origin[Geom::Y]);
    double y_proj_along_z_max = Util::round_to_upper_multiple_plus(y_proj_along_z, spacing_v, origin[Geom::Y]);
    double y_proj_along_z_min = Util::round_to_lower_multiple_plus(y_proj_along_z, spacing_v, origin[Geom::Y]);
    double y_proj_along_y_max = Util::round_to_upper_multiple_plus(y_proj_along_y, spacing_v * 0.5, origin[Geom::Y]);
    double y_proj_along_y_min = Util::round_to_lower_multiple_plus(y_proj_along_y, spacing_v * 0.5, origin[Geom::Y]);

    // Calculate the versor for the angled grid lines
    Geom::Point vers_x = Geom::Point(1, -ta_x);
    Geom::Point vers_z = Geom::Point(1, ta_z);
    Geom::Point vers_y = Geom::Point(1, ta_y);

    // Calculate the normal for the angled grid lines
    Geom::Point norm_x = Geom::rot90(vers_x);
    Geom::Point norm_z = Geom::rot90(vers_z);
    Geom::Point norm_y = Geom::rot90(vers_y);

    // The four angled grid lines form a parallelogram, enclosing the point
    // One of the two vertical grid lines divides this parallelogram in two triangles
    // We will now try to find out in which half (i.e. triangle) our point is, and return
    // only the three grid lines defining that triangle

    // The vertical grid line is at the intersection of two angled grid lines.
    // Now go find that intersection!
    Geom::Point p_x(0, y_proj_along_x_max);
    Geom::Line line_x(p_x, p_x + vers_x);
    Geom::Point p_z(0, y_proj_along_z_max);
    Geom::Line line_z(p_z, p_z + vers_z);
    Geom::Point p_y(0, y_proj_along_y_max);
    Geom::Line line_y(p_y, p_y + vers_y);

    Geom::OptCrossing inters = Geom::OptCrossing(); // empty by default
    try
    {
        inters = Geom::intersection(line_x, line_z);
    }
    catch (Geom::InfiniteSolutions &e)
    {
        // We're probably dealing with parallel lines; this is useless!
        return s;
    }

    // Determine which half of the parallelogram to use
    bool use_left_half = true;
    bool use_right_half = true;

    if (inters) {
        Geom::Point inters_pt = line_x.pointAt((*inters).ta);
        use_left_half = (p[Geom::X] - origin[Geom::X]) < inters_pt[Geom::X];
        use_right_half = !use_left_half;
    }

    // Return the three grid lines which define the triangle that encloses our point
    // If we didn't find an intersection above, all 6 grid lines will be returned
    if (use_left_half) {
        s.emplace_back(norm_x, Geom::Point(origin[Geom::X], y_proj_along_x_min));
        s.emplace_back(norm_z, Geom::Point(origin[Geom::X], y_proj_along_z_max));
        if (is_y_vertical) {
            s.emplace_back(Geom::Point(1, 0), Geom::Point(x_max, 0));
        }
    }

    if (use_right_half) {
        s.emplace_back(norm_x, Geom::Point(origin[Geom::X], y_proj_along_x_max));
        s.emplace_back(norm_z, Geom::Point(origin[Geom::X], y_proj_along_z_min));
        if (is_y_vertical) {
            s.emplace_back(Geom::Point(1, 0), Geom::Point(x_min, 0));
        }
    }

    if (!is_y_vertical) {
        s.emplace_back(norm_y, Geom::Point(origin[Geom::X], y_proj_along_y_min));
        s.emplace_back(norm_y, Geom::Point(origin[Geom::X], y_proj_along_y_max));
    }

    return s;
}

} // namespace Inkscape
