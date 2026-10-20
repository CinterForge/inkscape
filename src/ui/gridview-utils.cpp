// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 9/22/24.
//

#include "gridview-utils.h"

#include <string>
#include <utility>
#include <glib/gi18n.h>
#include <glibmm/object.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>

#include "desktop.h"
#include "inkscape.h"
#include "colors/color.h"
#include "widget/canvas.h"
#include "widget/color-page.h"
#include "widget/color-preview.h"
#include "widget/generic/spin-button.h"
#include "widget/paint-switch.h"

namespace Inkscape::UI::Utils {
using namespace Inkscape::UI;

namespace {

struct ItemData : public Glib::Object {
    std::string id;
    double value = 0;
    Glib::ustring label;
    Glib::ustring icon;
    Glib::ustring tooltip;
    std::optional<Color> color;
    Cairo::RefPtr<Cairo::Pattern> pattern;
    bool is_swatch = false;
    bool is_radial = false;

    static Glib::RefPtr<ItemData> create(
        const std::string& id,
        double value,
        const Glib::ustring& label,
        const Glib::ustring& icon,
        const Glib::ustring& tooltip,
        std::optional<Color> color,
        Cairo::RefPtr<Cairo::Pattern> pattern,
        bool is_swatch,
        bool is_radial
    ) {
        auto item = Glib::make_refptr_for_instance<ItemData>(new ItemData());
        item->id = id;
        item->value = value;
        item->label = label;
        item->icon = icon;
        item->tooltip = tooltip;
        item->color = color;
        item->pattern = pattern;
        item->is_swatch = is_swatch;
        item->is_radial = is_radial;
        return item;
    }

    bool operator == (const ItemData& item) const {
        return id == item.id &&
            value == item.value &&
            label == item.label &&
            icon == item.icon &&
            tooltip == item.tooltip &&
            is_swatch == item.is_swatch &&
            is_radial == item.is_radial &&
            color == item.color &&
            pattern == item.pattern;
    }

