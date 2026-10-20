// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Rendering with Cairo.
 */
/*
 * Author:
 *   Miklos Erdelyi <erdelyim@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Miklos Erdelyi
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <locale>
#include <sstream>
#ifdef HAVE_CONFIG_H
# include "config.h"  // only include where actually required!
#endif

#ifndef PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_BACKEND
#endif

#ifndef PANGO_ENABLE_ENGINE
#define PANGO_ENABLE_ENGINE
#endif


#include <csignal>
#include <cerrno>

#include <2geom/transforms.h>
#include <2geom/pathvector.h>
#include <2geom/point.h>
#include <2geom/rect.h>
#include <cairo.h>
#include <glib.h>
#include <glibmm/i18n.h>

// include support for only the compiled-in surface types
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif

#include "cairo-render-context.h"
#include "cairo-renderer.h"
#include "document.h"
#include "style-internal.h"
#include "display/cairo-utils.h"
#include "filter-chemistry.h"
#include "helper/pixbuf-ops.h"
#include "helper/png-write.h"
#include "libnrtype/Layout-TNG.h"

#include "object/sp-anchor.h"
#include "object/sp-clippath.h"
#include "object/sp-flowtext.h"
#include "object/sp-hatch-path.h"
#include "object/sp-image.h"
#include "object/sp-item-group.h"
#include "object/sp-item.h"
#include "object/sp-marker.h"
#include "object/sp-marker-loc.h"
#include "object/sp-mask.h"
#include "object/sp-page.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "object/sp-symbol.h"
#include "object/sp-text.h"
#include "object/sp-use.h"

#include "util/units.h"

//#define TRACE(_args) g_printf _args
#define TRACE(_args)
//#define TEST(_args) _args
#define TEST(_args)

namespace Inkscape {
namespace Extension {
namespace Internal {

CairoRenderer::CairoRenderer() = default;

CairoRenderer::~CairoRenderer() = default;

CairoRenderContext CairoRenderer::createContext()
{
    return CairoRenderContext{this};
}

/** CairoTagNumpunct and CairoTagStringStream below are used for number to string formatting when
 * setting tag's attribute for Cairo.
 *
 * Cairo 1.16 has a bug and requires the decimal separator to be the one of the current C locale.
 * 
 * For later versions of Cairo, we always use "." as a decimal separator.
 *
 * @see https://gitlab.freedesktop.org/cairo/cairo/-/issues/347
 * @see https://gitlab.com/inkscape/inkscape/-/issues/5354
 */
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 16, 0) && CAIRO_VERSION < CAIRO_VERSION_ENCODE(1, 17, 0)
#define _CAIRO_1_16
/** This facet causes the decimal separator to be the one of the current C locale.
 */
struct CairoTagNumpunct : std::numpunct<char>
{
    protected:
      char do_decimal_point() const override {
        return *std::localeconv()->decimal_point;
      }
};
#endif

/** Used for formatting number to string when sending tag' attributes to Cairo.
 */
class CairoTagStringStream : public std::ostringstream
{
public:
    CairoTagStringStream()
    {
        #if defined _CAIRO_1_16
        imbue(std::locale(std::locale::classic(), new CairoTagNumpunct));
        #else
        imbue(std::locale::classic());
        #endif
    }
};

/* The below functions are copy&pasted plus slightly modified from *_invoke_print functions. */
static void sp_item_invoke_render(SPItem const *item, CairoRenderContext *ctx, SPItem const *origin = nullptr, SPPage const *page = nullptr);
static void sp_group_render(SPGroup const *group, CairoRenderContext *ctx, SPItem const *origin = nullptr, SPPage const *page = nullptr);
static void sp_anchor_render(SPAnchor const *a, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page);
static void sp_use_render(SPUse const *use, CairoRenderContext *ctx, SPPage const *page = nullptr);
static void sp_shape_render(SPShape const *shape, CairoRenderContext *ctx, SPItem const *origin = nullptr);
static void sp_text_render(SPText const *text, CairoRenderContext *ctx);
static void sp_flowtext_render(SPFlowtext const *flowtext, CairoRenderContext *ctx);
static void sp_image_render(SPImage const *image, CairoRenderContext *ctx);
static void sp_symbol_render(SPSymbol const *symbol, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page);
static void sp_asbitmap_render(SPItem const *item, CairoRenderContext *ctx, SPPage const *page = nullptr);

