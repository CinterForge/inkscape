// SPDX-License-Identifier: GPL-2.0-or-later
#include "multi-marker-color-plate.h"

#include <glibmm/i18n.h>
#include <gtkmm/grid.h>
#include <gtkmm/gridlayout.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include "colors/spaces/base.h"
#include "colors/spaces/enum.h"
#include "ui/icon-names.h"
#include "ui/widget/color-page.h"
#include "ui/widget/color-preview.h"
#include "ui/widget/color-slider.h"
#include "ui/widget/generic/icon-combobox.h"
#include "ui/widget/generic/spin-button.h"

namespace Inkscape::UI::Widget {

MultiMarkerColorPlate::MultiMarkerColorPlate(Colors::ColorSet const &colors)
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , _specific_colors(std::make_shared<Colors::ColorSet>(manager->find(Space::Type::HSL),
                                                          colors.getAlphaConstraint().value_or(true)))
    , _color_wheel(Gtk::make_managed<MultiMarkerWheel>())
    , _hue_lock(*Gtk::make_managed<Gtk::ToggleButton>())
    , _lightness_bar(*Gtk::make_managed<Gtk::Scale>(adjustment, Gtk::Orientation::HORIZONTAL))
    , _saturation_bar(*Gtk::make_managed<Gtk::Scale>(_saturation_adjustment, Gtk::Orientation::HORIZONTAL))
    , _color_wheel_preview(Gtk::make_managed<ColorPreview>())
    , _grid(Gtk::make_managed<Gtk::Grid>())
    , _spaces_combo(Gtk::make_managed<IconComboBox>())
    , _switcher(Gtk::make_managed<Gtk::StackSwitcher>())
    , _spaces_stack(Gtk::make_managed<Gtk::Stack>())
    , _reset(Gtk::make_managed<Gtk::Button>())
{
    _specific_colors->set(Color(0xFF0000FF));

    _switcher->set_stack(*_spaces_stack);

    _spaces_combo->add_css_class("regular");
    _spaces_combo->set_focusable(false);
    _spaces_combo->set_tooltip_text(_("Choose style of color selection"));
    _spaces_combo->set_hexpand(false);
    _spaces_combo->set_halign(Gtk::Align::END);
    _spaces_combo->set_margin_top(4);
    _spaces_combo->set_margin_bottom(8);

    _spaces_combo->signal_changed().connect([this](int index) {
        _specific_colors = _color_sets[index].second;
        _spaces_stack->set_visible_child(_color_sets[index].first);
        _specific_colors_changed.disconnect();
        _specific_colors_changed = _specific_colors->signal_changed.connect([this]() {
            Color new_color = _specific_colors->get().value();
            if (_color_wheel->getActiveIndex() != -1) {
                _color_wheel->changeColor(_color_wheel->getActiveIndex(), new_color);
            }
        });
    });

    int index = 0;
    for (auto &space : Colors::Manager::get().spaces(Space::Traits::Picker)) {
        _createSlidersForSpace(space, _specific_colors, index);
        _addPageForSpace(space, index++);
    }

    _lightness_icon->set_from_icon_name(INKSCAPE_ICON("lightness"));
    _lightness_icon->set_tooltip_text(_("Change saturation for all if hue lock is on"));
    _lightness_bar.set_value_pos(Gtk::PositionType::RIGHT);
    _lightness_bar.set_hexpand(true);
    _lightness_bar.set_draw_value(true);
    _lightness_bar.signal_value_changed().connect([this]() {
        double value = _lightness_bar.get_value();
        _color_wheel->setLightness(value);
    });

    _saturation_icon->set_from_icon_name(INKSCAPE_ICON("saturation"));
    _saturation_icon->set_tooltip_text(_("Change saturation for all if hue lock is on"));
    _saturation_bar.set_value_pos(Gtk::PositionType::RIGHT);
    _saturation_bar.set_hexpand(true);
    _saturation_bar.set_draw_value(true);
    _saturation_bar.signal_value_changed().connect([this]() {
        double value = _saturation_bar.get_value();
        _color_wheel->setSaturation(value);
    });

    _hue_lock_image = Gtk::make_managed<Gtk::Image>();
    _hue_lock_image->set_from_icon_name(INKSCAPE_ICON("object-unlocked"));
    _hue_lock.set_child(*_hue_lock_image);
    _hue_lock.signal_toggled().connect([this]() {
        _color_wheel->toggleHueLock(_hue_lock.get_active());
        if (_hue_lock.get_active()) {
            _hue_lock_image->set_from_icon_name(INKSCAPE_ICON("object-locked"));
            _hue_lock.set_child(*_hue_lock_image);
        } else {
            _hue_lock_image->set_from_icon_name(INKSCAPE_ICON("object-unlocked"));
            _hue_lock.set_child(*_hue_lock_image);
        }
        _color_wheel->redrawOnHueLocked();
    });
    _hue_lock.set_tooltip_text(_("Lock hue angles for colors set"));
    _hue_lock.set_hexpand(false);
    _hue_lock.set_margin_top(8);
    _hue_lock.set_halign(Gtk::Align::END);

    _color_wheel_preview->set_hexpand(false);
    _color_wheel_preview->set_can_focus(false);
    _color_wheel_preview->set_size_request(35,35);
    _color_wheel_preview->set_halign(Gtk::Align::START);
    _color_wheel_preview->set_margin_top(8);
    _color_wheel_preview->setStyle(_color_wheel_preview->Style::Outlined);
    _color_wheel_preview->set_tooltip_text(_("Color preview"));
    _color_wheel->connect_color_changed(static_cast<sigc::slot<void()>>([this] {
        _color_wheel_preview->setRgba32(_color_wheel->getColor().toRGBA());
    }));

    auto image = Gtk::make_managed<Gtk::Image>();
    image->set_from_icon_name(INKSCAPE_ICON("reset-settings"));
    _reset->set_child(*image);
    _reset->set_margin_top(8);
    _reset->signal_clicked().connect([this] {
        if (_ra) {
            _ra->onResetClicked();
        }
    });
    _reset->set_tooltip_text(_("Reset to original colors"));

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    box->set_spacing(64);
    box->append(*_color_wheel_preview);
    box->append(*_reset);
    box->append(_hue_lock);

    _lightness_box->append(*_lightness_icon);
    _lightness_box->append(_lightness_bar);

    _saturation_box->append(*_saturation_icon);
    _saturation_box->append(_saturation_bar);

    _spaces_stack->set_visible_child("RGB");

    append(*box);
    append(*_color_wheel);
    append(*_lightness_box);
    append(*_saturation_box);
    append(*_spaces_combo);
    append(*_spaces_stack);
}

void MultiMarkerColorPlate::_addPageForSpace(std::shared_ptr<Colors::Space::AnySpace> space, int page_num)
{
    auto mode_name = space->getName();
    _spaces_combo->add_row(space->getIcon(), mode_name, page_num);
}

void MultiMarkerColorPlate::_createSlidersForSpace(std::shared_ptr<Colors::Space::AnySpace> space,
                                                   std::shared_ptr<Colors::ColorSet> &colors, int index)
{
    auto mode_name = space->getName();
    Gtk::Grid *_grid = Gtk::make_managed<Gtk::Grid>();
    auto new_colors = std::make_shared<Colors::ColorSet>(space, colors->getAlphaConstraint().value_or(true));
    new_colors->set(colors->get().value());
    int row = 0;
    for (auto &component : new_colors->getComponents()) {
        auto label = Gtk::make_managed<Gtk::Label>();
        auto slider = Gtk::make_managed<ColorSlider>(new_colors, component);
        auto spin = Gtk::make_managed<InkSpinButton>();
        spin->set_digits(component.id == "alpha" ? 0 : 1);
        if (component.scale < 100) {
            // for small values increase precision
            spin->set_digits(2);
            spin->get_adjustment()->set_step_increment(0.1);
        }
        _grid->attach(*label, 0, row);
        _grid->attach(*slider, 1, row);
        _grid->attach(*spin, 2, row++);
        _channels.emplace_back(std::make_unique<ColorPageChannel>(new_colors, *label, *slider, *spin));
    }
    _color_sets[index] = {mode_name, new_colors};
    _spaces_stack->add(*_grid, mode_name, mode_name);
}

} // namespace Inkscape::UI::Widget
