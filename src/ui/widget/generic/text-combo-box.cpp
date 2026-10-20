// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#include "text-combo-box.h"

#include <gtkmm/accelerator.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/stringobject.h>

#include "ui/containerize.h"

namespace Inkscape::UI::Widget {

TextComboBox::TextComboBox() :
    Glib::ObjectBase("TextComboBox"),
    CssNameClassInit{"text-combobox"} {
    construct();
}

TextComboBox::TextComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
    Glib::ObjectBase("TextComboBox"),
    CssNameClassInit{"text-combobox"},
    BuildableWidget(cobject, builder) {
    construct();
}

static bool starts_with(const Glib::ustring& text, const Glib::ustring& prefix) {
    return text.lowercase().raw().starts_with(prefix.lowercase().raw());
}

void TextComboBox::construct() {
    _box.add_css_class("linked");
    _box.append(_entry);
    _box.append(_menu_btn);
    _menu_btn.set_popover(_popup);
    _menu_btn.set_can_focus(false);
    _popup.set_has_arrow(false);
    _popup.add_css_class("menu");
    _popup.set_autohide(false); // no autohide, so it doesn't steal focus from our entry widget
    _entry.set_hexpand();

    auto wnd = Gtk::make_managed<Gtk::ScrolledWindow>();
    wnd->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    wnd->set_propagate_natural_height();
    wnd->set_propagate_natural_width(false);
    wnd->set_child(_list_view);
    _popup.set_child(*wnd);

    _factory = Gtk::SignalListItemFactory::create();
    _factory->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto label = Gtk::make_managed<Gtk::Label>();
        label->add_css_class("menuitem");
        label->set_valign(Gtk::Align::CENTER);
        label->set_halign(Gtk::Align::START);
        label->set_ellipsize(Pango::EllipsizeMode::END);
        list_item->set_child(*label);
    });
    _factory->signal_bind().connect([this](const Glib::RefPtr<Gtk::ListItem>& list_item) {
        auto obj = list_item->get_item();
        auto& label = dynamic_cast<Gtk::Label&>(*list_item->get_child());
        if (_use_markup) {
            label.set_markup(_get_item_markup(obj));
        }
        else {
            label.set_text(_get_item_label(obj));
        }
    });
    _list_view.set_single_click_activate(true);
    _list_view.set_factory(_factory);

    _key_entry->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
    _key_entry->signal_key_pressed().connect([this](auto keyval, auto keycode, auto modifiers) {
        return on_key_pressed(keyval, modifiers);
    }, false); // Before default handler.
    add_controller(_key_entry);

    auto expression = Gtk::ClosureExpression<bool>::create([this](auto& item) {
        if (_search_text.empty()) return true; // all visible
        auto label = _get_item_label(item);
        return starts_with(label, _search_text);
    });
    _filter->set_expression(expression);

    _popup.signal_show().connect([this] {
        // focus text entry (if not yet done) to allow combobox to react to keyboard events;
        // the popup itself is not focusable to let entry function properly
        _entry.grab_focus_without_selecting();
        // align a popup menu with combobox rather than the menu button
        auto alloc = get_allocation();
        double x = 0, y = 0;
        translate_coordinates(_menu_btn, 0, 0, x, y);
        _popup.set_pointing_to(Gdk::Rectangle(x, y, alloc.get_width(), alloc.get_height()));
    });

    // init all models, so there are no null pointers
    unset_model();

    // connect all signals now that combobox is initialized

    _list_view.signal_activate().connect([this](auto index) {
        if (_update.pending()) return;

        _menu_btn.popdown();
        if (auto text = get_text_item(index); !text.empty()) {
            auto scope = _update.block();
            _entry.set_text(text);
            _signal_value_changed.emit(text);
        }
        reset_filter();
    });

    _entry.signal_changed().connect([this] {
        if (_update.pending()) return;
        // search for a matching text
        _search_text = _entry.get_text();
        refilter();
        _menu_btn.popup();
        select_item(0, false, false);
    });

    _entry.signal_activate().connect([this] {
        if (!_popup.get_visible()) return;
        // accept current entry
        _menu_btn.popdown();
        if (auto text = get_selected_text(); !text.empty()) {
            auto scope = _update.block();
            _entry.set_text(text);
            _signal_value_changed.emit(text);
        }
        reset_filter();
    });

    _box.insert_at_end(*this);
    containerize(*this);
}

void TextComboBox::set_model(
    const Glib::RefPtr<Gio::ListModel>& model,
    const std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)>& get_label_text,
    const std::function<Glib::ustring (const Glib::RefPtr<Glib::ObjectBase>&)>& get_label_markup) {

    // get label text callback or no-op operation
    _get_item_label = get_label_text ? get_label_text : [](auto&) { return Glib::ustring{}; };
    if (get_label_markup) {
        _use_markup = true;
        _get_item_markup = get_label_markup;
    }
    else {
        _use_markup = false;
        _get_item_markup = _get_item_label;
    }

    _model = model;
    _filtered_model = Gtk::FilterListModel::create(model, _filter);
    _selection_model = Gtk::SingleSelection::create(_filtered_model);
    _list_view.set_model(_selection_model);
    reset_filter();
}