/** Compute the final page dimensions in the resulting PS or PDF.
 *
 * Cairo PS and PDF surfaces only work with integer dimensions, taking ceil() of the doubles
 * passed as arguments. To work around this limitation, we want to "lie" about page dimensions.
 *
 * -> If the page dimension is very very slightly larger than an integer (within an epsilon),
 *    we snap it to that integer. This can happen due to rounding errors in transforms.
 * -> Otherwise, we round the page dimension up to the next integer.
 */
static Geom::Point compute_final_page_dimensions(Geom::Rect const &page_rect) {
    Geom::Point result;
    auto const dims = page_rect.dimensions();

    for (auto const axis : {Geom::X, Geom::Y}) {
        double const floor_size = std::floor(dims[axis]);
        result[axis] = (dims[axis] > floor_size + Geom::EPSILON) ? floor_size + 1.0 : floor_size;
    }
    return result;
}

/** A helper RAII class to manage the temporary rewriting of styles
 * needed to support context-fill and context-stroke values for fill
 * and stroke paints. The destructor restores the old values.
 */
class ContextPaintManager
{
public:
    ContextPaintManager(SPStyle *target_style, SPItem const *style_origin)
        : _managed_style{target_style}
        , _origin{style_origin}
    {
        auto const fill_origin = target_style->fill.paintOrigin;
        if (fill_origin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) {
            _copyPaint(&target_style->fill, *_origin->style->getFillOrStroke(true));
        } else if (fill_origin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE) {
            _copyPaint(&target_style->fill, *_origin->style->getFillOrStroke(false));
        }

        auto const stroke_origin = target_style->stroke.paintOrigin;
        if (stroke_origin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) {
            _copyPaint(&target_style->stroke, *_origin->style->getFillOrStroke(true));
        } else if (stroke_origin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE) {
            _copyPaint(&target_style->stroke, *_origin->style->getFillOrStroke(false));
        }
    }

    ~ContextPaintManager()
    {
        // Restore rewritten paints.
        if (_rewrote_fill) {
            _managed_style->fill = _old_fill;
        }
        if (_rewrote_stroke) {
            _managed_style->stroke = _old_stroke;
        }
    }

private:
    /** Copy paint from origin to destination, saving a copy of the old paint. */
    template<typename PainT>
    void _copyPaint(PainT *destination, SPIPaint paint)
    {
        // Keep a copy of the old paint
        if constexpr (std::is_same<PainT, decltype(_old_fill)>::value) {
            _rewrote_fill = true;
            _old_fill = *destination;
        } else if constexpr (std::is_same<PainT, decltype(_old_stroke)>::value) {
            _rewrote_stroke = true;
            _old_stroke = *destination;
        }
        static_assert(std::is_same_v<PainT, decltype(_old_fill)> || std::is_same_v<PainT, decltype(_old_stroke)>,
                      "ContextPaintManager::_copyPaint() instantiated with neither fill nor stroke type.");
        PainT new_value;
        new_value.upcast()->operator=(paint);
        *destination = new_value;
    }

    SPStyle *_managed_style;
    SPItem const *_origin;
    decltype(_managed_style->fill) _old_fill;
    decltype(_managed_style->stroke) _old_stroke;
    bool _rewrote_fill = false;
    bool _rewrote_stroke = false;
};

