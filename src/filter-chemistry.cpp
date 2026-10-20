// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Various utility methods for filters
 *
 * Authors:
 *   Hugo Rodrigues
 *   bulia byak
 *   Niko Kiirala
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006-2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "filter-chemistry.h"

#include <cstring>
#include <glibmm.h>

#include "desktop-style.h"
#include "document.h"
#include "filter-enums.h"
#include "style.h"

#include "object/sp-defs.h"
#include "object/sp-item.h"

#include "object/filters/blend.h"
#include "object/filters/gaussian-blur.h"

/**
 * Count how many times the filter is used by the styles of o and its
 * descendants
 */
static guint count_filter_hrefs(SPObject *o, SPFilter *filter)
{
    if (!o)
        return 1;

    guint i = 0;

    SPStyle *style = o->style;
    if (style
        && style->filter.set
        && style->getFilter() == filter)
    {
        i ++;
    }

    for (auto& child: o->children) {
        i += count_filter_hrefs(&child, filter);
    }

    return i;
}

SPFilter *new_filter(SPDocument *document)
{
    g_return_val_if_fail(document != nullptr, NULL);

    SPDefs *defs = document->getDefs();

    Inkscape::XML::Document *xml_doc = document->getReprDoc();

    // create a new filter
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:filter");

    // Inkscape now supports both sRGB and linear color-interpolation-filters.
    // But, for the moment, keep sRGB as default value for new filters
    // (historically set to sRGB and doesn't require conversion between
    // filter cairo surfaces and other types of cairo surfaces).
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property(css, "color-interpolation-filters", "sRGB");
    sp_repr_css_change(repr, css, "style");
    sp_repr_css_attr_unref(css);

    // Append the new filter node to defs
    defs->appendChild(repr);
    Inkscape::GC::release(repr);

    // get corresponding object
    auto f = cast<SPFilter>( document->getObjectByRepr(repr) );
    
    
    g_assert(f != nullptr);

    return f;
}

SPFilterPrimitive *
filter_add_primitive(SPFilter *filter, const Inkscape::Filters::FilterPrimitiveType type)
{
    Inkscape::XML::Document *xml_doc = filter->document->getReprDoc();

    //create filter primitive node
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement(FPConverter.get_key(type).c_str());

    // set default values
    switch(type) {
        case Inkscape::Filters::NR_FILTER_BLEND:
            repr->setAttribute("mode", "normal");
            break;
        case Inkscape::Filters::NR_FILTER_COLORMATRIX:
            break;
        case Inkscape::Filters::NR_FILTER_COMPONENTTRANSFER:
            break;
        case Inkscape::Filters::NR_FILTER_COMPOSITE:
            break;
        case Inkscape::Filters::NR_FILTER_CONVOLVEMATRIX:
            repr->setAttribute("order", "3 3");
            repr->setAttribute("kernelMatrix", "0 0 0 0 0 0 0 0 0");
            break;
        case Inkscape::Filters::NR_FILTER_DIFFUSELIGHTING:
            break;
        case Inkscape::Filters::NR_FILTER_DISPLACEMENTMAP:
            break;
        case Inkscape::Filters::NR_FILTER_FLOOD:
            break;
        case Inkscape::Filters::NR_FILTER_GAUSSIANBLUR:
            repr->setAttribute("stdDeviation", "1");
            break;
        case Inkscape::Filters::NR_FILTER_IMAGE:
            break;
        case Inkscape::Filters::NR_FILTER_MERGE:
            break;
        case Inkscape::Filters::NR_FILTER_MORPHOLOGY:
            repr->setAttribute("radius", "1");
            break;
        case Inkscape::Filters::NR_FILTER_OFFSET:
            repr->setAttribute("dx", "0");
            repr->setAttribute("dy", "0");
            break;
        case Inkscape::Filters::NR_FILTER_SPECULARLIGHTING:
            break;
        case Inkscape::Filters::NR_FILTER_TILE:
            break;
        case Inkscape::Filters::NR_FILTER_TURBULENCE:
            break;
        default:
            break;
    }

    //set primitive as child of filter node
    // XML tree being used directly while/where it shouldn't be...
    filter->appendChild(repr);
    Inkscape::GC::release(repr);
    
    // get corresponding object
    auto prim = cast<SPFilterPrimitive>( filter->document->getObjectByRepr(repr) );
 
    g_assert(prim != nullptr);

    return prim;
}

