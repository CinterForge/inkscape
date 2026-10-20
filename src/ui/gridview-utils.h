// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 9/22/24.
//

#ifndef GRIDVIEW_UTILS_H
#define GRIDVIEW_UTILS_H
#include <gtkmm/boolfilter.h>
#include <gtkmm/box.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/gridview.h>
#include <gtkmm/popover.h>
#include <gtkmm/singleselection.h>

#include "filtered-store.h"
#include "colors/color.h"
#include "widget/paint-switch.h"

namespace Gtk {
class SignalListItemFactory;
}

namespace Gio {
class ListStoreBase;
}

namespace Inkscape::UI::Utils {

class GridViewList : public Gtk::FlowBox {
public:
    // type of content elements
    enum Type { Label, ColorLong, ColorCompact, Button, Spin };
    GridViewList(Type type);
    GridViewList(Glib::RefPtr<Gtk::Adjustment> adjustment, int digits);
    ~GridViewList() override;

    static Glib::RefPtr<Glib::Object> create_item(
        const std::string& id,
        double value,
        const Glib::ustring& label,
        const Glib::ustring& icon,
        const Glib::ustring& tooltip,
        std::optional<Colors::Color> color,
        Cairo::RefPtr<Cairo::Pattern> pattern,
        bool is_swatch = false,
        bool is_radial = false);

    struct Item {
        std::string id;
        Glib::ustring label, icon;
        double value = 0;
        std::optional<Colors::Color> color;
        Glib::ustring tooltip;
        bool swatch = false;
    };

    void update_store(size_t count, std::function<Glib::RefPtr<Glib::Object> (size_t)> callback);

    sigc::signal<void (const std::string& id, double original)> get_signal_button_clicked() {
        return _signal_button_clicked;
    }
    sigc::signal<void (const std::string& id, double original, double new_value)> get_signal_value_changed() {
        return _signal_value_changed;
    }

private:
    GridViewList(Type type, Glib::RefPtr<Gtk::Adjustment> adjustment, int digits);
    Type _type;
    Glib::RefPtr<Gtk::Adjustment> _adjustment;
    int _digits = 0;
    void create_store();
    Glib::RefPtr<Gio::ListStoreBase> _store;
    int _tile_size = 16;
    sigc::signal<void ()> _signal_selection_changed;
    sigc::signal<void (const std::string& id, double original)> _signal_button_clicked;
    sigc::signal<void (const std::string& id, double original, double new_value)> _signal_value_changed;
    Gtk::Popover _popover;
    std::unique_ptr<UI::Widget::PaintSwitch> _paint;
};

} // namespace

#endif //GRIDVIEW_UTILS_H