static void sp_shape_render(SPShape const *shape, CairoRenderContext *ctx, SPItem const *origin)
{
    if (!shape->curve()) {
        return;
    }

    Geom::PathVector const &pathv = *shape->curve();
    if (pathv.empty()) {
        return;
    }

    Geom::OptRect pbox = shape->geometricBounds();
    SPStyle* style = shape->style;
    std::unique_ptr<ContextPaintManager> context_fs_manager;

    if (origin) {
        context_fs_manager = std::make_unique<ContextPaintManager>(style, origin);
    }

    if (style->paint_order.layer[0] == SP_CSS_PAINT_ORDER_NORMAL ||
        (style->paint_order.layer[0] == SP_CSS_PAINT_ORDER_FILL &&
         style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_STROKE)) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::STROKE_OVER_FILL);
    } else if (style->paint_order.layer[0] == SP_CSS_PAINT_ORDER_STROKE &&
               style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_FILL ) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::FILL_OVER_STROKE);
    } else if (style->paint_order.layer[0] == SP_CSS_PAINT_ORDER_STROKE &&
               style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_MARKER ) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::STROKE_ONLY);
    } else if (style->paint_order.layer[0] == SP_CSS_PAINT_ORDER_FILL &&
               style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_MARKER ) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::FILL_ONLY);
    }

    // Render markers
    bool has_stroke = style->stroke_width.computed > 0.f;
    if (shape->hasMarkers() && has_stroke) {
        for (auto const &[_, marker, tr] : shape->get_markers()) {
            if (auto marker_item = sp_item_first_item_child(marker)) {
                auto const old_tr = marker_item->transform;
                marker_item->transform = old_tr * marker->c2p * tr;
                // Marker's context-fill/context-stroke always refer to shape.
                ctx->getRenderer()->renderItem(ctx, marker_item, shape);
                marker_item->transform = old_tr;
            }
        }
    }

    if (style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_FILL &&
        style->paint_order.layer[2] == SP_CSS_PAINT_ORDER_STROKE) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::STROKE_OVER_FILL);
    } else if (style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_STROKE &&
               style->paint_order.layer[2] == SP_CSS_PAINT_ORDER_FILL ) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::FILL_OVER_STROKE);
    } else if (style->paint_order.layer[2] == SP_CSS_PAINT_ORDER_STROKE &&
               style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_MARKER ) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::STROKE_ONLY);
    } else if (style->paint_order.layer[2] == SP_CSS_PAINT_ORDER_FILL &&
               style->paint_order.layer[1] == SP_CSS_PAINT_ORDER_MARKER ) {
        ctx->renderPathVector(pathv, style, pbox, CairoRenderContext::FILL_ONLY);
    }
}

static void sp_group_render(SPGroup const *group, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page)
{
    CairoRenderer *renderer = ctx->getRenderer();
    for (auto &obj : group->children) {
        if (auto item = cast<SPItem>(&obj)) {
            renderer->renderItem(ctx, item, origin, page);
        }
    }
}

static void sp_use_render(SPUse const *use, CairoRenderContext *ctx, SPPage const *page)
{
    bool translated = false;
    CairoRenderer *renderer = ctx->getRenderer();

    if ((use->x._set && use->x.computed != 0) || (use->y._set && use->y.computed != 0)) {
        // FIXME: This translation sometimes isn't in the correct units; e.g.
        // x="0" y="42" has a different effect than transform="translate(0,42)".
        Geom::Affine tp(Geom::Translate(use->x.computed, use->y.computed));
        ctx->pushState();
        ctx->transform(tp);
        translated = true;
    }

    if (use->child) {
        // Padding in the use object as the origin here ensures markers
        // are rendered with their correct context-fill.
        renderer->renderItem(ctx, use->child, use, page);
    }

    if (translated) {
        ctx->popState();
    }
}

static void sp_text_render(SPText const *text, CairoRenderContext *ctx)
{
    text->layout.showGlyphs(ctx);
}

static void sp_flowtext_render(SPFlowtext const *flowtext, CairoRenderContext *ctx)
{
    flowtext->layout.showGlyphs(ctx);
}

