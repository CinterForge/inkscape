// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief SVG feDropShadow filter effect
 *//*
 * Authors:
 *   Ryan Malloy <ryan@supported.systems>
 *
 * Copyright (C) 2025 Ryan Malloy
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_FEDROPSHADOW_H_SEEN
#define SP_FEDROPSHADOW_H_SEEN

#include <optional>
#include "sp-filter-primitive.h"
#include "colors/color.h"

class SPFeDropShadow final
    : public SPFilterPrimitive
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

    Geom::Rect calculate_region(Geom::Rect const &region) const override;

    // Getters for testing
    double get_dx() const { return dx; }
    double get_dy() const { return dy; }
    double get_stdDeviation() const { return stdDeviation; }
    double get_flood_opacity() const { return flood_opacity; }
    std::optional<Inkscape::Colors::Color> get_flood_color() const { return flood_color; }

private:
    double dx = 2.0;                          // Default horizontal offset
    double dy = 2.0;                          // Default vertical offset
    double stdDeviation = 2.0;                           // Default blur amount
    std::optional<Inkscape::Colors::Color> flood_color;  // Shadow color
    double flood_opacity = 1.0;                          // Default full opacity

    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;

    std::unique_ptr<Inkscape::Filters::FilterPrimitive> build_renderer(Inkscape::DrawingItem *item) const override;
};

#endif // SP_FEDROPSHADOW_H_SEEN

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :