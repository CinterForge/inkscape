// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "font-selector.h"

#include <gtkmm/dragsource.h>
#include <gtkmm/grid.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>

#include "libnrtype/font-factory.h"
#include "libnrtype/font-lister.h"
// For updating from selection
#include <glibmm/main.h>

#include "desktop.h"
#include "inkscape.h"
#include "object/sp-text.h"
#include "preferences.h"
#include "util-string/ustring-format.h"

namespace Inkscape::UI::Widget {

std::unique_ptr<FontSelectorInterface> FontSelector::create_font_selector() {
    return std::make_unique<FontSelector>();
}

FontSelector::FontSelector(bool with_size, bool with_variations)
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , family_frame(_("Font family"))
    , style_frame(C_("Font selector", "Style"))
    , size_label(_("Font size"))
    , signal_block(false)
{
    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    Glib::RefPtr<Gtk::TreeModel> model = font_lister->get_font_list();

    // Font family
    family_treecolumn.pack_start (family_cell, false);
    int total = model->children().size();
    int height = 30;
    if (total > 1000) {
        height = 30000/total;
        g_warning("You have a huge number of font families (%d), "
                    "and Cairo is limiting the size of widgets you can draw.\n"
                    "Your preview cell height is capped to %d.",
                    total, height);
        // hope we dont need a forced height because now pango line height
        // not add data outside parent rendered expanding it so no naturall cells become over 30 height
        family_cell.set_fixed_size(-1, height);
    } else {
#if !PANGO_VERSION_CHECK(1,50,0)
    family_cell.set_fixed_size(-1, height);
#endif
    }
    family_treecolumn.add_attribute (family_cell, "text", 0);
    family_treecolumn.set_fixed_width(160); // limit minimal width to keep entire dialog narrow; column can still grow
    family_treecolumn.set_cell_data_func (family_cell, &font_lister_cell_data_func_markup);
    family_treeview.set_row_separator_func (&font_lister_separator_func);
    family_treeview.set_model(model);
    family_treeview.set_name ("FontSelector: Family");
    family_treeview.set_headers_visible (false);
    family_treeview.append_column (family_treecolumn);

    family_scroll.set_policy (Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    family_scroll.set_child(family_treeview);

    family_frame.set_hexpand (true);
    family_frame.set_vexpand (true);
    family_frame.set_child(family_scroll);

    // Style
    style_treecolumn.pack_start (style_cell, false);
    style_treecolumn.add_attribute (style_cell, "text", 0);
    style_treecolumn.set_cell_data_func (style_cell, sigc::mem_fun(*this, &FontSelector::style_cell_data_func));
    style_treecolumn.set_title ("Face");
    style_treecolumn.set_resizable (true);

    style_treeview.set_model (font_lister->get_style_list());
    style_treeview.set_name ("FontSelectorStyle");
    style_treeview.set_headers_visible (false);
    // Moved to tooltip to save space, but still useful for debugging.
    // style_treeview.append_column ("CSS", font_lister->font_style_list.cssStyle);
    style_treeview.append_column (style_treecolumn);
    style_treeview.set_tooltip_column(font_lister->font_style_list.cssStyle.index());

    style_treeview.get_column(0)->set_resizable (true);

    style_scroll.set_policy (Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    style_scroll.set_child(style_treeview);

    style_frame.set_hexpand (true);
    style_frame.set_vexpand (true);
    style_frame.set_child(style_scroll);

    // Size
    size_selector.setSize(sp_style_css_size_px_to_units(18, size_selector.getUnit()));

    // Font Variations
    font_variations.set_vexpand (true);
    font_variations_scroll.set_policy (Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    font_variations_scroll.set_child(font_variations);

    // Grid
    set_name ("FontSelectorGrid");
    set_spacing(4);

    auto const grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_column_homogeneous(true);
    grid->set_column_spacing(4);
    grid->attach(family_frame, 0, 0, 1, 1);
    grid->attach(style_frame, 1, 0, 1, 1);
    append(*grid);

    if (with_size) { // Glyph panel does not use size.
        auto const size_grid = Gtk::make_managed<Gtk::Grid>();
        size_grid->set_column_spacing(4);
        size_grid->attach(size_label, 0, 0, 1, 1);
        size_grid->attach(size_selector, 1, 0, 1, 1);
        append(*size_grid);
    }
    if (with_variations) { // Glyphs panel does not use variations.
        append(font_variations_scroll);
    }

    update_variations(font_lister->get_fontspec());

    // For drag and drop.
    auto const drag = Gtk::DragSource::create();
    drag->signal_prepare().connect(sigc::mem_fun(*this, &FontSelector::on_drag_prepare), false); // before
    drag->signal_drag_begin().connect([this, &drag = *drag](auto &&...args) { on_drag_begin(drag, args...); });
    family_treeview.add_controller(drag);

    // Add signals
    family_treeview.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &FontSelector::on_family_changed));
    style_treeview.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &FontSelector::on_style_changed));
    size_selector.signal_size_changed().connect(sigc::mem_fun(*this, &FontSelector::on_size_changed));
    font_variations.connectChanged(sigc::mem_fun(*this, &FontSelector::on_variations_changed));
    family_treeview.signal_realize().connect(sigc::mem_fun(*this, &FontSelector::on_realize_list));