static void sp_image_render(SPImage const *image, CairoRenderContext *ctx)
{
    if (!image->pixbuf) {
        return;
    }

    double width = image->width.computed;
    double height = image->height.computed;
    if (width <= 0.0 || height <= 0.0) {
        return;
    }

    double const w = static_cast<double>(image->pixbuf->width());
    double const h = static_cast<double>(image->pixbuf->height());
    double x = image->x.computed;
    double y = image->y.computed;

    if (image->aspect_align != SP_ASPECT_NONE) {
        calculatePreserveAspectRatio(image->aspect_align, image->aspect_clip, w, h, &x, &y, &width, &height);
    }

    if (image->aspect_clip == SP_ASPECT_SLICE && !ctx->getCurrentState()->has_overflow) {
        ctx->addClippingRect(image->x.computed, image->y.computed, image->width.computed, image->height.computed);
    }

    Geom::Affine const transform = Geom::Scale(width / w, height / h) * Geom::Translate(x, y);
    ctx->renderImage(image->pixbuf.get(), transform, image->style);
}

static void sp_anchor_render(SPAnchor const *a, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page)
{
    if (a->href) {
        // Raw linking, whatever the user said they wanted
        auto link = Glib::ustring::compose("uri='%1'", a->href);
        if (a->local_link) {
            if (auto obj = a->local_link->getObject()) {
                // We wanted to use the syntax page=%d here to link to pages, but
                // cairo has an odd bug that only allows linking to previous pages
                // So we link everything with a dest link instead.
                link = Glib::ustring::compose("dest='%1'", obj->getId());
            }
        }
        // Write a box for this hyperlink so it's contained and positioned correctly.
        if (auto vbox = a->visualBounds()) {
            CairoTagStringStream os;

            // Apply transforms as we are writing out the box directly.
            auto bbox = *vbox * ctx->getTransform();
            os << " rect=[" << bbox.left() << " " << bbox.top() << " " <<  bbox.width() << " " << bbox.height() << "]";
            link += os.str();
        }
        ctx->tagBegin(link.c_str());
    }

    CairoRenderer *renderer = ctx->getRenderer();
    for (auto const &object : a->children) {
        if (auto item = cast<SPItem>(&object)) {
            renderer->renderItem(ctx, item, origin, page);
        }
    }
    if (a->href)
        ctx->tagEnd();
}

static void sp_symbol_render(SPSymbol const *symbol, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page)
{
    if (!symbol->cloned) {
        return;
    }

    /* Cloned <symbol> is actually renderable */
    ctx->pushState();
    ctx->transform(symbol->c2p);

    // apply viewbox if set
    if (false /*symbol->viewBox_set*/) {
        Geom::Affine vb2user;
        double x, y, width, height;
        double view_width, view_height;
        x = 0.0;
        y = 0.0;
        width = 1.0;
        height = 1.0;

        view_width = symbol->viewBox.width();
        view_height = symbol->viewBox.height();

        calculatePreserveAspectRatio(symbol->aspect_align, symbol->aspect_clip, view_width, view_height,
                                     &x, &y,&width, &height);

        // [itemTransform *] translate(x, y) * scale(w/vw, h/vh) * translate(-vx, -vy);
        vb2user = Geom::identity();
        vb2user[0] = width / view_width;
        vb2user[3] = height / view_height;
        vb2user[4] = x - symbol->viewBox.left() * vb2user[0];
        vb2user[5] = y - symbol->viewBox.top() * vb2user[3];

        ctx->transform(vb2user);
    }

    sp_group_render(symbol, ctx, origin, page);
    ctx->popState();
}

static void sp_root_render(SPRoot const *root, CairoRenderContext *ctx)
{
    if (!ctx->getCurrentState()->has_overflow && root->parent) {
        ctx->addClippingRect(root->x.computed, root->y.computed, root->width.computed,
                             root->height.computed);
    }
    ctx->pushState();
    ctx->setStateForItem(root);
    ctx->transform(root->c2p);
    sp_group_render(root, ctx);
    ctx->popState();
}

