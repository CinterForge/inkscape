// SPDX-License-Identifier: GPL-2.0-or-later

#include <2geom/symbolic/matrix.h>
#include <2geom/affine.h>
#include <cairomm/surface.h>
#include <gtkmm/liststore.h>
#include <glibmm/i18n.h>

#include "colors/color.h"
#include "pattern-manager.h"
#include "pattern-manipulation.h"
#include "document.h"
#include "manipulation/copy-resource.h"
#include "style.h"
#include "object/sp-pattern.h"
#include "object/sp-defs.h"
#include "object/sp-hatch.h"
#include "object/sp-root.h"
#include "util/units.h"
#include "ui/svg-renderer.h"

using Inkscape::UI::Widget::PatternItem;
using namespace std::literals;

namespace Inkscape {

// pattern preview for the UI list, with a light-gray background and border
std::unique_ptr<SPDocument> get_preview_document() {
    // language=XML
    constexpr auto buffer = R"A(
<svg width="40" height="40" viewBox="0 0 40 40"
   xmlns:xlink="http://www.w3.org/1999/xlink"
   xmlns="http://www.w3.org/2000/svg">
  <defs id="defs">
  </defs>
  <g id="layer1">
    <rect
       style="fill:#f6f6f6;fill-opacity:1;stroke:none"
       id="rect2620"
       width="100%" height="100%" x="0" y="0" />
    <rect
       style="fill:url(#sample);fill-opacity:1;stroke:black;stroke-opacity:0.3;stroke-width:1px"
       id="rect236"
       width="100%" height="100%" x="0" y="0" />
  </g>
</svg>
)A"sv;
    return SPDocument::createNewDocFromMem(buffer);
}

// pattern preview document without a background
std::unique_ptr<SPDocument> get_big_preview_document() {
    // language=XML
    constexpr auto buffer = R"A(
<svg width="100" height="100"
   xmlns:xlink="http://www.w3.org/1999/xlink"
   xmlns="http://www.w3.org/2000/svg">
  <defs id="defs">
  </defs>
  <g id="layer1">
    <rect
       style="fill:url(#sample);fill-opacity:1;stroke:none"
       width="100%" height="100%" x="0" y="0" />
  </g>
</svg>
)A"sv;
    return SPDocument::createNewDocFromMem(buffer);
}

PatternManager::PatternManager() {
    _preview_doc = get_preview_document();
    if (!_preview_doc.get() || !_preview_doc->getReprDoc()) {
        throw std::runtime_error("Pattern embedded preview document cannot be loaded");
    }

    _big_preview_doc = get_big_preview_document();
    if (!_big_preview_doc.get() || !_big_preview_doc->getReprDoc()) {
        throw std::runtime_error("Pattern embedded big preview document cannot be loaded");
    }
}

// delayed initialization, until stock patterns are needed
void PatternManager::init() {
    if (_initialized) return;

    auto model = Gtk::ListStore::create(columns);

    std::vector<SPPaintServer*> all;

    auto stock = sp_get_stock_patterns();
    auto hatch = sp_get_stock_hatches();
    stock.insert(stock.end(), hatch.begin(), hatch.end());

    for (auto doc : stock) {
        auto patterns = sp_get_pattern_list(doc);
        auto hatches = sp_get_hatch_list(doc);
        all.insert(all.end(), patterns.begin(), patterns.end());
        all.insert(all.end(), hatches.begin(), hatches.end());

        Glib::ustring name = doc->getDocumentName();
        name = name.substr(0, name.rfind(".svg"));
        _categories.push_back(std::make_shared<Category>(Category{name, std::move( patterns.empty() ? hatches : patterns), false}));
    }

    for (auto pat : all) {
        // create empty items for stock patterns
        _cache[pat] = Glib::RefPtr<PatternItem>();
    }

    // special "all patterns" category
    _categories.push_back(std::make_shared<Category>(Category{_("All patterns"), std::move(all), true}));

    // sort by name, keep "all patterns" category first
    std::sort(_categories.begin(), _categories.end(), [] (auto const &a, auto const &b) {
        if (a->all != b->all) {
            return a->all;
        }
        return a->name < b->name;
    });

    for (auto& category : _categories) {
        auto row = *model->append();
        row[columns.name] = category->name;
        row[columns.category] = category;
        row[columns.all_patterns] = category->all;
    }

    _model = model;
    _initialized = true;
}

static Cairo::RefPtr<Cairo::ImageSurface> create_pattern_image(SPDocument &sandbox,
    char const* name, SPDocument &source, double scale,
    std::optional<uint32_t> checkerboard = {})
{
    // Retrieve the pattern named 'name' from the source SVG document
    SPObject const* pattern = source.getObjectById(name);
    if (pattern == nullptr) {
        g_warning("bad name: %s", name);
        return Cairo::RefPtr<Cairo::ImageSurface>();
    }

    auto list = sandbox.getDefs()->childList(true);
    for (auto obj : list) {
        obj->deleteObject();
        sp_object_unref(obj);
    }

    SPDocument::install_reference_document scoped(&sandbox, &source);

    // Create a copy of the pattern, name it "sample"
    auto copy = sp_copy_resource(pattern, &sandbox);
    copy->getRepr()->setAttribute("id", "sample");

    sandbox.getRoot()->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    sandbox.ensureUpToDate();

    svg_renderer renderer(sandbox);
    if (checkerboard.has_value()) {
        renderer.set_checkerboard_color(*checkerboard);
    }
    auto surface = renderer.render_surface(scale);
    if (surface) {
        cairo_surface_set_device_scale(surface->cobj(), scale, scale);
    }

    // delete sample to release href to the original pattern, if any has been referenced by 'copy'
    SPObject* oldpattern = sandbox.getObjectById("sample");
    if (oldpattern) {
        oldpattern->deleteObject(false);
    }
    return surface;
}