    font_variations_scroll.set_vexpand(true);

    // Initialize font family lists. (May already be done.) Should be done on document change.
    font_lister->update_font_list(SP_ACTIVE_DESKTOP->getDocument());
}

void FontSelector::on_realize_list() {
    family_treecolumn.set_cell_data_func (family_cell, &font_lister_cell_data_func);
    _idle_connection = Glib::signal_idle().connect(sigc::mem_fun(*this, &FontSelector::set_cell_markup));
}

bool FontSelector::set_cell_markup()
{
    family_treeview.set_visible(false);
    family_treecolumn.set_cell_data_func (family_cell, &font_lister_cell_data_func_markup);
    family_treeview.set_visible(true);
    return false;
}

void FontSelector::hide_others()
{
    style_frame.set_visible(false);
    size_label.set_visible(false);
    size_selector.set_visible(false);
    font_variations_scroll.set_visible(false);
    font_variations_scroll.set_vexpand(false);
}

// TODO: Dropping doesnʼt seem to be implemented anywhere
void FontSelector::on_drag_begin(Gtk::DragSource &source,
                                 Glib::RefPtr<Gdk::Drag> const &drag)
{
    // Get the current collection.
    Glib::RefPtr<Gtk::TreeSelection> selection = family_treeview.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    Gtk::TreePath path(iter);
    auto paintable = family_treeview.create_row_drag_icon(path);
    source.set_icon(paintable, 0, 0);
}

Glib::RefPtr<Gdk::ContentProvider> FontSelector::on_drag_prepare(double /*x*/, double /*y*/)
{
    Inkscape::FontLister *font_lister = Inkscape::FontLister::get_instance();
    Glib::ustring family_name = font_lister->get_dragging_family();

    Glib::Value<Glib::ustring> value;
    value.init(G_TYPE_STRING);
    value.set(family_name);
    return Gdk::ContentProvider::create(value);
}

// Update GUI.
// We keep a private copy of the style list as the font-family in widget is only temporary
// until the "Apply" button is set so the style list can be different from that in
// FontLister.
void FontSelector::update_font()
{
    signal_block = true;

    auto font_lister = Inkscape::FontLister::get_instance();
    Gtk::TreePath path;
    Glib::ustring family = font_lister->get_font_family();
    Glib::ustring style  = font_lister->get_font_style();

    // Set font family
    try {
        path = font_lister->get_row_for_font(family).get_iter();
    } catch (FontLister::Exception) {
        std::cerr << "FontSelector::update_font: Couldn't find row for font-family: "
                  << family.raw() << std::endl;
        path.clear();
        path.push_back(0);
    }

    Gtk::TreePath currentPath;
    Gtk::TreeViewColumn *currentColumn;
    family_treeview.get_cursor(currentPath, currentColumn);
    if (currentPath.empty() || !font_lister->is_path_for_font(currentPath, family)) {
        family_treeview.set_cursor(path);
        family_treeview.scroll_to_row(path);
    }

    // Get font-lister style list for selected family
    auto const row = *family_treeview.get_model()->get_iter(path);
    auto styles = row.get_value(font_lister->font_list.styles);

    // Copy font-lister style list to private list store, searching for match.
    Gtk::TreeModel::iterator match;
    auto local_style_list_store = Gtk::ListStore::create(font_lister->font_style_list);

    for (auto const &s : *styles) {
        auto srow = *local_style_list_store->append();
        srow[font_lister->font_style_list.cssStyle] = s.css_name;
        srow[font_lister->font_style_list.displayStyle] = s.display_name;
        if (style == s.css_name) {
            match = srow.get_iter();
        }
    }

    // Attach store to tree view and select row.
    style_treeview.set_model(local_style_list_store);
    if (match) {
        style_treeview.get_selection()->select(match);
    }

    Glib::ustring fontspec = font_lister->get_fontspec();
    update_variations(fontspec);

    signal_block = false;
}

void FontSelector::unset_model()
{
    family_treeview.unset_model();
}

void FontSelector::set_model()
{
    Inkscape::FontLister* font_lister = Inkscape::FontLister::get_instance();
    Glib::RefPtr<Gtk::TreeModel> model = font_lister->get_font_list();
    family_treeview.set_model(model);
}