/**
    This function converts the item to a raster image and includes the image into the cairo renderer.
    It is only used for filters and then only when rendering filters as bitmaps is requested.
*/
static void sp_asbitmap_render(SPItem const *item, CairoRenderContext *ctx, SPPage const *page)
{

    // The code was adapted from sp_selection_create_bitmap_copy in selection-chemistry.cpp

    // Calculate resolution
    /** @TODO reimplement the resolution stuff   (WHY?)
    */
    double res = ctx->getBitmapResolution();
    if (res == 0) {
        res = Inkscape::Util::Quantity::convert(1, "in", "px");
    }
    TRACE(("sp_asbitmap_render: resolution: %f\n", res ));

    // Get the bounding box of the selection in document coordinates.
    Geom::OptRect bbox = item->documentVisualBounds();

    bbox &= (page ? page->getDocumentRect() : item->document->preferredBounds());

    // no bbox, e.g. empty group or item not overlapping its page
    if (!bbox) {
        return;
    }

    // The width and height of the bitmap in pixels
    unsigned width =  ceil(bbox->width() * Inkscape::Util::Quantity::convert(res, "px", "in"));
    unsigned height = ceil(bbox->height() * Inkscape::Util::Quantity::convert(res, "px", "in"));

    if (width == 0 || height == 0) return;

    // Scale to exactly fit integer bitmap inside bounding box
    double scale_x = bbox->width() / width;
    double scale_y = bbox->height() / height;

    // Location of bounding box in document coordinates.
    double shift_x = bbox->min()[Geom::X];
    double shift_y = bbox->top();

    // For default 96 dpi, snap bitmap to pixel grid
    if (res == Inkscape::Util::Quantity::convert(1, "in", "px")) {
        shift_x = round (shift_x);
        shift_y = round (shift_y);
    }

    // Calculate the matrix that will be applied to the image so that it exactly overlaps the source objects

    // Matrix to put bitmap in correct place on document
    Geom::Affine t_on_document = Geom::Scale(scale_x, scale_y) * Geom::Translate(shift_x, shift_y);

    // ctx matrix already includes item transformation. We must substract.
    Geom::Affine t_item =  item->i2doc_affine();
    Geom::Affine t = t_on_document * t_item.inverse();

    // Do the export
    std::unique_ptr<Inkscape::Pixbuf> pb(sp_generate_internal_bitmap(item->document, *bbox, res, {item}, true));

    if (pb) {
        //TEST(gdk_pixbuf_save( pb, "bitmap.png", "png", NULL, NULL ));
        ctx->renderImage(pb.get(), t, item->style);
    }
}

static void sp_item_invoke_render(SPItem const *item, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page)
{
    bool is_linked = false;
    for (auto link : item->getLinked(SPObject::LinkedObjectNature::DEPENDENT)) {
        is_linked |= is<SPAnchor>(link);
    }

    // Test to see if the objects would be invisible on this page and hide them if so.
    if (page && !origin && !page->itemOnPage(item, false, false))
        return;

    if (is_linked)
        ctx->destBegin(item->getId());

    if (auto root = cast<SPRoot>(item)) {
        TRACE(("root\n"));
        sp_root_render(root, ctx);
    } else if (auto symbol = cast<SPSymbol>(item)) {
        TRACE(("symbol\n"));
        sp_symbol_render(symbol, ctx, origin, page);
    } else if (auto anchor = cast<SPAnchor>(item)) {
        TRACE(("<a>\n"));
        sp_anchor_render(anchor, ctx, origin, page);
    } else if (auto shape = cast<SPShape>(item)) {
        TRACE(("shape\n"));
        sp_shape_render(shape, ctx, origin);
    } else if (auto use = cast<SPUse>(item)) {
        TRACE(("use begin---\n"));
        sp_use_render(use, ctx, page);
        TRACE(("---use end\n"));
    } else if (auto text = cast<SPText>(item)) {
        TRACE(("text\n"));
        sp_text_render(text, ctx);
    } else if (auto flowtext = cast<SPFlowtext>(item)) {
        TRACE(("flowtext\n"));
        sp_flowtext_render(flowtext, ctx);
    } else if (auto image = cast<SPImage>(item)) {
        TRACE(("image\n"));
        sp_image_render(image, ctx);
    } else if (is<SPMarker>(item)) {
        // Marker contents shouldn't be rendered, even outside of <defs>.
    } else if (auto group = cast<SPGroup>(item)) {
        TRACE(("<g>\n"));
        sp_group_render(group, ctx, origin, page);
    }

    if (is_linked)
        ctx->destEnd();
}