// given a hatch, create a PatternItem instance that describes it;
// input pattern can be a link or a root pattern
static Glib::RefPtr<PatternItem> create_hatch_item(SPHatch* hatch, bool stock_hatch, double scale) {
    if (!hatch) return {};

    auto item = PatternItem::create();

    //  this is a link:     this is a root:
    // <hatch href="abc"/> <hatch id="abc"/>
    auto link_hatch = hatch;
    auto root_hatch = hatch->rootHatch();

    // get label and ID from root pattern
    XML::Node* repr = root_hatch->getRepr();
    if (auto id = repr->attribute("id")) {
        item->id = id;
    }
    item->label = sp_get_pattern_label(root_hatch);
    item->stock = stock_hatch;
    item->transform = link_hatch->get_this_transform();
    item->rotation = link_hatch->rotate();
    item->pitch = link_hatch->pitch();
    item->offset = Geom::Point(link_hatch->x(), link_hatch->y());

    // reading color style form "root" hatch; linked one won't have any effect, as it's not a parent;
    // hatches (through <hatchpath>) support stroke colors, and there could be multiple potentially different ones,
    // but if stroke color is defined on a <hatch> element, we use it:
    if (root_hatch->style && root_hatch->style->isSet(SPAttr::STROKE) && root_hatch->style->stroke.isColor()) {
        item->color.emplace(root_hatch->style->stroke.getColor());
    }
    // likewise, use stroke width if it is set:
    if (root_hatch->style) {
        auto& stroke = root_hatch->style->stroke_width;
        if (stroke.set) {
            item->stroke = stroke.computed ? stroke.computed : 1.0;
        }
    }

    //TODO uniform scaling? no such attribute

    // item->uniform_scale = true; // there's no viewbox

    // hatch tile pitch; on a root hatch
    item->gap = Geom::Scale(root_hatch->pitch(), 0);

    // which collection stock pattern comes from
    item->collection = stock_hatch ? hatch->document : nullptr;

    return item;
}

// given a pattern, create a PatternItem instance that describes it;
// input pattern can be a link or a root pattern
static Glib::RefPtr<PatternItem> create_pattern_item(SPPattern* pattern, bool stock_pattern, double scale) {
    if (!pattern) return {};

    auto item = PatternItem::create();

    //  this is a link:       this is a root:
    // <pattern href="abc"/> <pattern id="abc"/>
    // if pattern is a root one to begin with, then both pointers will be the same:
    auto link_pattern = pattern;
    auto root_pattern = pattern->rootPattern();

    // get label and ID from root pattern
    Inkscape::XML::Node* repr = root_pattern->getRepr();
    if (auto id = repr->attribute("id")) {
        item->id = id;
    }
    item->label = sp_get_pattern_label(root_pattern);
    item->stock = stock_pattern;
    // read transformation from a link pattern
    item->transform = link_pattern->get_this_transform();
    item->offset = Geom::Point(link_pattern->x(), link_pattern->y());

    // reading color style form "root" pattern; linked one won't have any effect, as it's not a parent
    if (root_pattern->style && root_pattern->style->isSet(SPAttr::FILL) && root_pattern->style->fill.isColor()) {
        item->color.emplace(root_pattern->style->fill.getColor());
    }
    // uniform scaling?
    if (link_pattern->aspect_set) {
        auto preserve = link_pattern->getAttribute("preserveAspectRatio");
        item->uniform_scale = preserve && strcmp(preserve, "none") != 0;
    }
    else {
        item->uniform_scale = false;
    }
    // pattern tile gap (only valid for link patterns)
    item->gap = link_pattern != root_pattern ? sp_pattern_get_gap(link_pattern) : Geom::Scale(0, 0);

    // which collection stock pattern comes from
    item->collection = stock_pattern ? pattern->document : nullptr;

    return item;
}

Cairo::RefPtr<Cairo::ImageSurface> PatternManager::get_image(SPPaintServer* pattern, int width, int height, double device_scale) {
    if (!pattern) return {};

    _preview_doc->setWidth(Inkscape::Util::Quantity(width, "px"));
    _preview_doc->setHeight(Inkscape::Util::Quantity(height, "px"));
    return create_pattern_image(*_preview_doc, pattern->getId(), *pattern->document, device_scale);
}

Cairo::RefPtr<Cairo::ImageSurface> PatternManager::get_preview(SPPaintServer* pattern, int width, int height, unsigned int rgba_background, double device_scale) {
    if (!pattern) return {};

    _big_preview_doc->setWidth(Inkscape::Util::Quantity(width, "px"));
    _big_preview_doc->setHeight(Inkscape::Util::Quantity(height, "px"));

    return create_pattern_image(*_big_preview_doc, pattern->getId(), *pattern->document, device_scale, rgba_background);
}

Glib::RefPtr<PatternItem> PatternManager::get_item(SPPaintServer* paint) {
    Glib::RefPtr<PatternItem> item;
    if (!paint) return item;

    init();
    auto it = _cache.find(paint);
    // if pattern entry was present in the cache, then it is a stock pattern
    bool stock = it != _cache.end();
    if (!stock || !it->second) {
        // generate item
        auto pattern = cast<SPPattern>(paint);
        item = pattern ? create_pattern_item(pattern, stock, 0) : create_hatch_item(cast<SPHatch>(paint), stock, 0);

        if (stock) {
            _cache[paint] = item;
        }
    }
    else {
        item = it->second;
    }

    return item;
}

Glib::RefPtr<Gtk::TreeModel> PatternManager::get_categories() {
    init();
    return _model;
}

} // namespace