// If use_variations is true (default), we get variation values from variations widget otherwise we
// get values from CSS widget (we need to be able to keep the two widgets synchronized both ways).
Glib::ustring
FontSelector::get_fontspec(bool use_variations) {

    // Build new fontspec from GUI settings
    Glib::ustring family = "Sans";  // Default...family list may not have been constructed.
    Gtk::TreeModel::iterator iter = family_treeview.get_selection()->get_selected();
    if (iter) {
        (*iter).get_value(0, family);
    }

    Glib::ustring style = "Normal";
    iter = style_treeview.get_selection()->get_selected();
    if (iter) {
        (*iter).get_value(0, style);
    }

    if (family.empty()) {
        std::cerr << "FontSelector::get_fontspec: empty family!" << std::endl;
    }

    if (style.empty()) {
        std::cerr << "FontSelector::get_fontspec: empty style!" << std::endl;
    }

    Glib::ustring fontspec = family + ", ";

    if (use_variations) {
        // Clip any font_variation data in 'style' as we'll replace it.
        auto pos = style.find('@');
        if (pos != Glib::ustring::npos) {
            style.erase (pos, style.length()-1);
        }

        Glib::ustring variations = font_variations.get_pango_string();

        if (variations.empty()) {
            fontspec += style;
        } else {
            fontspec += style;
            fontspec += " ";
            fontspec += variations;
        }
    } else {
        fontspec += style;
    }

    return fontspec;
}

void
FontSelector::style_cell_data_func(Gtk::CellRenderer * const renderer,
                                   Gtk::TreeModel::const_iterator const &iter)
{
    Glib::ustring family = "Sans";  // Default...family list may not have been constructed.
    auto const iter_family = family_treeview.get_selection()->get_selected();
    if (iter_family) {
        (*iter_family).get_value(0, family);
    }

    Glib::ustring style = "Normal";
    (*iter).get_value(1, style);
    Glib::ustring css = "Normal";
    (*iter).get_value(0, css);

    Glib::ustring style_escaped  = Glib::Markup::escape_text( style );
    Glib::ustring font_desc = Glib::Markup::escape_text( family + ", " + css );
    Glib::ustring markup;

    markup = "<span font='" + font_desc + "'>" + style_escaped + "</span>";
    // std::cout << "CSS: " << css << "   Markup: " << markup << std::endl;
    renderer->set_property("markup", markup);
}

// Callbacks

// Need to update style list
void
FontSelector::on_family_changed() {

    if (signal_block) return;
    signal_block = true;

    Glib::RefPtr<Gtk::TreeModel> model;
    Gtk::TreeModel::iterator iter = family_treeview.get_selection()->get_selected(model);

    if (!iter) {
        // This can happen just after the family list is recreated.
        signal_block = false;
        return;
    }

    auto fontlister = Inkscape::FontLister::get_instance();
    fontlister->ensureRowStyles(iter);

    Gtk::TreeModel::Row row = *iter;

    // Get family name
    Glib::ustring family;
    row.get_value(0, family);

    fontlister->set_dragging_family(family);

    // Get style list.
    auto styles = row.get_value(fontlister->font_list.styles);

    // Find best style match for selected family with current style (e.g. of selected text).
    Glib::ustring style = fontlister->get_font_style();
    Glib::ustring best  = fontlister->get_best_style_match (family, style);

    // Create are own store of styles for selected font-family (the font-family selected
    // in the dialog may not be the same as stored in the font-lister class until the
    // "Apply" button is triggered).
    Gtk::TreeModel::iterator it_best;
    FontLister::FontStyleListClass FontStyleList;
    Glib::RefPtr<Gtk::ListStore>  local_style_list_store = Gtk::ListStore::create(FontStyleList);

    // Build list and find best match.
    for (auto const &s : *styles) {
        auto srow = *local_style_list_store->append();
        srow[FontStyleList.cssStyle] = s.css_name;
        srow[FontStyleList.displayStyle] = s.display_name;
        if (best == s.css_name) {
            it_best = srow.get_iter();
        }
    }

    // Attach store to tree view and select row.
    style_treeview.set_model (local_style_list_store);
    if (it_best) {
        style_treeview.get_selection()->select (it_best);
    }

    // variation sliders are refreshed immediately,
    // rather than waiting for an Apply.
    update_variations(get_fontspec(false));

    signal_block = false;

    // Let world know
    changed_emit();
}

void
FontSelector::on_style_changed() {
    if (signal_block) return;

    // Update variations widget if new style selected from style widget.
    signal_block = true;
    Glib::ustring fontspec = get_fontspec( false );
    update_variations(fontspec);
    signal_block = false;

    // Let world know
    changed_emit();
}

void
FontSelector::on_size_changed(double size, int unit) {

    if (signal_block) return;

    changed_emit();
}

void
FontSelector::on_variations_changed() {

    if (signal_block) return;

    // Let world know
    changed_emit();
}

void
FontSelector::changed_emit() {
    signal_block = true;
    _signal_set_default.emit();
    if (initial) {
        initial = false;
        family_treecolumn.unset_cell_data_func (family_cell);
        family_treecolumn.set_cell_data_func (family_cell, &font_lister_cell_data_func);
        _idle_connection = Glib::signal_idle().connect(sigc::mem_fun(*this, &FontSelector::set_cell_markup));
    }
    signal_block = false;
}

void FontSelector::update_variations(const Glib::ustring& fontspec) {
    font_variations.update(fontspec);

    // Check if there are any variations available; if not, don't expand font_variations_scroll
    bool hasContent = font_variations.variations_present();
    font_variations_scroll.set_visible(hasContent);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