bool CairoRenderer::_shouldRasterize(CairoRenderContext *ctx, SPItem const *item)
{
    // rasterize filtered items as per user setting
    // however, clipPaths ignore any filters, so do *not* rasterize
    // TODO: might apply to some degree to masks with filtered elements as well;
    //       we need to figure out where in the stack it would be safe to rasterize
    if (ctx->getFilterToBitmap() && !item->isInClipPath()) {
        if (auto const *clone = cast<SPUse>(item)) {
            return clone->anyInChain([](SPItem const *i) { return i && i->isFiltered(); });
        } else {
            return item->isFiltered();
        }
    }
    return false;
}

void CairoRenderer::_doRender(SPItem const *item, CairoRenderContext *ctx, SPItem const *origin, SPPage const *page)
{
    // Check item's visibility
    if (item->isHidden() || has_hidder_filter(item)) {
        return;
    }

    if (_shouldRasterize(ctx, item)) {
        sp_asbitmap_render(item, ctx, page);
    } else {
        sp_item_invoke_render(item, ctx, origin, page);
    }
}

void CairoRenderer::renderItem(CairoRenderContext *ctx, SPItem const *item, SPItem const *origin, SPPage const *page)
{
    ctx->pushState();
    ctx->setStateForItem(item);

    auto *state = ctx->getCurrentState();
    ctx->setStateNeedsLayer(state->mask || state->clip_path || state->opacity != 1.0);
    SPStyle* style = item->style;
    auto group = cast<SPGroup>(item);
    bool blend = false;
    if (group && style->mix_blend_mode.set && style->mix_blend_mode.value != SP_CSS_BLEND_NORMAL) {
        // Force the creation of a new layer
        ctx->setStateNeedsLayer(true);
        blend = true;
    }
    // Draw item on a temporary surface so a mask, clip-path, or opacity can be applied to it.
    if (state->need_layer) {
        ctx->setStateMergeOpacity(false);
        ctx->pushLayer();
    }

    ctx->transform(item->transform);

    _doRender(item, ctx, origin, page);

    if (ctx->getCurrentState()->need_layer) {
        if (blend) {
            ctx->popLayer(ink_css_blend_to_cairo_operator(style->mix_blend_mode.value)); // This applies clipping/masking
        } else {
            ctx->popLayer(); // This applies clipping/masking
        }
    }
    ctx->popState();
}

void CairoRenderer::renderHatchPath(CairoRenderContext *ctx, SPHatchPath const &hatchPath, unsigned key) {
    ctx->pushState();
    ctx->setStateForStyle(hatchPath.style);
    ctx->transform(Geom::Translate(hatchPath.offset.computed, 0));

    auto const pathv = hatchPath.calculateRenderCurve(key);
    if (!pathv.empty()) {
        ctx->renderPathVector(pathv, hatchPath.style, Geom::OptRect());
    }

    ctx->popState();
}

bool CairoRenderer::setupDocument(CairoRenderContext *ctx, SPDocument *doc, SPItem const *base)
{
// PLEASE note when making changes to the boundingbox and transform calculation, corresponding changes should be made to LaTeXTextRenderer::setupDocument !!!
    g_assert(ctx);

    if (!base) {
        base = doc->getRoot();
    }

    // Most pages will ignore this setup, but we still want to initialise something useful.
    Geom::Rect d = Geom::Rect::from_xywh(Geom::Point(0,0), doc->getDimensions());
    double px_to_ctx_units = 1.0;
    if (ctx->_vector_based_target) {
        // convert from px to pt
        px_to_ctx_units = Inkscape::Util::Quantity::convert(1, "px", "pt");
    }

    auto width = d.width() * px_to_ctx_units;
    auto height = d.height() * px_to_ctx_units;

    ctx->setMetadata(*doc);

    TRACE(("setupDocument: %f x %f\n", width, height));
    return ctx->setupSurface(width, height);
}