    ItemData& operator = (const ItemData& src) {
        id = src.id;
        value = src.value;
        label = src.label;
        icon = src.icon;
        tooltip = src.tooltip;
        color = src.color;
        pattern = src.pattern;
        is_swatch = src.is_swatch;
        is_radial = src.is_radial;
        return *this;
    }

private:
    ItemData() {}
};

} // namespace

GridViewList::GridViewList(Type type, Glib::RefPtr<Gtk::Adjustment> adjustment, int digits):
    _type(type),
    _adjustment(std::move(adjustment)),
    _digits(digits) {

    create_store();
    add_css_class("compact-flowbox");
}

GridViewList::GridViewList(Type type):
    GridViewList(type, {}, 0) {}

GridViewList::GridViewList(Glib::RefPtr<Gtk::Adjustment> adjustment, int digits):
    GridViewList(Spin, adjustment, digits) {}

GridViewList::~GridViewList() {
    _popover.unparent();
}

Glib::RefPtr<Glib::Object> GridViewList::create_item(const std::string& id, double value, const Glib::ustring& label,
    const Glib::ustring& icon, const Glib::ustring& tooltip, std::optional<Colors::Color> color, Cairo::RefPtr<Cairo::Pattern> pattern,
    bool is_swatch, bool is_radial) {

    return ItemData::create(id, value, label, icon, tooltip, color, pattern, is_swatch, is_radial);
}

void GridViewList::update_store(size_t count, std::function<Glib::RefPtr<Glib::Object> (size_t)> callback) {
    _popover.unparent();
    //todo: improve
    _store->freeze_notify();
    _store->remove_all();
    for (size_t i = 0; i < count; ++i) {
        auto item = callback(i);
        //assert( auto item = std::dynamic_pointer_cast<ItemData>(callback(i));
        _store->append(item);
    }
    _store->thaw_notify();

    for (auto* box : get_children()) {
        box->set_focusable(false);
    }
}

namespace {

Widget::InkSpinButton* create_spin_button(const Glib::RefPtr<ItemData>& item, const Glib::RefPtr<Gtk::Adjustment>& adjustment, int digits) {
    auto button = Gtk::make_managed<Widget::InkSpinButton>();
    button->set_hexpand();
    button->set_drag_sensitivity(0); // disable adjustment with scrolling/dragging
    button->set_has_arrows(false);
    if (adjustment) {
        auto adj = Gtk::Adjustment::create(adjustment->get_value(), adjustment->get_lower(), adjustment->get_upper(),
            adjustment->get_step_increment(), adjustment->get_page_increment());
        button->set_adjustment(adj);
    }
    button->set_digits(digits);
    button->set_value(item->value);
    return button;
}

Gtk::Widget* create_color_preview(const Glib::RefPtr<ItemData>& item, int tile_size) {
    if (item->color.has_value()) {
        auto& color = *Gtk::make_managed<Widget::ColorPreview>();
        color.set_size_request(tile_size, tile_size);
        color.set_checkerboard_tile_size(4);
        color.set_frame(true);
        color.set_valign(Gtk::Align::CENTER);
        color.setRgba32(item->color->toRGBA());
        color.setIndicator(item->is_swatch ? Widget::ColorPreview::Swatch : Widget::ColorPreview::None);
        color.set_tooltip_text(item->tooltip);
        return &color;
    }
    else if (item->pattern) {
        auto& color = *Gtk::make_managed<Widget::ColorPreview>();
        color.set_size_request(tile_size, tile_size);
        color.set_frame(true);
        color.set_valign(Gtk::Align::CENTER);
        color.setPattern(item->pattern);
        color.setIndicator(item->is_radial ? Widget::ColorPreview::RadialGradient : Widget::ColorPreview::LinearGradient);
        // todo: swatch gradient
        // color.setIndicator(item->is_swatch ? UI::Widget::ColorPreview::Swatch : UI::Widget::ColorPreview::None);
        color.set_tooltip_text(item->tooltip);
        return &color;
    }
    else {
        auto& image = *Gtk::make_managed<Gtk::Image>();
        image.set_size_request(tile_size, tile_size);
        image.set_from_icon_name(item->icon);
        return &image;
    }
}

Gtk::Button* create_compact_color_button(const Glib::RefPtr<ItemData>& item, int tile_size) {
    auto button = Gtk::make_managed<Gtk::Button>();
    auto color = create_color_preview(item, tile_size);
    color->set_halign(Gtk::Align::CENTER);
    color->set_valign(Gtk::Align::CENTER);
    button->set_child(*color);
    button->set_tooltip_text(item->tooltip);
    return button;
}

Gtk::Button* create_color_button(const Glib::RefPtr<ItemData>& item, int tile_size) {
    auto box = Gtk::make_managed<Gtk::Box>();
    box->add_css_class("item-box");
    box->set_orientation(Gtk::Orientation::HORIZONTAL);
    box->set_spacing(4);

    box->append(*create_color_preview(item, tile_size));

    auto label = Gtk::make_managed<Gtk::Label>();
    label->set_hexpand();
    label->set_xalign(0);
    label->set_valign(Gtk::Align::CENTER);
    label->set_max_width_chars(16); // put breaks on unlimited growth
    label->set_ellipsize(Pango::EllipsizeMode::MIDDLE);
    label->set_label(item->label);
    label->set_tooltip_text(item->tooltip);
    box->append(*label);

    auto img = Gtk::make_managed<Gtk::Image>();
    img->set_from_icon_name("pan-down");
    img->set_halign(Gtk::Align::END);
    img->set_hexpand();
    box->append(*img);

    auto button = Gtk::make_managed<Gtk::Button>();
    button->set_child(*box);
    return button;
}

} // namespace

void GridViewList::create_store() {
    auto store = Gio::ListStore<ItemData>::create();
    _store = store;

    set_homogeneous();
    set_row_spacing(0);
    set_column_spacing(0);
    set_min_children_per_line(1);
    set_max_children_per_line(999);
    set_halign(Gtk::Align::START);
    set_selection_mode(); // none
    bind_list_store(store, [this](const Glib::RefPtr<ItemData>& item) -> Widget* {
        switch (_type) {
        case Button:
            {
                auto button = Gtk::make_managed<Gtk::Button>(item->label);
                button->signal_clicked().connect([this, id = item->id, value = item->value]() {
                    _signal_button_clicked.emit(id, value);
                });
                return button;
            }

        case ColorLong:
            {
                auto button = create_color_button(item, _tile_size);
                auto id = item->id;
                button->signal_clicked().connect([this, button]() {
                    int x = 0, y = 0;
                    auto alloc = button->get_allocation();
                    _popover.unparent();
                    _popover.set_parent(*button);
                    _popover.set_pointing_to(Gdk::Rectangle(x, y, alloc.get_width(), alloc.get_height()));
                    _popover.set_offset(0, -8);
                    _popover.set_position(Gtk::PositionType::BOTTOM);
                    if (!_paint) {
                        _paint = UI::Widget::PaintSwitch::create(true, false);
                        _popover.set_child(*_paint);
                    }
                    //todo: update popup paint
                    _popover.popup();
                });
                return button;
            }

        case ColorCompact:
            {
                auto button = create_compact_color_button(item, _tile_size);
                auto id = item->id;
                button->signal_clicked().connect([this, button]() {
                    int x = 0, y = 0;
                    auto alloc = button->get_allocation();
                    _popover.unparent();
                    _popover.set_parent(*button);
                    _popover.set_pointing_to(Gdk::Rectangle(x, y, alloc.get_width(), alloc.get_height()));
                    _popover.set_offset(0, -8);
                    _popover.set_position(Gtk::PositionType::BOTTOM);
                    _popover.popup();
                });
                return button;
            }

        case Label:
            {
                auto label = Gtk::make_managed<Gtk::Label>(item->label);
                label->set_hexpand();
                label->set_xalign(0);
                return label;
            }
        case Spin:
            {
                auto spin = create_spin_button(item, _adjustment, _digits);
                spin->set_enter_exit_edit();
                // this is not pretty but useful
                //TODO: replace with DefocusTarget, when ready
                // spin->set_defocus_widget(SP_ACTIVE_DESKTOP->getCanvas());
                spin->signal_value_changed().connect([this, id = item->id, orig = item->value](double new_value) {
                    _signal_value_changed.emit(id, orig, new_value);
                });
                return spin;
            }
        }
        return nullptr;
    });
}

} // namespace
