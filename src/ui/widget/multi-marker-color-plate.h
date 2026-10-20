// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_MULTI_MARKER_COLOR_PLATE_H
#define INKSCAPE_UI_WIDGET_MULTI_MARKER_COLOR_PLATE_H

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/scale.h>
#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>
#include <gtkmm/togglebutton.h>

#include "colors/color-set.h"
#include "colors/manager.h"
#include "ui/widget/color-page.h"
#include "ui/widget/color-preview.h"
#include "ui/widget/generic/icon-combobox.h"
#include "ui/widget/ink-color-wheel.h"
#include "recolor-art.h"

namespace Inkscape::Colors {
class Color;
class ColorSet;
namespace Space {
class AnySpace;
}
} // namespace Inkscape::Colors

namespace Inkscape::UI::Widget {

class MultiMarkerColorPlate : public Gtk::Box
{
public:
    explicit MultiMarkerColorPlate(Colors::ColorSet const &colors);

    void setColors(std::vector<Color> colors) { _color_wheel->setColors(std::move(colors)); }
    void setLightness(double value)
    {
        _color_wheel->setLightness(value);
        _lightness_bar.set_value(value);
    }

    void setSaturation(double value)
    {
        _color_wheel->setSaturation(value);
        _saturation_bar.set_value(value);
    }
    void setRecolorWidget(RecolorArt *ra) { _ra = ra; }
    void setActiveIndex(int index) { _color_wheel->setActiveIndex(index); }
    void toggleHueLock(bool locked) { _color_wheel->toggleHueLock(locked); _hue_lock.set_active(locked);}
    std::vector<Color> getColors() const { return _color_wheel->getColors(); }
    Colors::Color getColor() const { return _color_wheel->getColor(); }
    bool getHueLock() const { return _color_wheel->getHueLock(); }
    int getActiveIndex() const { return _color_wheel->getActiveIndex(); }
    int getHoverIndex() const { return _color_wheel->getHoverIndex(); }
    void connect_color_hovered(sigc::slot<void()> slot) { _color_wheel->connect_color_hovered(slot); }
    void connect_color_changed(sigc::slot<void()> slot) { _color_wheel->connect_color_changed(slot); }
    void changeColor(int index, Color color)
    {
        _color_wheel->changeColor(index, color);
        _color_wheel_preview->setRgba32(color.toRGBA());
        _specific_colors->set(color);
    }

private:
    MultiMarkerWheel *_color_wheel = nullptr;
    Gtk::Image *_lightness_icon = Gtk::make_managed<Gtk::Image>();
    Gtk::Box *_lightness_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    Glib::RefPtr<Gtk::Adjustment> adjustment = Gtk::Adjustment::create(100.0, 0.0, 100.0, 1.0, 10.0);
    Gtk::Scale &_lightness_bar;

    Gtk::Image *_saturation_icon = Gtk::make_managed<Gtk::Image>();
    Gtk::Box *_saturation_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    Glib::RefPtr<Gtk::Adjustment> _saturation_adjustment = Gtk::Adjustment::create(100.0, 0.0, 100.0, 1.0, 10.0);
    Gtk::Scale &_saturation_bar;

    Gtk::ToggleButton &_hue_lock;
    ColorPreview *_color_wheel_preview = nullptr;
    Manager *manager = &Manager::get();
    Gtk::Grid *_grid = nullptr;
    std::shared_ptr<Colors::ColorSet> _specific_colors;
    std::vector<std::unique_ptr<ColorPageChannel>> _channels;
    Gtk::Image *_hue_lock_image = nullptr;
    IconComboBox *_spaces_combo = nullptr;
    Gtk::Stack *_spaces_stack = nullptr;
    Gtk::StackSwitcher *_switcher = nullptr;
    RecolorArt *_ra = nullptr;
    Gtk::Button *_reset = nullptr;
    std::map<int, std::pair<std::string, std::shared_ptr<Colors::ColorSet>>> _color_sets;
    sigc::scoped_connection _specific_colors_changed;

    void _addPageForSpace(std::shared_ptr<Colors::Space::AnySpace> space, int page_num);
    void _createSlidersForSpace(std::shared_ptr<Colors::Space::AnySpace> space,
                                std::shared_ptr<Colors::ColorSet> &colors, int index);
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_MULTI_MARKER_COLOR_PLATE_H