void TextComboBox::set_model(Glib::RefPtr<Gtk::StringList> model) {
    set_model(model, [](const auto& ptr){ return std::dynamic_pointer_cast<Gtk::StringObject>(ptr)->get_string(); });
}

void TextComboBox::unset_model() {
    // populate models, so there are no null pointers
    set_model(Gtk::StringList::create({}), {});
}


void TextComboBox::measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural,
                                 int& minimum_baseline, int& natural_baseline) const {

    _box.measure(orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
}

void TextComboBox::size_allocate_vfunc(int width, int height, int baseline) {
    auto alloc = Gtk::Allocation(0, 0, width, height);
    _box.size_allocate(alloc, baseline);

    if (_popup_width <= 0) {
        _popup.set_size_request(width, -1);
        _popup.queue_resize();
    }
}

void TextComboBox::refilter() {
    Gtk::Filter* f = _filter.get();
    gtk_filter_changed(f->gobj(), GtkFilterChange::GTK_FILTER_CHANGE_DIFFERENT);
}

void TextComboBox::reset_filter() {
    _search_text.clear();
    refilter();
}

bool TextComboBox::on_key_pressed(guint keyval, Gdk::ModifierType state) {
    state &= Gtk::Accelerator::get_default_mod_mask();

    switch (keyval) {
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(-1);
            return true;
        }
        else if (state == Gdk::ModifierType::ALT_MASK) {
            _menu_btn.popdown();
            reset_filter();
            return true;
        }
        break;
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(-10);
            return true;
        }
        break;
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(+1);
            return true;
        }
        else if (state == Gdk::ModifierType::ALT_MASK) {
            _menu_btn.popup();
            return true;
        }
        break;
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_next(+10);
            return true;
        }
        break;
    // Home and End keys are used by Entry, so we won't see them.
    //todo: add a modifier?
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_item(0);
            return true;
        }
        break;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            select_item(-1);
            return true;
        }
        break;
    case GDK_KEY_Escape:
        if (state == Gdk::ModifierType::NO_MODIFIER_MASK) {
            if (_popup.get_visible()) {
                _menu_btn.popdown();
                reset_filter();
                return true;
            }
            else if (_defocus) {
                _defocus();
                return true;
            }
        }
        break;
    }

    // key combination is not used
    return false;
}

void TextComboBox::select_next(int delta, bool add_suffix) {
    if (int size = _selection_model->get_n_items(); size > 0) {
        int index = _selection_model->get_selected();
        auto next = std::clamp(index + delta, 0, size - 1);
        select_item(next, add_suffix);
    }
}

void TextComboBox::select_item(int index, bool add_suffix, bool notify) {
    if (index == -1) index = _selection_model->get_n_items();

    if (index >= 0 && index < _selection_model->get_n_items()) {
        _selection_model->set_selected(index);
        _list_view.scroll_to(index);

        if (auto text = get_selected_text(); !text.empty()) {
            if (add_suffix) {
                append_text(text);
            }
            if (notify) {
                _signal_value_changed.emit(text);
            }
        }
    }
}

void TextComboBox::append_text(const Glib::ustring& text) {
    auto scope = _update.block();
    auto str = _entry.get_text();
    _entry.set_text(text);
    if (!_search_text.empty() && starts_with(text, _search_text) && starts_with(str, _search_text)) {
        // keep the old prefix, highlight the rest
        _entry.select_region(_search_text.size(), -1);
    }
}

Glib::ustring TextComboBox::get_text_item(int index) const {
    if (auto item = _selection_model->get_object(index)) {
        return _get_item_label(item);
    }
    return {};
}

Glib::ustring TextComboBox::get_selected_text() const {
    if (auto item = _selection_model->get_selected_item()) {
        return _get_item_label(item);
    }
    return {};
}

int TextComboBox::get_selected() const {
    auto text = _entry.get_text();
    if (text.empty()) return -1;

    // find matching entry, if any
    auto str = text.lowercase();
    auto n = _model->get_n_items();
    for (int i = 0; i < n; ++i) {
        //TODO: use ICU Collator for case-insensitive comparison?
        if (_get_item_label(_model->get_object(i)).lowercase() == str) {
            return i;
        }
    }
    return -1;
}

void TextComboBox::set_selected(int index) {
    auto scope = _update.block();

    // remove filter if any, we need all items
    if (!_search_text.empty()) {
        reset_filter();
    }

    if (index >= 0 && index < _selection_model->get_n_items()) {
        _selection_model->set_selected(index);
        _list_view.scroll_to(index);
        _entry.set_text(get_selected_text());
    }
}

void TextComboBox::set_popup_width(int width) {
    _popup_width = width > 0 ? width : 0;

    if (_popup_width > 0) {
        _popup.set_size_request(width, -1);
    }
    else {
        _popup.set_size_request(-1, -1);
        queue_resize();
    }
}

void TextComboBox::set_defocus_callback(const std::function<void()>& defocus) {
    _defocus = defocus;
}

} // namespace
