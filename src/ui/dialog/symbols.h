// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Symbols dialog
 */
/* Authors:
 *   Tavmjong Bah, Martin Owens
 *
 * Copyright (C) 2012 Tavmjong Bah
 *               2013 Martin Owens
 *               2017 Jabiertxo Arraiza
 *               2023 Mike Kowalski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_DIALOG_SYMBOLS_H
#define INKSCAPE_UI_DIALOG_SYMBOLS_H

#include <giomm/liststore.h>
#include <gtkmm/boolfilter.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/gridview.h>
#include <gtkmm/singleselection.h>

#include <boost/compute/detail/lru_cache.hpp>  // for lru_cache

#include <gtkmm/cellrendererpixbuf.h>          // for CellRendererPixbuf
#include <gtkmm/liststore.h>                   // for ListStore
#include <gtkmm/treemodelfilter.h>             // for TreeModelFilter
#include <gtkmm/treemodelsort.h>               // for TreeModelSort

#include "display/drawing.h"
#include "ui/dialog/dialog-base.h"
#include "ui/iconview-item-factory.h"
#include "ui/operation-blocker.h"

namespace Gtk {
class Box;
class Builder;
class Button;
class CheckButton;
class IconView;
class Image;
class Label;
class MenuButton;
class Overlay;
class Scale;
class ScrolledWindow;
class SearchEntry2;
} // namespace Gtk

class SPDesktop;
class SPObject;
class SPSymbol;
class SPUse;

namespace Inkscape {
class Selection;
} // namespace Inkscape

namespace Inkscape::UI::Dialog {

struct SymbolItem;

/**
 * A dialog that displays selectable symbols and allows users to drag or paste
 * those symbols from the dialog into the document.
 *
 * Symbol documents are loaded from the preferences paths and displayed in a
 * drop-down list to the user. The user then selects which of the symbols
 * documents they want to get symbols from. The first document in the list is
 * always the current document.
 *
 * This then updates an icon-view with all the symbols available. Selecting one
 * puts it onto the clipboard. Dragging it or pasting it onto the canvas copies
 * the symbol from the symbol document, into the current document and places a
 * new <use> element at the correct location on the canvas.
 *
 * Selected groups on the canvas can be added to the current document's symbols
 * table, and symbols can be removed from the current document. This allows
 * new symbols documents to be constructed and if saved in the prefs folder will
 * make those symbols available for all future documents.
 */
class SymbolsDialog final : public DialogBase
{
public:
    SymbolsDialog(char const *prefsPath = "/dialogs/symbols");
    ~SymbolsDialog() final;

private:
    void documentReplaced() override;
    void selectionChanged(Inkscape::Selection *selection) override;
    void rebuild_set(Gtk::TreeModel::iterator current);
    void convert_object_to_symbol();
    void revertSymbol();
    void copy_symbol();
    void sendToClipboard(const SymbolItem& symbol, Geom::Rect const &bbox, bool set_clipboard);
    Geom::Point getSymbolDimensions(const std::shared_ptr<SymbolItem>& symbol) const;
    SPDocument* get_symbol_document(const std::optional<Gtk::TreeModel::iterator>& it) const;
    void onDragStart();
    void addSymbol(SPSymbol* symbol, Glib::ustring doc_title, SPDocument* document);
    static std::unique_ptr<SPDocument> symbolsPreviewDoc();
    void useInDoc(SPObject *r, std::vector<SPUse*> &l);
    std::vector<SPUse*> useInDoc( SPDocument* document);
    void addSymbols();
    void showOverlay();
    void hideOverlay();
    gchar const* styleFromUse( gchar const* id, SPDocument* document);
    Cairo::RefPtr<Cairo::Surface> drawSymbol(SPSymbol *symbol);
    Cairo::RefPtr<Cairo::Surface> draw_symbol(SPSymbol* symbol);
    Glib::RefPtr<Gdk::Pixbuf> getOverlay(gint width, gint height);
    void set_info();
    void set_info(const Glib::ustring& text);
    std::optional<Gtk::TreeModel::iterator> get_current_set() const;
    Glib::ustring get_current_set_id() const;
    std::shared_ptr<SymbolItem> get_selected_symbol() const;
    void load_all_symbols();
    void update_tool_buttons();
    size_t total_symbols() const;
    size_t visible_symbols() const;
    void get_cell_data_func(Gtk::CellRenderer* cell_renderer, Gtk::TreeModel::Row row, bool visible);
    void refresh_on_idle(int delay = 100);