/**
 * Handle multiple pages, pushing each out to cairo as needed using renderItem()
 */
bool
CairoRenderer::renderPages(CairoRenderContext *ctx, SPDocument *doc, bool stretch_to_fit)
{
    auto pages = doc->getPageManager().getPages();
    if (pages.size() == 0) {
        // Output the page bounding box as already set up in the initial setupDocument.
        renderItem(ctx, doc->getRoot());
        return true;
    }

    for (auto &page : pages) {
        ctx->pushState();
        if (!renderPage(ctx, doc, page, stretch_to_fit)) {
            return false;
        }
        // Create a page dest for any anchor tags that link to this page.
        ctx->destBegin(page->getId());
        ctx->destEnd();

        if (!ctx->finishPage()) {
            g_warning("Couldn't render page in output!");
            return false;
        }
        ctx->popState();
    }
    return true;
}

bool
CairoRenderer::renderPage(CairoRenderContext *ctx, SPDocument *doc, SPPage const *page, bool stretch_to_fit)
{
    // Calculate exact page rectangle in PostScript points:
    auto const scale = doc->getDocumentScale();
    auto const unit_conversion = Geom::Scale(Inkscape::Util::Quantity::convert(1, "px", "pt"));

    auto const rect = page->getBleed();
    auto const exact_rect = rect * scale * unit_conversion;
    auto const [final_width, final_height] = compute_final_page_dimensions(exact_rect);

    if (stretch_to_fit) {
        // Calculate distortion from rounding (only really matters for small paper sizes):
        auto distortion = Geom::Scale(final_width / exact_rect.width(),
                                      final_height / exact_rect.height());

        // Scale the drawing a tiny bit so that it still fills the rounded page:
        ctx->transform(scale * distortion);
    } else {
        ctx->transform(scale);
    }

    SPRoot *root = doc->getRoot();
    ctx->transform(root->transform);
    ctx->nextPage(final_width, final_height, page->label());

    // Set up page transformation which pushes objects back into the 0,0 location
    ctx->transform(Geom::Translate(rect.corner(0)).inverse());

    for (auto &child : page->getOverlappingItems(false, true, false)) {
        ctx->pushState();

        // This process does not return layers, so those affines are added manually.
        for (auto anc : child->ancestorList(true)) {
            if (auto layer = cast<SPItem>(anc)) {
                if (layer != child && layer != root) {
                    ctx->transform(layer->transform);
                }
            }
        }

        // Render the page into the context in the new location.
        renderItem(ctx, child, nullptr, page);
        ctx->popState();
    }
    return true;
}

// Apply an SVG clip path
void
CairoRenderer::applyClipPath(CairoRenderContext *ctx, SPClipPath const *cp)
{
    g_assert( ctx != nullptr && ctx->_is_valid );

    if (cp == nullptr)
        return;

    CairoRenderContext::CairoRenderMode saved_mode = ctx->getRenderMode();
    ctx->setRenderMode(CairoRenderContext::RENDER_MODE_CLIP);

    // FIXME: the access to the first clippath view to obtain the bbox is completely bogus
    Geom::Affine saved_ctm;
    if (cp->clippath_units() == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && cp->get_last_bbox()) {
        Geom::Rect clip_bbox = *cp->get_last_bbox();
        Geom::Affine t(Geom::Scale(clip_bbox.dimensions()));
        t[4] = clip_bbox.left();
        t[5] = clip_bbox.top();
        t *= ctx->getCurrentState()->transform;
        saved_ctm = ctx->getTransform();
        ctx->setTransform(t);
    }

    TRACE(("BEGIN clip\n"));
    SPObject const *co = cp;
    for (auto& child: co->children) {
        SPItem const *item = cast<SPItem>(&child);
        if (item) {
            // combine transform of the item in clippath and the item using clippath:
            Geom::Affine tempmat = item->transform * ctx->getCurrentState()->item_transform;

            // render this item in clippath
            ctx->pushState();
            ctx->transform(tempmat);
            ctx->setStateForItem(item);
            _doRender(item, ctx);
            ctx->popState();
        }
    }
    TRACE(("END clip\n"));

    // do clipping only if this was the first call to applyClipPath
    if (ctx->getClipMode() == CairoRenderContext::CLIP_MODE_PATH
        && saved_mode == CairoRenderContext::RENDER_MODE_NORMAL)
        cairo_clip(ctx->_cr);

    if (cp->clippath_units() == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX)
        ctx->setTransform(saved_ctm);

    ctx->setRenderMode(saved_mode);
}

