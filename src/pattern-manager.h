// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_PATTERN_MANAGER_H
#define INKSCAPE_PATTERN_MANAGER_H

#include <gtkmm/treemodel.h>

#include "helper/stock-items.h"
#include "ui/widget/pattern-store.h"
#include "util/statics.h"
#include "style.h"

namespace Cairo {
class Surface;
}

class SPPattern;
class SPDocument;

namespace Inkscape {

class PatternManager
    : public Util::EnableSingleton<PatternManager, Util::Depends<StockPaintDocuments>>
{
public:
    struct Category {
        const Glib::ustring name;
        const std::vector<SPPaintServer*> patterns;
        const bool all;
    };

    class PatternCategoryColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        PatternCategoryColumns() {
            add(name);
            add(category);
            add(all_patterns);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<std::shared_ptr<Category>> category;
        Gtk::TreeModelColumn<bool> all_patterns;
    };
    PatternCategoryColumns columns;

    // get all stock pattern categories
    Glib::RefPtr<Gtk::TreeModel> get_categories();

    // get pattern description item
    Glib::RefPtr<Inkscape::UI::Widget::PatternItem> get_item(SPPaintServer* pattern);

    // get pattern image on a solid background for use in UI lists
    Cairo::RefPtr<Cairo::ImageSurface> get_image(SPPaintServer* pattern, int width, int height, double device_scale);

    // get pattern image on a checkerboard background for use as a larger preview
    Cairo::RefPtr<Cairo::ImageSurface> get_preview(SPPaintServer* pattern, int width, int height, unsigned int rgba_background, double device_scale);

private:
    PatternManager();
    friend class EnableSingleton;

    void init();
    Glib::RefPtr<Gtk::TreeModel> _model;
    std::vector<std::shared_ptr<Category>> _categories;
    std::unordered_map<SPPaintServer*, Glib::RefPtr<Inkscape::UI::Widget::PatternItem>> _cache;
    std::unique_ptr<SPDocument> _preview_doc;
    std::unique_ptr<SPDocument> _big_preview_doc;
    bool _initialized = false;
};

} // namespace

#endif
