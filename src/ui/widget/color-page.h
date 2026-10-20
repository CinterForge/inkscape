// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Build a set of color sliders for a given color space
 *//*
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_COLOR_SLIDERS_H
#define SEEN_SP_COLOR_SLIDERS_H

#include <gtkmm/box.h>

#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/expander.h>
#include <gtkmm/grid.h>
#include <gtkmm/sizegroup.h>
#include <sigc++/scoped_connection.h>

#include "color-notebook.h"
#include "color-slider.h"
#include "color-preview.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/ink-color-wheel.h"

using namespace Inkscape::Colors;

namespace Gtk {
class Label;
}

namespace Inkscape::Colors {
class Color;
class ColorSet;
namespace Space {
class AnySpace;
}
}

namespace Inkscape::UI::Widget {
class ColorWheel;
class InkSpinButton;
class ColorWheelBase;
class ColorPageChannel;

class ColorPage : public Gtk::Box
{
public:
    ColorPage(std::shared_ptr<Space::AnySpace> space, std::shared_ptr<ColorSet> colors);
    ~ColorPage() override;

    void show_expander(bool show);
    ColorWheel* create_color_wheel(Space::Type type, bool disc);
    void set_spinner_size_pattern(const std::string& pattern);

    void attach_page(Glib::RefPtr<Gtk::SizeGroup> first_column, Glib::RefPtr<Gtk::SizeGroup> last_column);
    void detach_page(Glib::RefPtr<Gtk::SizeGroup> first_column, Glib::RefPtr<Gtk::SizeGroup> last_column);
    void setCurrentColor(std::shared_ptr<ColorSet> color)
    {
        if (_color_wheel) {
            _color_wheel->set_color(color->get().value());
        }
    }

protected:
    std::shared_ptr<Space::AnySpace> _space;
    std::shared_ptr<ColorSet> _selected_colors;
    std::shared_ptr<ColorSet> _specific_colors;

    std::vector<std::unique_ptr<ColorPageChannel>> _channels;
private:
    Gtk::Grid _grid;
    Gtk::Expander _expander;
    Gtk::Box _frame;
    ColorPreview _preview;
    Gtk::Entry _rgb_edit;
    // eye dropper color picker
    Gtk::Button _dropper;
    sigc::scoped_connection _specific_changed_connection;
    sigc::scoped_connection _selected_changed_connection;
    ColorWheel* _color_wheel = nullptr;
    sigc::scoped_connection _color_wheel_changed;
};

class ColorPageChannel
{
public:
    ColorPageChannel(
        std::shared_ptr<Colors::ColorSet> color,
        Gtk::Label &label,
        ColorSlider &slider,
        InkSpinButton &spin);
    ColorPageChannel(const ColorPageChannel&) = delete;
    Gtk::Label& get_label() { return _label; }
    InkSpinButton& get_spin() { return _spin; }
private:
    Gtk::Label &_label;
    ColorSlider &_slider;
    InkSpinButton &_spin;
    Glib::RefPtr<Gtk::Adjustment> _adj;
    std::shared_ptr<Colors::ColorSet> _color;
    sigc::scoped_connection _adj_changed;
    sigc::scoped_connection _slider_changed;
    sigc::scoped_connection _color_changed;
};

} // namespace Inkscape::UI::Widget

#endif /* !SEEN_SP_COLOR_SLIDERS_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