    sigc::scoped_connection _idle_search;
    Glib::RefPtr<Gtk::Builder> _builder;
    Gtk::Scale& _zoom;
    // Index into sizes which is selected
    int pack_size;
    // Scale factor
    int scale_factor;
    bool sensitive = false;
    OperationBlocker _update;
    double previous_height;
    double previous_width;
    Gtk::MenuButton& _symbols_popup;
    Gtk::SearchEntry2& _set_search;
    Gtk::IconView& _symbol_sets_view;
    Gtk::Label& _cur_set_name;
    Gtk::SearchEntry2& _search;
    Gtk::Button* add_symbol;
    Gtk::Button* remove_symbol;
    Gtk::Button* _copy_symbol;
    Gtk::Box* tools;
    Gtk::Overlay* overlay;
    Gtk::Image* overlay_icon;
    Gtk::Image* overlay_opacity;
    Gtk::Label* overlay_title;
    Gtk::Label* overlay_desc;
    Gtk::ScrolledWindow *scroller;
    Gtk::CheckButton* fit_symbol;
    Gtk::CellRendererPixbuf _renderer;
    Gtk::CellRendererPixbuf _renderer2;
    std::unique_ptr<SPDocument> preview_document; // Document to render single symbol
    Glib::RefPtr<Gtk::ListStore> _symbol_sets;
    Gtk::GridView& _gridview;

    Cairo::RefPtr<Cairo::Surface> render_icon(SPDocument* document, const std::string& symbol_id, Geom::Point icon_size, int device_scale);
    Glib::RefPtr<Gdk::Texture> get_image(const std::string& key, SPDocument* document, const std::string& id);
    bool is_item_visible(const Glib::RefPtr<Glib::ObjectBase>& item) const;
    void refilter();
    void rebuild(bool clear_image_cache);

    struct Store {
        Glib::RefPtr<Gtk::ListStore> _store;
        Glib::RefPtr<Gtk::TreeModelFilter> _filtered;
        Glib::RefPtr<Gtk::TreeModelSort> _sorted;

        Gtk::TreeModel::iterator path_to_child_iter(Gtk::TreeModel::Path path) const {
            if (_sorted) path = _sorted->convert_path_to_child_path(path);
            if (_filtered) path = _filtered->convert_path_to_child_path(path);
            return _store->get_iter(path);
        }
        void refilter() {
            if (_filtered) _filtered->refilter();
        }
    };
    Store _sets;

    /* For rendering the template drawing */
    unsigned key;
    Inkscape::Drawing renderDrawing;
    sigc::scoped_connection _defs_modified;
    sigc::scoped_connection _doc_resource_changed;
    sigc::scoped_connection _idle_refresh;
    sigc::scoped_connection _selection_changed;
    boost::compute::detail::lru_cache<std::string, Glib::RefPtr<Gdk::Texture>> _image_cache;
    Glib::RefPtr<Gtk::BoolFilter> _filter;
    Glib::RefPtr<Gtk::FilterListModel> _filtered_model;
    Glib::RefPtr<Gtk::SingleSelection> _selection_model;
    std::unique_ptr<IconViewItemFactory> _factory;
    Glib::RefPtr<Gio::ListStore<SymbolItem>> _symbol_store;
};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_UI_DIALOG_SYMBOLS_H

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
