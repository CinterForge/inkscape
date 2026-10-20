// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 5/27/24.
//
// This is a widget hosting ColorPages and adding a color plate/wheel on top.
// It also injects a row with color eye dropper, rgb edit and color type selctor.
// This is a component used to implement https://gitlab.com/inkscape/ux/-/issues/246

#ifndef COLOR_PICKER_PANEL_H
#define COLOR_PICKER_PANEL_H

#include <gtkmm/grid.h>
#include <gtkmm/sizegroup.h>

#include "colors/color-set.h"
#include "colors/color.h"

class SPDesktop;

namespace Inkscape::UI::Widget {

class ColorPickerPanel : public Gtk::Grid {
public:
    // color plate type - rectangular, color wheel, no plate (only sliders)
    enum PlateType {Rect, Circle, None};
    // create a new color picker
    static std::unique_ptr<ColorPickerPanel> create(Colors::Space::Type space, PlateType type, std::shared_ptr<Colors::ColorSet> color);

    virtual void set_desktop(SPDesktop* dekstop) = 0;
    virtual void set_color(const Colors::Color& color) = 0;
    // request color type/space change
    virtual void set_picker_type(Colors::Space::Type type) = 0;
    // request type of color wheel/plate
    virtual void set_plate_type(PlateType plate) = 0;
    virtual PlateType get_plate_type() const = 0;
    // width of widgets in the first column: component names
    virtual Glib::RefPtr<Gtk::SizeGroup> get_first_column_size() = 0;
    // width of widgets in the last column: component entry boxes
    virtual Glib::RefPtr<Gtk::SizeGroup> get_last_column_size() = 0;
    // get a color space type change signal
    virtual sigc::signal<void (Colors::Space::Type)> get_color_space_changed() = 0;
};

// get a plate type from preferences
ColorPickerPanel::PlateType get_plate_type_preference(const char* pref_path_base, ColorPickerPanel::PlateType def_type = ColorPickerPanel::Rect);

// persist a plate type in preferences
void set_plate_type_preference(const char* pref_path_base, ColorPickerPanel::PlateType type);

} // namespace

#endif //COLOR_PICKER_PANEL_H