// Apply an SVG mask
void
CairoRenderer::applyMask(CairoRenderContext *ctx, SPMask const *mask)
{
    g_assert( ctx != nullptr && ctx->_is_valid );

    if (mask == nullptr)
        return;

    // FIXME: the access to the first mask view to obtain the bbox is completely bogus
    // TODO: should the bbox be transformed if maskUnits != userSpaceOnUse ?
    if (mask->mask_content_units() == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && mask->get_last_bbox()) {
        Geom::Rect mask_bbox = *mask->get_last_bbox();
        Geom::Affine t(Geom::Scale(mask_bbox.dimensions()));
        t[4] = mask_bbox.left();
        t[5] = mask_bbox.top();
        t *= ctx->getCurrentState()->transform;
        ctx->setTransform(t);
    }

    // Clip mask contents... but...
    // The mask's bounding box is the "geometric bounding box" which doesn't allow for
    // filters which extend outside the bounding box. So don't clip.
    // ctx->addClippingRect(mask_bbox.x0, mask_bbox.y0, mask_bbox.x1 - mask_bbox.x0, mask_bbox.y1 - mask_bbox.y0);

    ctx->pushState();

    TRACE(("BEGIN mask\n"));
    for (auto const &child : mask->children) {
        if (auto item = cast<SPItem>(&child)) {
            renderItem(ctx, item);
        }
    }
    TRACE(("END mask\n"));

    ctx->popState();
}

void
calculatePreserveAspectRatio(unsigned int aspect_align, unsigned int aspect_clip, double vp_width, double vp_height,
                             double *x, double *y, double *width, double *height)
{
    if (aspect_align == SP_ASPECT_NONE)
        return;

    double scalex, scaley, scale;
    double new_width, new_height;
    scalex = *width / vp_width;
    scaley = *height / vp_height;
    scale = (aspect_clip == SP_ASPECT_MEET) ? MIN(scalex, scaley) : MAX(scalex, scaley);
    new_width = vp_width * scale;
    new_height = vp_height * scale;
    /* Now place viewbox to requested position */
    switch (aspect_align) {
        case SP_ASPECT_XMIN_YMIN:
            break;
        case SP_ASPECT_XMID_YMIN:
            *x -= 0.5 * (new_width - *width);
            break;
        case SP_ASPECT_XMAX_YMIN:
            *x -= 1.0 * (new_width - *width);
            break;
        case SP_ASPECT_XMIN_YMID:
            *y -= 0.5 * (new_height - *height);
            break;
        case SP_ASPECT_XMID_YMID:
            *x -= 0.5 * (new_width - *width);
            *y -= 0.5 * (new_height - *height);
            break;
        case SP_ASPECT_XMAX_YMID:
            *x -= 1.0 * (new_width - *width);
            *y -= 0.5 * (new_height - *height);
            break;
        case SP_ASPECT_XMIN_YMAX:
            *y -= 1.0 * (new_height - *height);
            break;
        case SP_ASPECT_XMID_YMAX:
            *x -= 0.5 * (new_width - *width);
            *y -= 1.0 * (new_height - *height);
            break;
        case SP_ASPECT_XMAX_YMAX:
            *x -= 1.0 * (new_width - *width);
            *y -= 1.0 * (new_height - *height);
            break;
        default:
            break;
    }
    *width = new_width;
    *height = new_height;
}

#include "clear-n_.h"

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */

#undef TRACE


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