/**
 * Creates a filter with blur primitive of specified radius for an item with the given matrix expansion, width and height
 */
SPFilter *
new_filter_gaussian_blur (SPDocument *document, gdouble radius, double expansion)
{
    g_return_val_if_fail(document != nullptr, NULL);

    SPDefs *defs = document->getDefs();

    Inkscape::XML::Document *xml_doc = document->getReprDoc();

    // create a new filter
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:filter");
    //repr->setAttribute("inkscape:collect", "always");


    /* Inkscape now supports both sRGB and linear color-interpolation-filters.  
     * But, for the moment, keep sRGB as default value for new filters.
     * historically set to sRGB and doesn't require conversion between
     * filter cairo surfaces and other types of cairo surfaces. lp:1127103 */
    SPCSSAttr *css = sp_repr_css_attr_new();                                    
    sp_repr_css_set_property(css, "color-interpolation-filters", "sRGB");       
    sp_repr_css_change(repr, css, "style");                                     
    sp_repr_css_attr_unref(css);

    //create feGaussianBlur node
    Inkscape::XML::Node *b_repr;
    b_repr = xml_doc->createElement("svg:feGaussianBlur");
    //b_repr->setAttribute("inkscape:collect", "always");
    
    double stdDeviation = radius;
    if (expansion != 0)
        stdDeviation /= expansion;

    //set stdDeviation attribute
    b_repr->setAttributeSvgDouble("stdDeviation", stdDeviation);
    
    //set feGaussianBlur as child of filter node
    repr->appendChild(b_repr);
    Inkscape::GC::release(b_repr);
    
    // Append the new filter node to defs
    defs->appendChild(repr);
    Inkscape::GC::release(repr);

    // get corresponding object
    auto f = cast<SPFilter>( document->getObjectByRepr(repr) );
    auto b = cast<SPGaussianBlur>( document->getObjectByRepr(b_repr) );
    
    g_assert(f != nullptr);
    g_assert(b != nullptr);

    return f;
}


/**
 * Creates a simple filter with a blend primitive and a blur primitive of specified radius for
 * an item with the given matrix expansion, width and height
 */
static SPFilter *
new_filter_blend_gaussian_blur (SPDocument *document, const char *blendmode, gdouble radius, double expansion)
{
    g_return_val_if_fail(document != nullptr, NULL);

    SPDefs *defs = document->getDefs();

    Inkscape::XML::Document *xml_doc = document->getReprDoc();

    // create a new filter
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:filter");
    repr->setAttribute("inkscape:collect", "always");

    /* Inkscape now supports both sRGB and linear color-interpolation-filters.  
     * But, for the moment, keep sRGB as default value for new filters. 
     * historically set to sRGB and doesn't require conversion between
     * filter cairo surfaces and other types of cairo surfaces. lp:1127103 */
    SPCSSAttr *css = sp_repr_css_attr_new();                                    
    sp_repr_css_set_property(css, "color-interpolation-filters", "sRGB");       
    sp_repr_css_change(repr, css, "style");                                     
    sp_repr_css_attr_unref(css);

    // Append the new filter node to defs
    defs->appendChild(repr);
    Inkscape::GC::release(repr);
 
    // get corresponding object
    auto f = cast<SPFilter>( document->getObjectByRepr(repr) );
    // Gaussian blur primitive
    if(radius != 0) {
        //create feGaussianBlur node
        Inkscape::XML::Node *b_repr;
        b_repr = xml_doc->createElement("svg:feGaussianBlur");
        b_repr->setAttribute("inkscape:collect", "always");
        
        double stdDeviation = radius;
        if (expansion != 0)
            stdDeviation /= expansion;
        
        //set stdDeviation attribute
        b_repr->setAttributeSvgDouble("stdDeviation", stdDeviation);
     
        //set feGaussianBlur as child of filter node
        repr->appendChild(b_repr);
        Inkscape::GC::release(b_repr);

        auto b = cast<SPGaussianBlur>( document->getObjectByRepr(b_repr) );
        g_assert(b != nullptr);
    }
    // Blend primitive
    if(strcmp(blendmode, "normal")) {
        Inkscape::XML::Node *b_repr;
        b_repr = xml_doc->createElement("svg:feBlend");
        b_repr->setAttribute("inkscape:collect", "always");
        b_repr->setAttribute("mode", blendmode);
        b_repr->setAttribute("in2", "BackgroundImage");

        // set feBlend as child of filter node
        repr->appendChild(b_repr);
        Inkscape::GC::release(b_repr);

        // Enable background image buffer for document
        Inkscape::XML::Node *root = b_repr->root();
        if (!root->attribute("enable-background")) {
            root->setAttribute("enable-background", "new");
        }

        auto b = cast<SPFeBlend>(document->getObjectByRepr(b_repr));
        g_assert(b != nullptr);
    }
    
    g_assert(f != nullptr);
 
    return f;
}

