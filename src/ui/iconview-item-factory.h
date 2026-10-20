// SPDX-License-Identifier: GPL-2.0-or-later
//
// This is an item factory for a ColumnView container. It creates an item with picture and label.
// During "bind" phase it will ask for label markup and picture image, plus tooltip, and populate item.

#ifndef _ICONVIEWITEMFACTORY_H_
#define _ICONVIEWITEMFACTORY_H_

#include <unordered_map>
#include <vector>
#include <gtkmm/binlayout.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/editablelabel.h>
#include <gtkmm/label.h>
#include <gtkmm/picture.h>
#include <gtkmm/signallistitemfactory.h>

#include "util.h"

namespace Inkscape::UI {

class IconViewItemFactory {
public:
    struct ItemData {
        Glib::ustring label_markup;
        Glib::RefPtr<Gdk::Texture> image; 
        Glib::ustring tooltip;
    };

    static std::unique_ptr<IconViewItemFactory> create(std::function<ItemData (Glib::RefPtr<Glib::ObjectBase>&)> get_item) {
        return std::unique_ptr<IconViewItemFactory>(new IconViewItemFactory(std::move(get_item)));
    }

    Glib::RefPtr<Gtk::ListItemFactory> get_factory() { return _factory; }

    // requests that labels are created (or not); gridview needs to be refreshed afterward
    void set_include_label(bool enable_labels) { _enable_labels = enable_labels; }

    // use EditableLabels instead of Labels;
    // note that editable labels do not support markup text nor text wrapping
    void enable_label_editing(bool enable) {
        _editable_labels = enable;
        set_track_bindings(true);
    }

    // keep track of bound items, so we can query them
    void set_track_bindings(bool track) { _track_items = track; }

    // given child widget find its corresponding model item
    Glib::RefPtr<Glib::ObjectBase> find_item(Gtk::Widget& item_container) {
        auto it = _bound_items.find(item_container.get_first_child());
        return it != _bound_items.end() ? it->second : Glib::RefPtr<Glib::ObjectBase>();
    }

    // given model item find a container's item that refers to it
    Gtk::Widget* find_child_item(Gtk::Widget& container, const Glib::RefPtr<Glib::ObjectBase>& model_item) {
        for (auto &child : children(container)) {
            if (find_item(child).get() == model_item.get()) {
                return child.get_first_child();
            }
        }
        return nullptr;
    }

    void set_use_tooltip_markup(bool use_markup = true) { _use_markup = use_markup; }

    sigc::signal<void (bool, Gtk::Editable&, Glib::RefPtr<Glib::ObjectBase>)>& signal_editing() {
        return _signal_editing;
    }
private:
    IconViewItemFactory(std::function<ItemData (Glib::RefPtr<Glib::ObjectBase>&)> get_item):
        _get_item_data(std::move(get_item)) {

        _factory = Gtk::SignalListItemFactory::create();

        _connections.emplace_back(_factory->signal_setup().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto box = Gtk::make_managed<Gtk::CenterBox>();
            box->add_css_class("item-box");
            box->set_orientation(Gtk::Orientation::VERTICAL);
            auto image = Gtk::make_managed<Gtk::Picture>();
            // add bin layout manager, so picture doesn't propagate its size to the parent container;
            // that way picture widget can be freely resized to desired dimensions, and it will not grow beyond them
            image->set_layout_manager(Gtk::BinLayout::create());
            image->set_halign(Gtk::Align::CENTER);
            image->set_valign(Gtk::Align::CENTER);
            box->set_start_widget(*image);
            // add a label below the picture
            if (_enable_labels) {
                Gtk::Widget* label = nullptr;
                if (_editable_labels) {
                    auto edit = Gtk::make_managed<Gtk::EditableLabel>();
                    label = edit;
                    edit->property_editing().signal_changed().connect([this, edit, box] {
                        if (auto it = _bound_items.find(box); it != _bound_items.end()) {
                            _signal_editing(edit->get_editing(), *edit, it->second);
                        }
                    });
                }
                else {
                    label = Gtk::make_managed<Gtk::Label>();
                }
                label->set_vexpand();
                label->set_valign(Gtk::Align::START);
                box->set_end_widget(*label);
            }

            list_item->set_child(*box);
        }));

        _connections.emplace_back(_factory->signal_bind().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto item = list_item->get_item();

            auto box = dynamic_cast<Gtk::CenterBox*>(list_item->get_child());
            if (!box) return;
            auto image = dynamic_cast<Gtk::Picture*>(box->get_start_widget());
            if (!image) return;

            auto item_data = _get_item_data(item);

            image->set_can_shrink(true);
            image->set_content_fit(Gtk::ContentFit::CONTAIN);
            auto tex = item_data.image;
            image->set_paintable(tex);
            // poor man's high dpi support here:
            auto scale = box->get_scale_factor();
            auto width = tex ? tex->get_intrinsic_width() / scale : 0;
            auto height = tex ? tex->get_intrinsic_height() / scale : 0;
            image->set_size_request(width, height);

            auto max_chars = std::min(5 + width / 10, 12);
            if (_editable_labels) {
                if (auto label = dynamic_cast<Gtk::EditableLabel*>(box->get_end_widget())) {
                    label->set_text(item_data.label_markup);
                    //those two are no-ops, no way to limit size as of gtk 4.20.1
                    label->set_max_width_chars(max_chars);
                    // label->set_width_chars(4);
                    label->set_alignment(0.5f);
                    label->set_valign(Gtk::Align::START);
                }
            }
            else {
                if (auto label = dynamic_cast<Gtk::Label*>(box->get_end_widget())) {
                    label->set_markup(item_data.label_markup);
                    label->set_max_width_chars(max_chars);
                    label->set_wrap();
                    label->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
                    label->set_natural_wrap_mode(Gtk::NaturalWrapMode::WORD);
                    label->set_justify(Gtk::Justification::CENTER);
                    label->set_valign(Gtk::Align::START);
                }
            }

            if (_use_markup) {
                box->set_tooltip_markup(item_data.tooltip);
            } else {
                box->set_tooltip_text(item_data.tooltip);
            }

            if (_track_items) _bound_items[box] = item;
        }));

        _connections.emplace_back(_factory->signal_unbind().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            if (_track_items) {
                auto box = dynamic_cast<Gtk::CenterBox*>(list_item->get_child());
                _bound_items.erase(box);
            }
        }));
    }

    std::function<ItemData (Glib::RefPtr<Glib::ObjectBase>&)> _get_item_data;
    Glib::RefPtr<Gtk::SignalListItemFactory> _factory;
    bool _use_markup = false;
    bool _enable_labels = true;
    bool _track_items = false;
    bool _editable_labels = false;
    std::unordered_map<Gtk::Widget*, Glib::RefPtr<Glib::ObjectBase>> _bound_items;
    std::vector<sigc::scoped_connection> _connections;
    sigc::signal<void (bool, Gtk::Editable& edit, Glib::RefPtr<Glib::ObjectBase>)> _signal_editing;
};

} // namespace

#endif
