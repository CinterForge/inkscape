// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Font selection widgets
 */
/*
 * Authors:
 *   Chris Lahey <clahey@ximian.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   See Git history
 *
 * Copyright (C) 1999-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2013 Tavmjong Bah
 * Copyright (C) 2018+ Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef FONT_LISTER_H
#define FONT_LISTER_H

#include <memory>
#include <set>
#include <vector>
#include <unordered_map>

#include <glibmm/ustring.h>
#include <glibmm/stringutils.h> // For strescape()

#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treepath.h>
#include <pangomm/fontfamily.h>

#include "libnrtype/font-utils.h" // canonize_fontspec

inline constexpr int FONT_FAMILIES_GROUP_SIZE = 30;

class SPObject;
class SPDocument;
class SPCSSAttr;
class SPStyle;
class StyleNames;

namespace Gtk {
class CellRenderer;
} // namespace Gtk

namespace Inkscape {

/**
 *  This class enumerates fonts using libnrtype into reusable data stores and
 *  allows for random access to the font-family list and the font-style list.
 *  Setting the font-family updates the font-style list. "Style" in this case
 *  refers to everything but family and size (e.g. italic/oblique, weight).
 *
 *  This class handles font-family lists and fonts that are not on the system,
 *  where there is not an entry in the fontInstanceMap.
 *
 *  This class uses the idea of "font_spec". This is a plain text string as used by
 *  Pango. It is similar to the CSS font shorthand except that font-family comes
 *  first and in this class the font-size is not used.
 *
 *  This class uses the FontFactory class to get a list of system fonts
 *  and to find best matches via Pango. The Pango interface is only setup
 *  to deal with fonts that are on the system so care must be taken. For
 *  example, best matches should only be done with the first font-family
 *  in a font-family list. If the first font-family is not on the system
 *  then a generic font-family should be used (sans-serif -> Sans).
 *
 *  This class is used by the UI interface (text-toolbar, font-select, etc.).
 *  Those items can change the selected font family and style here. When that
 *  happens. this class emits a signal for those items to update their displayed
 *  values.
 *
 *  This class is a singleton (one instance per Inkscape session). Since fonts
 *  used in a document are added to the list, there really should be one
 *  instance per document.
 *
 *  "Font" includes family and style. It should not be used when one
 *  means font-family.
 */

class FontLister
{
public:
    enum class Exception
    {
        FAMILY_NOT_FOUND,
        STYLE_NOT_FOUND
    };

    using Styles = std::vector<StyleNames>;

    /**
     * GtkTreeModelColumnRecord for the font-family list Gtk::ListStore
     */
    struct FontListClass : Gtk::TreeModelColumnRecord
    {
        /// Family name
        Gtk::TreeModelColumn<Glib::ustring> family;

        /// Styles for each family name.
        Gtk::TreeModelColumn<std::shared_ptr<Styles>> styles;

        /// Whether font is on system
        Gtk::TreeModelColumn<bool> onSystem;
        
        /**
         * Not actually a column.
         * Necessary for quick initialization of FontLister;
         * we initially store the pango family and if the
         * font style is actually used we'll cache it in
         * %styles.
         */
        Gtk::TreeModelColumn<Glib::RefPtr<Pango::FontFamily>> pango_family;
        
        FontListClass()
        {
            add(family);
            add(styles);
            add(onSystem);
            add(pango_family);
        }
    };

    FontListClass font_list;

    struct FontStyleListClass : Gtk::TreeModelColumnRecord
    {
        /// Column containing the styles as Font designer used.
        Gtk::TreeModelColumn<Glib::ustring> displayStyle;

        /// Column containing the styles in CSS/Pango format.
        Gtk::TreeModelColumn<Glib::ustring> cssStyle;

        FontStyleListClass()
        {
            add(cssStyle);
            add(displayStyle);
        }
    };

    FontStyleListClass font_style_list;

    /**
     * The list of fonts, sorted by the order they will appear in the UI.
     * Also used to give log-time access to each font's PangoFontFamily, owned by the FontFactory.
     */
    std::map<std::string, Glib::RefPtr<Pango::FontFamily>> pango_family_map;

    /** 
     * @return the ListStore with the family names
     *
     * The ListStore is ready to be used after class instantiation
     * and should not be modified.
     */
    Glib::RefPtr<Gtk::ListStore> const &get_font_list() const { return font_list_store; }

    /**
     * @return the ListStore with the styles
     */
    Glib::RefPtr<Gtk::ListStore> const &get_style_list() const { return style_list_store; }

    /** 
     * Inserts a font family or font-fallback list (for use when not
     *  already in document or on system).
     */
    void insert_font_family(Glib::ustring const &new_family);

    int add_document_fonts_at_top(SPDocument *document);

    /**
     * Updates font list to include fonts in document.
     */
    void update_font_list(SPDocument *document);

public:
    static Inkscape::FontLister *get_instance();

    /**
     * Takes a hand written font spec and returns a Pango generated one in
     *  standard form.
     */

    /**
     * Functions to display the search results in the font list.
     */
    bool find_string_case_insensitive(std::string const &text, std::string const &pat);
    void show_results(Glib::ustring const &search_text);
    void apply_collections(std::set <Glib::ustring> &selected_collections);
    void set_dragging_family(Glib::ustring const &new_family);

    /**
     * Gets font-family and style from fontspec.
     *  font-family and style returned.
     */
    std::pair<Glib::ustring, Glib::ustring> ui_from_fontspec(Glib::ustring const &fontspec) const;

    /**
     * Sets font-family and style after a selection change.
     *  New font-family and style returned.
     */
    std::pair<Glib::ustring, Glib::ustring> selection_update();