/**
 * Creates a simple filter for the given item with blend and blur primitives, using the
 * specified mode and radius, respectively
 */
SPFilter *
new_filter_simple_from_item (SPDocument *document, SPItem *item, const char *mode, gdouble radius)
{
    return new_filter_blend_gaussian_blur(document, mode, radius, item->i2dt_affine().descrim());
}

/**
 * Modifies the gaussian blur applied to the item.
 * If no filters are applied to given item, creates a new blur filter.
 * If a filter is applied and it contains a blur, modify that blur.
 * If the filter doesn't contain blur, a blur is added to the filter.
 * Should there be more references to modified filter, that filter is
 * duplicated, so that other elements referring that filter are not modified.
 */
/* TODO: this should be made more generic, not just for blurs */
SPFilter *modify_filter_gaussian_blur_from_item(SPDocument *document, SPItem *item,
                                                gdouble radius)
{
    if (!item->style || !item->style->filter.set) {
        return new_filter_simple_from_item(document, item, "normal", radius);
    }

    SPFilter *filter = item->style->getFilter();
    if (!filter) {
        // We reach here when filter.set is true, but the href is not found in the document
        return new_filter_simple_from_item(document, item, "normal", radius);
    }

    Inkscape::XML::Document *xml_doc = document->getReprDoc();

    // If there are more users for this filter, duplicate it
    if (filter->hrefcount > count_filter_hrefs(item, filter)) {
        Inkscape::XML::Node *repr = item->style->getFilter()->getRepr()->duplicate(xml_doc);
        SPDefs *defs = document->getDefs();
        defs->appendChild(repr);

        filter = cast<SPFilter>( document->getObjectByRepr(repr) );
        Inkscape::GC::release(repr);
    }

    // Determine the required standard deviation value
    Geom::Affine i2d (item->i2dt_affine ());
    double expansion = i2d.descrim();
    double stdDeviation = radius;
    if (expansion != 0)
        stdDeviation /= expansion;

    // Set the filter effects area
    SPFilter *f = item->style->getFilter();

    Inkscape::XML::Node *repr = f->getRepr();
    // Search for gaussian blur primitives. If found, set the stdDeviation
    // of the first one and return.
    Inkscape::XML::Node *primitive = repr->firstChild();
    while (primitive) {
        if (strcmp("svg:feGaussianBlur", primitive->name()) == 0) {
            primitive->setAttributeSvgDouble("stdDeviation", stdDeviation);
            return filter;
        }
        primitive = primitive->next();
    }

    // If there were no gaussian blur primitives, create a new one

    //create feGaussianBlur node
    Inkscape::XML::Node *b_repr;
    b_repr = xml_doc->createElement("svg:feGaussianBlur");
    //b_repr->setAttribute("inkscape:collect", "always");
    
    //set stdDeviation attribute
    b_repr->setAttributeSvgDouble("stdDeviation", stdDeviation);
    
    //set feGaussianBlur as child of filter node
    filter->getRepr()->appendChild(b_repr);
    Inkscape::GC::release(b_repr);

    return filter;
}

void remove_filter (SPObject *item, bool recursive)
{
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_unset_property(css, "filter");
    if (recursive) {
        sp_repr_css_change_recursive(item->getRepr(), css, "style");
    } else {
        sp_repr_css_change(item->getRepr(), css, "style");
    }
    sp_repr_css_attr_unref(css);
}

