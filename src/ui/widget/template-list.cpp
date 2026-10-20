// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "template-list.h"

#include <glib/gi18n.h>
#include <glibmm/markup.h>
#include <glibmm/miscutils.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/numericsorter.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/sortlistmodel.h>

#include "document.h"
#include "inkscape-application.h"
#include "io/resource.h"
#include "ui/builder-utils.h"
#include "ui/svg-renderer.h"
#include "ui/util.h"

using namespace Inkscape::IO::Resource;
using Inkscape::Extension::TemplatePreset;

namespace Inkscape::UI::Widget {

struct TemplateList::TemplateItem : public Glib::Object {
    Glib::ustring name;
    Glib::ustring label;
    Glib::ustring tooltip;
    Glib::RefPtr<Gdk::Texture> icon;
    Glib::ustring key;
    int priority;
    Glib::ustring category;

    static Glib::RefPtr<TemplateItem> create(const Glib::ustring& name, const Glib::ustring& label, const Glib::ustring& tooltip,
        Glib::RefPtr<Gdk::Texture> icon, Glib::ustring key, int priority, const Glib::ustring& category) {

        auto item = Glib::make_refptr_for_instance<TemplateItem>(new TemplateItem());
        item->name = name;
        item->label = label;
        item->tooltip = tooltip;
        item->icon = icon;
        item->key = key;
        item->priority = priority;
        item->category = category;
        return item;
    }
private:
    TemplateItem() = default;
};


TemplateList::TemplateList(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Stack(cobject)
{
}

static Glib::ustring all_templates = NC_("TemplateCategory", "All templates");

/**
 * Initialise this template list with categories and icons
 */
void TemplateList::init(Inkscape::Extension::TemplateShow mode, AddPage add_page, bool allow_unselect)
{
    // same width for all items
    set_hhomogeneous();
    // height can vary per row
    set_vhomogeneous(false);
    // track page switching
    property_visible_child_name().signal_changed().connect([this]() {
        _signal_switch_page.emit(get_visible_child_name());
    });

    std::map<std::string, Glib::RefPtr<Gio::ListStore<TemplateItem>>> stores;

    Inkscape::Extension::DB::TemplateList extensions;
    Inkscape::Extension::db.get_template_list(extensions);

    Glib::RefPtr<Gio::ListStore<TemplateItem>> all;
    if (add_page == All) {
        all = generate_category(all_templates, allow_unselect);
    }

    int group = 0;
    for (auto tmod : extensions) {
        for (auto preset : tmod->get_presets(mode)) {
            auto const &cat = preset->get_category();
            if (add_page == Custom && cat != "Custom") continue;

            if (auto it = stores.lower_bound(cat);
                it == stores.end() || it->first != cat)
            {
                try {
                    group += 10000;
                    it = stores.emplace_hint(it, cat, generate_category(cat, allow_unselect));
                    it->second->remove_all();
                } catch (UIBuilderError const& error) {
                    g_error("Error building templates %s\n", error.what());
                    return;
                }

                if (add_page == Custom) {
                    // add new template placeholder
                    auto const filepath = Glib::build_filename("icons", "custom.svg");
                    auto const fullpath = get_filename(TEMPLATES, filepath.c_str(), false, true);
                    auto icon = to_texture(icon_to_pixbuf(fullpath, get_scale_factor()));
                    auto templ = TemplateItem::create(
                        Glib::Markup::escape_text(_("<new template>")),
                        "", "", icon, "-new-template-", -1, cat
                    );
                    stores[cat]->append(templ);
                }
            }

            auto& name = preset->get_name();
            auto& desc = preset->get_description();
            auto& label = preset->get_label();

            auto tooltip = _(desc.empty() ? name.c_str() : desc.c_str());
            auto trans_label = label.empty() ? "" : _(label.c_str());
            auto icon = to_texture(icon_to_pixbuf(preset->get_icon_path(), get_scale_factor()));

            auto templ = TemplateItem::create(
                Glib::Markup::escape_text(_(name.c_str())),
                Glib::Markup::escape_text(trans_label),
                Glib::Markup::escape_text(tooltip),
                icon, preset->get_key(), group + preset->get_sort_priority(),
                cat
            );
            stores[cat]->append(templ);
            if (all) {
                all->append(templ);
            }
        }
    }

    refilter(_search_term);

    if (allow_unselect) {
        reset_selection();
    }
}

/**
 * Turn the requested template icon name into a pixbuf
 */
Cairo::RefPtr<Cairo::ImageSurface> TemplateList::icon_to_pixbuf(std::string const &path, int scale)
{
    // TODO: cache to filesystem. This function is a major bottleneck for startup time (ca. 1 second)!
    // The current memory-based caching only catches the case where multiple templates share the same icon.
    static std::map<std::string, Cairo::RefPtr<Cairo::ImageSurface>> cache;
    if (path.empty()) {
        return {};
    }
    if (cache.contains(path)) {
        return cache[path];
    }
    Inkscape::svg_renderer renderer(path.c_str());
    auto result = renderer.render_surface(scale * 0.7); // reduced template icon size to fit more in a dialog
    cache[path] = result;
    return result;
}

sigc::signal<void (const Glib::ustring&)> TemplateList::signal_switch_page() {
    return _signal_switch_page;
}

/**
 * Generate a new category with the given label and return it's list store.
 */
Glib::RefPtr<Gio::ListStore<TemplateList::TemplateItem>> TemplateList::generate_category(std::string const &label, bool allow_unselect)
{
    auto builder = create_builder("widget-new-from-template.ui");
    auto& container = get_widget<Gtk::ScrolledWindow>(builder, "container");
    auto& icons     = get_widget<Gtk::GridView>      (builder, "iconview");

    auto store = Gio::ListStore<TemplateItem>::create();
    auto sorter = Gtk::NumericSorter<int>::create(Gtk::ClosureExpression<int>::create([this](auto& item){
        auto ptr = std::dynamic_pointer_cast<TemplateItem>(item);
        return ptr ? ptr->priority : 0;
    }));
    auto sorted_model = Gtk::SortListModel::create(store, sorter);
    if (!_filter) {
        _filter = Gtk::BoolFilter::create({});
    }
    auto filtered_model = Gtk::FilterListModel::create(sorted_model, _filter);
    auto selection_model = Gtk::SingleSelection::create(filtered_model);
    if (allow_unselect) {
        selection_model->set_can_unselect();
        selection_model->set_autoselect(false);
    }
    auto factory = IconViewItemFactory::create([](auto& ptr) -> IconViewItemFactory::ItemData {
        auto tmpl = std::dynamic_pointer_cast<TemplateItem>(ptr);
        if (!tmpl) return {};

        auto label = tmpl->label.empty() ? tmpl->name :
            tmpl->name + "<small><span line_height='0.5'>\n\n</span><span alpha='60%'>" + tmpl->label + "</span></small>";
        return { .label_markup = label, .image = tmpl->icon, .tooltip = tmpl->tooltip };
    });
    icons.set_max_columns(30);
    icons.set_tab_behavior(Gtk::ListTabBehavior::ITEM); // don't steal the tab key
    icons.set_factory(factory->get_factory());
    icons.set_model(selection_model);

    // This packing keeps the Gtk widget alive, beyond the builder's lifetime
    add(container, label, get_category_label(label));

    _categories.emplace_back(label);

    selection_model->signal_selection_changed().connect([this](auto pos, auto count){
        _item_selected_signal.emit(count > 0 ? static_cast<int>(pos) : -1);
    });
    icons.signal_activate().connect([this](auto pos){
        _item_activated_signal.emit();
    });

    _factory.emplace_back(std::move(factory));
    return store;
}

Glib::ustring TemplateList::get_category_label(Glib::ustring const &cat) const
{
    return g_dpgettext2(nullptr, "TemplateCategory", cat.c_str());
}

/**
 * Returns true if the template list has a visible, selected preset.
 */
bool TemplateList::has_selected_preset()
{
    return !!get_selected_preset();
}

bool TemplateList::has_selected_new_template() {
    if (auto item = get_selected_item()) {
        return item->key == "-new-template-";
    }
    return false;
}

Glib::RefPtr<TemplateList::TemplateItem> TemplateList::get_selected_item(Gtk::Widget* current_page) {
    if (auto iconview = get_iconview(current_page ? current_page : get_visible_child())) {
        auto sel = std::dynamic_pointer_cast<Gtk::SingleSelection>(iconview->get_model());
        auto ptr = sel->get_selected_item();
        if (auto item = std::dynamic_pointer_cast<TemplateList::TemplateItem>(ptr)) {
            return item;
        }
    }
    return nullptr;
}

/**
 * Returns the selected template preset, if one is not selected returns nullptr.
 */
std::shared_ptr<TemplatePreset> TemplateList::get_selected_preset(Gtk::Widget* current_page)
{
    if (auto item = get_selected_item(current_page)) {
        return Extension::Template::get_any_preset(item->key);
    }
    return nullptr;
}

/**
 * Create a new document based on the selected item and return.
 */
SPDocument *TemplateList::new_document(Gtk::Widget* current_page)
{
    auto app = InkscapeApplication::instance();
    if (auto preset = get_selected_preset(current_page)) {
        if (auto doc = preset->new_from_template()) {
            // TODO: Add memory to remember this preset for next time.
            return app->document_add(std::move(doc));
        } else {
            // Cancel pressed in options box.
            return nullptr;
        }
    }
    // Fallback to the default template (already added)!
    return app->document_new();
}

// Show page by its name
void TemplateList::show_page(const Glib::ustring& name) {
    set_visible_child(name);
    refilter(_search_term);
}

// callback to check if template should be visible
bool TemplateList::is_item_visible(const Glib::RefPtr<Glib::ObjectBase>& item, const Glib::ustring& search) const {
    auto ptr = std::dynamic_pointer_cast<TemplateItem>(item);
    if (!ptr) return false;

    const auto& templ = *ptr;

    if (search.empty()) return true;

    // filter by name and label
    return templ.label.lowercase().find(search) != Glib::ustring::npos ||
           templ.name.lowercase().find(search) != Glib::ustring::npos;
}

// filter list of visible templates down to those that contain given search string in their name or label
void TemplateList::filter(Glib::ustring search) {
    _search_term = search;
    refilter(search);
}

// set keyboard focus on template list
void TemplateList::focus() {
    if (auto iconview = get_iconview(get_visible_child())) {
        iconview->grab_focus();
    }
}

void TemplateList::refilter(Glib::ustring search) {
    // When a new expression is set in the BoolFilter, it emits signal_changed(),
    // and the FilterListModel re-evaluates the filter.
    search = search.lowercase();
    auto expression = Gtk::ClosureExpression<bool>::create([this, search](auto& item){ return is_item_visible(item, search); });
    // filter results
    _filter->set_expression(expression);
}

/**
 * Reset the selection, forcing the use of the default template.
 */
void TemplateList::reset_selection(Gtk::Widget* current_page)
{
    // TODO: Add memory here for the new document default (see new_document).
    for (auto &widget : UI::children(current_page ? *current_page : *this)) {
        if (auto iconview = get_iconview(&widget)) {
            auto sel = std::dynamic_pointer_cast<Gtk::SingleSelection>(iconview->get_model());
            sel->unselect_all();
        }
    }
}

/**
 * Returns the internal iconview for the given widget.
 */
Gtk::GridView *TemplateList::get_iconview(Gtk::Widget *widget)
{
    if (!widget) return nullptr;

    for (auto &child : UI::children(*widget)) {
        if (auto iconview = get_iconview(&child)) {
            return iconview;
        }
    }

    return dynamic_cast<Gtk::GridView *>(widget);
}

} // namespace Inkscape::UI::Widget

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
