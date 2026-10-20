// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This file is for the logic behind the RecolorArt widget.
 */
/*
 * Authors:
 *   Fatma Omara <ftomara647@gmail.com>
 *
 * Copyright (C) 2025 authors
 */

#ifndef INKSCAPE_OBJECT_COLORS_H
#define INKSCAPE_OBJECT_COLORS_H

#include <unordered_map>
#include "colors/color.h"

class SPObject;
class SPStop;

namespace Inkscape {

enum class ObjectStyleType
{
    None,
    Fill,
    Stroke,
    Pattern,
    Swatch,
    Linear,
    Radial,
    Mesh,
    Mask,
    Marker,
};

struct ColorRef
{
    SPObject *item;
    std::string kind;
    ObjectStyleType type;
};

struct ColorPair
{
    Colors::Color old_color;
    Colors::Color new_color;
};

class ObjectColorSet
{
public:
    using SelectedColorsMap = std::unordered_map<uint32_t, std::pair<std::vector<ColorRef>, std::optional<ColorPair>>>;

    void populateMap(Colors::Color color, SPObject *style, ObjectStyleType type, std::string const &kind);
    void revertToOriginalColors(bool is_reset_clicked = false);
    void convertToRecoloredColors();
    void populateStopsMap(SPStop *stop);
    void recolorStops(uint32_t old_color, Colors::Color new_color);
    void changeObjectColor(ColorRef const &item, Colors::Color const &color);
    void changeOpacity(bool change_opacity = false , uint32_t color= 0 ,bool is_preview = true);
    void setSelectedNewColor(uint32_t key_color, Colors::Color const &new_color);
    std::optional<Colors::Color> getSelectedNewColor(uint32_t key_color) const;
    bool isGradientStopsEmpty() const { return _gradient_stops.empty(); }
    bool isColorsEmpty() const { return colors.empty(); }
    void clearData();
    bool setSelectedNewColor(std::vector<Colors::Color> const &new_colors);
    uint32_t getFirstKey() const { return _selected_colors.begin()->first; }
    std::vector<ColorRef> &getSelectedItems(uint32_t key_color);
    int getColorIndex(uint32_t key_color) const;
    std::vector<Colors::Color> &getColors() { return colors; }
    std::optional<Colors::Color>getColor(int index) const;
    SelectedColorsMap const &getSelectedColorsMap() const { return _selected_colors; }
    bool applyNewColorToSelection(uint32_t key_color, Colors::Color const &new_color);

private:
    SelectedColorsMap _selected_colors;
    std::unordered_map<uint32_t, std::vector<SPStop *>> _gradient_stops;
    std::vector<Colors::Color> colors;
    std::unordered_map<uint32_t, int> color_wheel_colors_map;
};

/// Extract the colors from a list of objects.
ObjectColorSet collect_colours(std::vector<SPObject *> const &objects, ObjectStyleType type = ObjectStyleType::None);

} // namespace Inkscape

#endif // INKSCAPE_OBJECT_COLORS_H