void remove_hidder_filter (SPObject *item)
{
    SPFilter *filt = item->style->getFilter();
    if (filt && filt->getId()) {
        Glib::ustring filter = filt->getId();
        if (!filter.rfind("selectable_hidder_filter", 0)) {
            remove_filter(item, false);
        }
    }
}

bool has_hidder_filter(SPObject const *item)
{
    SPFilter *filt = item->style->getFilter();
    if (filt && filt->getId()) {
        Glib::ustring filter = filt->getId();
        if (!filter.rfind("selectable_hidder_filter", 0)) {
            return true;
        }
    }
    return false;
}

/**
 * Removes the first feGaussianBlur from the filter attached to given item.
 * Should this leave us with an empty filter, remove that filter.
 */
/* TODO: the removed filter primitive may had had a named result image, so
 * after removing, the filter may be in erroneous state, this situation should
 * be handled gracefully */
bool remove_filter_gaussian_blur (SPObject *item)
{
    bool changed = false;

    if (item->style && item->style->filter.set && item->style->getFilter()) {
        // Search for the first blur primitive and remove it. (if found)
        Inkscape::XML::Node *repr = item->style->getFilter()->getRepr();
        Inkscape::XML::Node *primitive = repr->firstChild();
        while (primitive) {
            if (strcmp("svg:feGaussianBlur", primitive->name()) == 0) {
                sp_repr_unparent(primitive);
                changed = true;
                break;
            }
            primitive = primitive->next();
        }

        // If there are no more primitives left in this filter, discard it.
        if (repr->childCount() == 0) {
            remove_filter(item, false);
            changed = true;
        }
    }

    return changed;
}

/**
 * Removes blend primitive from the filter attached to given item.
 * Get if the filter have a < 1.0 blending filter and if it remove it
 * @params: the item to remove filtered blend
 */
/* TODO: the removed filter primitive may had had a named result image, so
 * after removing, the filter may be in erroneous state, this situation should
 * be handled gracefully */
void remove_filter_legacy_blend(SPObject *item)
{
    if (!item) {
        return;
    }
    if (item->style && item->style->filter.set && item->style->getFilter()) {
        // Search for the first blur primitive and remove it. (if found)
        size_t blurcount = 0;
        size_t blendcount = 0;
        size_t total = 0;
        // determine whether filter is simple (blend and/or blur) or complex
        SPFeBlend *blend = nullptr;
        for (auto &primitive_obj:item->style->getFilter()->children) {
            auto primitive = cast<SPFilterPrimitive>(&primitive_obj);
            if (primitive) {
                if (is<SPFeBlend>(primitive)) {
                    blend = cast<SPFeBlend>(primitive);
                    ++blendcount;
                }
                if (is<SPGaussianBlur>(primitive)) {
                    ++blurcount;
                }
                ++total;
            }
        }
        if (blend && total == 2 && blurcount == 1) {
            blend->deleteObject(true);
        } else if (total == 1 && blurcount != 1) {
            remove_filter(item, false);
        }
    }
}

/**
 * Get if the filter have a < 1.0 blending filter 
 * @params: the item to get filtered blend
 */
SPBlendMode filter_get_legacy_blend(SPObject *item)
{
    auto blend = SP_CSS_BLEND_NORMAL;
    if (!item) {
        return blend;
    }
    if (item->style && item->style->filter.set && item->style->getFilter()) {
        // Search for the first blur primitive and remove it. (if found)
        size_t blurcount = 0;
        size_t blendcount = 0;
        size_t total = 0;
        // determine whether filter is simple (blend and/or blur) or complex
        for (auto &primitive_obj:item->style->getFilter()->children) {
            auto primitive = cast<SPFilterPrimitive>(&primitive_obj);
            if (primitive) {
                auto spblend = cast<SPFeBlend>(primitive);
                if (spblend) {
                    ++blendcount;
                    blend = spblend->get_blend_mode();
                }
                if (is<SPGaussianBlur>(primitive)) {
                    ++blurcount;
                }
                ++total;
            }
        }
        if (!((blend && total == 2 && blurcount == 1) || total == 1)) {
            blend = SP_CSS_BLEND_NORMAL;
        }
    }
    return blend;
}