    /** 
     * Sets current_fontspec, etc. If check is false, won't
     *  try to find best style match (assumes style in fontspec
     *  valid for given font-family).
     */
    void set_fontspec(Glib::ustring const &fontspec, bool check = true);

    Glib::ustring get_fontspec() const {
        return Inkscape::canonize_fontspec(current_family + ", " + current_style);
    }

    /**
     * Changes font-family, updating style list and attempting to find
     *  closest style to current_style style (if check_style is true).
     *  New font-family and style returned.
     *  Does NOT update current_family and current_style.
     *  (For potential use in font-selector which doesn't update until
     *  "Apply" button clicked.)
     */
    std::pair<Glib::ustring, Glib::ustring> new_font_family(Glib::ustring const &family, bool check_style = true);

    /** 
     * Sets font-family, updating style list and attempting
     *  to find closest style to old current_style.
     *  New font-family and style returned.
     *  Updates current_family and current_style.
     *  Calls new_font_family().
     *  (For use in text-toolbar where update is immediate.)
     */
    std::pair<Glib::ustring, Glib::ustring> set_font_family(Glib::ustring const &family, bool check_style = true,
                                                            bool emit = true);

    /**
     * Sets font-family from row in list store.
     *  The row can be used to determine if we are in the
     *  document or system part of the font-family list.
     *  This is needed to handle scrolling through the
     *  font-family list correctly.
     *  Calls set_font_family().
     */
    std::pair<Glib::ustring, Glib::ustring> set_font_family(int row, bool check_style = true, bool emit = true);

    Glib::ustring const &get_font_family() const
    {
        return current_family;
    }

    Glib::ustring const &get_dragging_family() const
    {
        return dragging_family;
    }

    int get_font_family_row() const
    {
        return current_family_row;
    }

    /**
     * Sets style. Does not validate style for family.
     */
    void set_font_style(Glib::ustring style, bool emit = true);

    Glib::ustring const &get_font_style() const
    {
        return current_style;
    }

    Glib::ustring fontspec_from_style(SPStyle *style) const;

    /**
     * Fill css using given fontspec (doesn't need to be member function).
     */
    void fill_css(SPCSSAttr *css, Glib::ustring fontspec = {});

    Gtk::TreeModel::Row get_row_for_font() { return get_row_for_font(current_family); }

    Gtk::TreeModel::Row get_row_for_font(Glib::ustring const &family);

    Gtk::TreePath get_path_for_font(Glib::ustring const &family);

    bool is_path_for_font(Gtk::TreePath path, Glib::ustring family);

    Gtk::TreeModel::Row get_row_for_style() { return get_row_for_style(current_style); }

    Gtk::TreeModel::Row get_row_for_style(Glib::ustring const &style);

    Gtk::TreePath get_path_for_style(Glib::ustring const &style);

    std::pair<Gtk::TreePath, Gtk::TreePath> get_paths(Glib::ustring const &family, Glib::ustring const &style);

    /**
     * Return best style match for new font given style for old font.
     */
    Glib::ustring get_best_style_match(Glib::ustring const &family, Glib::ustring const &style);

    /**
     * Ensures the style list for a particular family has been created.
     */
    void ensureRowStyles(Gtk::TreeModel::iterator iter);

    /**
     * Get markup for font-family.
     */
    Glib::ustring get_font_family_markup(Gtk::TreeModel::const_iterator const &iter) const;

    /**
     * Let users of FontLister know to update GUI.
     * This is to allow synchronization of changes across multiple widgets.
     * Handlers should block signals.
     * Input is fontspec to set.
     */
    sigc::connection connectUpdate(sigc::slot<void ()> slot) {
        return update_signal.connect(slot);
    }
    sigc::connection connectNewFonts(sigc::slot<void ()> slot) {
        return new_fonts_signal.connect(slot);
    }

    bool blocked() const { return block; }

    int get_font_families_size() const { return pango_family_map.size(); }
    bool font_installed_on_system(Glib::ustring const &font) const;

    void init_font_families(int group_offset = -1, int group_size = -1);
    void init_default_styles();
    std::pair<bool, std::string> get_font_count_label() const;

private:
    FontLister();
    ~FontLister();

    void update_font_data_recursive(SPObject& r, std::map<Glib::ustring, std::set<Glib::ustring>> &font_data);

    void font_family_row_update(int start = 0);

    Glib::RefPtr<Gtk::ListStore> font_list_store;
    Glib::RefPtr<Gtk::ListStore> style_list_store;

    /**
     * Info for currently selected font (what is shown in the UI).
     *  May include font-family lists and fonts not on system.
     */
    int current_family_row = 0;
    Glib::ustring current_family = "sans-serif";
    Glib::ustring dragging_family;
    Glib::ustring current_style = "Normal";

    /**
     * If a font-family is not on system, this list of styles is used.
     */
    std::shared_ptr<Styles> default_styles;

    bool block = false;
    void emit_update();
    sigc::signal<void ()> update_signal;
    sigc::signal<void ()> new_fonts_signal;
};

} // namespace Inkscape

// Helper functions
bool font_lister_separator_func(Glib::RefPtr<Gtk::TreeModel> const &model,
                                Gtk::TreeModel::const_iterator const &iter);

void font_lister_cell_data_func(Gtk::CellRenderer *renderer,
                                Gtk::TreeModel::const_iterator const &iter);

void font_lister_cell_data_func_markup(Gtk::CellRenderer *renderer,
                                       Gtk::TreeModel::const_iterator const &iter);

void font_lister_cell_data_func2(Gtk::CellRenderer &cell,
                                 Gtk::TreeModel::const_iterator const &iter,
                                 bool with_markup);

void font_lister_style_cell_data_func(Gtk::CellRenderer *renderer,
                                      Gtk::TreeModel::const_iterator const &iter);

#endif // FONT_LISTER_H

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
