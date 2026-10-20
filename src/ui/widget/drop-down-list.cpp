// SPDX-License-Identifier: GPL-2.0-or-later

#include "drop-down-list.h"

#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/label.h>
#include <gtkmm/listheader.h>
#include <gtkmm/stringobject.h>

namespace Inkscape::UI::Widget {

DropDownList::DropDownList() {
    _init();
}

DropDownList::DropDownList(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Gtk::DropDown(cobject) {

    //TODO: verify if we want/need to set model too
    _init();
}

void DropDownList::_init() {
    set_name("DropDownList");

    // enable expression to support search early on, as it resets the item factory;
    // search can be enabled/disabled separately as needed
    auto expression = Gtk::ClosureExpression<Glib::ustring>::create([this](auto& item){
        return get_item_string(item);
    });
    set_expression(expression);

    // enable cycling through the items with key up/down
    auto key_entry = Gtk::EventControllerKey::create();
    key_entry->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    key_entry->signal_key_pressed().connect([this](auto keyval, auto keycode, auto state) {
        if (state != Gdk::ModifierType::NO_MODIFIER_MASK) return false;

        switch (keyval) {
        case GDK_KEY_Down:
            {
                int n = get_item_count();
                int sel = get_selected();
                if (sel + 1 < n) {
                    set_selected(sel + 1);
                }
            }
            return true;
        case GDK_KEY_Up:
            {
                int sel = get_selected();
                if (sel - 1 >= 0) {
                    set_selected(sel - 1);
                }
            }
            return true;
        default:
            return false;
        }
    }, false); // Before default handler.
    add_controller(key_entry);

    /* Example of setting up and binding labels in header factory:
     *
    _header->signal_setup_obj().connect([this](const Glib::RefPtr<Glib::Object>& obj) {
        auto list_header = std::dynamic_pointer_cast<Gtk::ListHeader>(obj);
        // list_header->set_child(*set_up_item(true));
    });
    _header->signal_bind_obj().connect([this](const Glib::RefPtr<Glib::Object>& obj) {
        return;
        auto list_header = std::dynamic_pointer_cast<Gtk::ListHeader>(obj);
        auto& label = dynamic_cast<Gtk::Label&>(*list_header->get_child());
        auto item = std::dynamic_pointer_cast<Gtk::StringObject>(list_header->get_item());
        label.set_label(item->get_string());
    });
    set_header_factory(_header);
    */

    auto dropdown_button = Gtk::SignalListItemFactory::create();
    dropdown_button->signal_setup().connect([this](auto& list_item) {
        list_item->set_child(*set_up_item(_ellipsize_button));
    });
    dropdown_button->signal_bind().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto& label = dynamic_cast<Gtk::Label&>(*list_item->get_child());
        label.set_label(get_item_string(list_item->get_item()));
    });

    _factory->signal_setup().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        list_item->set_child(*set_up_item(false));
    });

    _factory->signal_bind().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto& label = dynamic_cast<Gtk::Label&>(*list_item->get_child());
        if (_separator_callback) {
            auto pos = list_item->get_position();
            if (_separator_callback(pos)) {
                label.get_parent()->add_css_class("top-separator");
            }
        }
        label.set_label(get_item_string(list_item->get_item()));
    });

    // separate factory to allow dropdown button to shrink (ellipsis)
    set_factory(dropdown_button);
    // normal list items without ellipsis
    set_list_factory(_factory);
    set_model(_model);
}

Glib::ustring DropDownList::get_item_string(const Glib::RefPtr<Glib::ObjectBase>& item) {
    if (_to_string) {
        return _to_string(item);
    }
    auto str_item = std::dynamic_pointer_cast<Gtk::StringObject>(item);
    return str_item->get_string();
}

Gtk::Label* DropDownList::set_up_item(bool ellipsize) {
    auto label = Gtk::make_managed<Gtk::Label>();
    label->set_xalign(0);
    label->set_valign(Gtk::Align::CENTER);
    if (ellipsize) {
        label->set_ellipsize(Pango::EllipsizeMode::END);
        label->set_max_width_chars(_button_max_chars);
    }
    return label;
}

unsigned int DropDownList::append(const Glib::ustring& item) {
    auto n = _model->get_n_items();
    _model->append(item);
    return n;
}

void DropDownList::set_button_max_chars(int max_chars) {
    _button_max_chars = max_chars;
}

void DropDownList::enable_search(bool enable) {
    set_enable_search(enable);
}

void DropDownList::set_row_separator_func(std::function<bool (unsigned int)> callback) {
    _separator_callback = callback;
}

void DropDownList::set_to_string_func(std::function<Glib::ustring(const Glib::RefPtr<Glib::ObjectBase>&)> callback) {
    _to_string = callback;
}

} // namespace