bool filter_is_single_gaussian_blur(SPFilter *filter)
{
    return (filter->children.size() == 1 &&
            is<SPGaussianBlur>(&filter->children.front()));
}

double get_single_gaussian_blur_radius(SPFilter *filter)
{
    if (filter->children.size() == 1 &&
        is<SPGaussianBlur>(&filter->children.front())) {

        auto gb = cast<SPGaussianBlur>(filter->firstChild());
        double x = gb->get_std_deviation().getNumber();
        double y = gb->get_std_deviation().getOptNumber();
        if (x > 0 && y > 0) {
            return MAX(x, y);
        }
        return x;
    }
    return 0.0;
}

static bool _set_blend_mode(SPItem* item, SPBlendMode blend_mode, bool clear) {
    if (!item || !item->style) {
        return false;
    }
    auto& style = item->style;

    bool change_blend = (style->mix_blend_mode.set ? style->mix_blend_mode.value : SP_CSS_BLEND_NORMAL) != blend_mode;
    // < 1.0 filter based blend removal
    if (!style->mix_blend_mode.set && style->filter.set && style->getFilter()) {
        remove_filter_legacy_blend(item);
    }
    if (clear) {
        change_blend = change_blend || style->mix_blend_mode.set;
        style->mix_blend_mode.clear();
    }
    else {
        style->mix_blend_mode.set = true;
        if (style->isolation.value == SP_CSS_ISOLATION_ISOLATE) {
            style->mix_blend_mode.value = SP_CSS_BLEND_NORMAL;
        } else {
            style->mix_blend_mode.value = blend_mode;
        }
    }

    if (change_blend) { // we do blend so we need to update display style
        item->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    }

    return change_blend;
}

bool set_blend_mode(SPItem* item, SPBlendMode blend_mode) {
    return _set_blend_mode(item, blend_mode, false);
}

bool clear_blend_mode(SPItem* item) {
    return _set_blend_mode(item, SP_CSS_BLEND_NORMAL, true);

}

std::optional<double> object_query_blur_filter(const SPObject* object) {
    auto item = cast<SPItem>(object);
    if (!item || !item->style || !item->style->filter.set || !item->style->getFilter()) return {};

    for (auto& primitive: item->style->getFilter()->children) {
        if (auto blur = cast<SPGaussianBlur>(&primitive)) {
            double num = blur->get_std_deviation().getNumber();
            Geom::Affine i2d = item->i2dt_affine();
            double radius = num * i2d.descrim();
            if (!std::isnan(radius)) {
                return radius;
            }
        }
    }

    return {};
}

SPFilterPrimitive* get_first_filter_component(SPObject* object) {
    auto item = cast<SPItem>(object);
    if (!item || !item->style || !item->style->filter.set || !item->style->getFilter()) return nullptr;

    auto& filters = item->style->getFilter()->children;
    for (auto& child : filters) {
        if (auto f = cast<SPFilterPrimitive>(&child)) return f;
    }
    return nullptr;
}

size_t get_filter_primitive_count(const SPObject* object) {
    auto item = cast<SPItem>(object);
    if (!item || !item->style || !item->style->filter.set || !item->style->getFilter()) return 0;

    auto& filters = item->style->getFilter()->children;
    size_t count = 0;
    for (auto& child : filters) {
        // only count filter primitives
        if (is<SPFilterPrimitive>(&child)) ++count;
    }
    return count;
}

constexpr double BLUR_MULTIPLIER = 4.0;

bool modify_filter_gaussian_blur_amount(SPItem* item, double amount) {
    if (!item) return false;

    double radius = 0;
    if (Geom::OptRect bbox = item->desktopGeometricBounds()) {
        double perimeter = bbox->dimensions()[Geom::X] + bbox->dimensions()[Geom::Y];   // fixme: this is only half the perimeter, is that correct?
        double blur_value = amount / 100.0;
        radius = blur_value * blur_value * perimeter / BLUR_MULTIPLIER;
    }
    // if (!radius) return false;

    SPFilter* filter = modify_filter_gaussian_blur_from_item(item->document, item, radius);
    filter->update_filter_region(item);
    sp_style_set_property_url(item, "filter", filter, false);
    return true;
}

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
