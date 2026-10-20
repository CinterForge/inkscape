// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <filter> implementation.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-filter.h"

#include <cstring>
#include <utility>
#include <vector>
#include <unordered_map>

#include <2geom/transforms.h>
#include <glibmm.h>

#include "attributes.h"
#include "bad-uri-exception.h"
#include "display/drawing-item.h"
#include "display/nr-filter.h"
#include "document.h"
#include "filters/sp-filter-primitive.h"
#include "sp-filter-reference.h"
#include "uri.h"
#include "filters/slot-resolver.h"
#include "xml/href-attribute-helper.h"

SPFilter::SPFilter()
    : filterUnits(SP_FILTER_UNITS_OBJECTBOUNDINGBOX)
    , filterUnits_set(false)
    , primitiveUnits(SP_FILTER_UNITS_USERSPACEONUSE)
    , primitiveUnits_set(false)
{
    href = std::make_unique<SPFilterReference>(this);

    // Gets called when the filter is (re)attached to another filter.
    href->changedSignal().connect([this] (SPObject *old_ref, SPObject *ref) {
        if (old_ref) {
            modified_connection.disconnect();
        }

        if (is<SPFilter>(ref) && ref != this) {
            modified_connection = ref->connectModified([this] (SPObject*, unsigned) {
                requestModified(SP_OBJECT_MODIFIED_FLAG);
            });
        }

        requestModified(SP_OBJECT_MODIFIED_FLAG);
    });

    x = 0;
    y = 0;
    width = 0;
    height = 0;
    auto_region = true;
}

SPFilter::~SPFilter() = default;

void SPFilter::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    // Read values of key attributes from XML nodes into object.
    readAttr(SPAttr::STYLE); // struct not derived from SPItem, we need to do this ourselves.
    readAttr(SPAttr::FILTERUNITS);
    readAttr(SPAttr::PRIMITIVEUNITS);
    readAttr(SPAttr::X);
    readAttr(SPAttr::Y);
    readAttr(SPAttr::WIDTH);
    readAttr(SPAttr::HEIGHT);
    readAttr(SPAttr::AUTO_REGION);
    readAttr(SPAttr::FILTERRES);
    readAttr(SPAttr::XLINK_HREF);
    _refcount = 0;

    SPObject::build(document, repr);

    document->addResource("filter", this);
}

void SPFilter::release()
{
    document->removeResource("filter", this);

    if (href) {
        modified_connection.disconnect();
        href->detach();
        href.reset();
    }

    SPObject::release();
}

void SPFilter::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::FILTERUNITS:
            if (value) {
                if (!std::strcmp(value, "userSpaceOnUse")) {
                    filterUnits = SP_FILTER_UNITS_USERSPACEONUSE;
                } else {
                    filterUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
                }
                filterUnits_set = true;
            } else {
                filterUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
                filterUnits_set = false;
            }
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::PRIMITIVEUNITS:
            if (value) {
                if (!std::strcmp(value, "objectBoundingBox")) {
                    primitiveUnits = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
                } else {
                    primitiveUnits = SP_FILTER_UNITS_USERSPACEONUSE;
                }
                primitiveUnits_set = true;
            } else {
                primitiveUnits = SP_FILTER_UNITS_USERSPACEONUSE;
                primitiveUnits_set = false;
            }
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::X:
            x.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::Y:
            y.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::WIDTH:
            width.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::HEIGHT:
            height.readOrUnset(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::AUTO_REGION:
            auto_region = !value || std::strcmp(value, "false");
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::FILTERRES:
            filterRes.set(value);
            requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SPAttr::XLINK_HREF:
            if (value) {
                try {
                    href->attach(Inkscape::URI(value));
                } catch (Inkscape::BadURIException const &e) {
                    g_warning("%s", e.what());
                    href->detach();
                }
            } else {
                href->detach();
            }
            break;
        default:
            // See if any parents need this value.
            SPObject::set(key, value);
            break;
    }
}

/**
 * Returns the number of references to the filter.
 */
unsigned SPFilter::getRefCount()
{
    // NOTE: this is currently updated by sp_style_filter_ref_changed() in style.cpp
    return _refcount;
}

void SPFilter::update(SPCtx *ctx, unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    ensure_slots();

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        auto ictx = static_cast<SPItemCtx*>(ctx);

        // Do here since we know viewport (Bounding box case handled during rendering)
        // Note: This only works for root viewport since this routine is not called after
        // setting a new viewport. A true fix requires a strategy like SPItemView or SPMarkerView.
        if (filterUnits == SP_FILTER_UNITS_USERSPACEONUSE) {
            calcDimsFromParentViewport(ictx, true);
        }
    }

    // Update filter primitives in order to update filter primitive area
    for (auto &c : children) {
        if (cflags || (c.uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c.updateDisplay(ctx, cflags);
        }
    }

    SPObject::update(ctx, flags);
}

void SPFilter::modified(unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    // We are not an LPE, do not update filter regions on load.
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG)) {
        update_filter_all_regions();
    }

    for (auto &c : children) {
        if (cflags || (c.mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            c.emitModified(cflags);
        }
    }

    for (auto item : views) {
        item->setFilterRenderer(build_renderer(item));
    }
}

Inkscape::XML::Node *SPFilter::write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags)
{
    // Original from sp-item-group.cpp
    if (flags & SP_OBJECT_WRITE_BUILD) {
        if (!repr) {
            repr = doc->createElement("svg:filter");
        }

        std::vector<Inkscape::XML::Node *> l;
        for (auto &child : children) {
            auto crepr = child.updateRepr(doc, nullptr, flags);
            if (crepr) {
                l.push_back(crepr);
            }
        }

        for (auto i = l.rbegin(); i != l.rend(); ++i) {
            repr->addChild(*i, nullptr);
            Inkscape::GC::release(*i);
        }
    } else {
        for (auto &child : children) {
            child.updateRepr(flags);
        }
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || filterUnits_set) {
        switch (filterUnits) {
            case SP_FILTER_UNITS_USERSPACEONUSE:
                repr->setAttribute("filterUnits", "userSpaceOnUse");
                break;
            default:
                repr->setAttribute("filterUnits", "objectBoundingBox");
                break;
        }
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || primitiveUnits_set) {
        switch (primitiveUnits) {
            case SP_FILTER_UNITS_OBJECTBOUNDINGBOX:
                repr->setAttribute("primitiveUnits", "objectBoundingBox");
                break;
            default:
                repr->setAttribute("primitiveUnits", "userSpaceOnUse");
                break;
        }
    }

    if (x._set) {
        repr->setAttributeSvgDouble("x", x.computed);
    } else {
        repr->removeAttribute("x");
    }

    if (y._set) {
        repr->setAttributeSvgDouble("y", y.computed);
    } else {
        repr->removeAttribute("y");
    }

    if (width._set) {
        repr->setAttributeSvgDouble("width", width.computed);
    } else {
        repr->removeAttribute("width");
    }

    if (height._set) {
        repr->setAttributeSvgDouble("height", height.computed);
    } else {
        repr->removeAttribute("height");
    }

    if (filterRes.getNumber() >= 0) {
        auto tmp = filterRes.getValueString();
        repr->setAttribute("filterRes", tmp);
    } else {
        repr->removeAttribute("filterRes");
    }

    if (href->getURI()) {
        auto uri_string = href->getURI()->str();
        auto href_key = Inkscape::getHrefAttribute(*repr).first;
        repr->setAttributeOrRemoveIfEmpty(href_key, uri_string);
    }

    SPObject::write(doc, repr, flags);

    return repr;
}

/**
 * Update the filter's region based on its detectable href links
 *
 * Automatic region only updated if auto_region is false
 * and filterUnits is not UserSpaceOnUse
 */
void SPFilter::update_filter_all_regions()
{
    if (!auto_region || filterUnits == SP_FILTER_UNITS_USERSPACEONUSE) {
        return;
    }

    // Combine all items into one region for updating.
    Geom::OptRect opt_r;
    for (auto &obj : hrefList) {
        auto item = cast<SPItem>(obj);
        opt_r.unionWith(get_automatic_filter_region(item));
    }
    if (opt_r) {
        Geom::Rect region = *opt_r;
        set_filter_region(region.left(), region.top(), region.width(), region.height());
    }
}

/**
 * Update the filter region based on the object's bounding box
 *
 * @param item - The item whose coords are used as the basis for the area.
 */
void SPFilter::update_filter_region(SPItem *item)
{
    if (!auto_region || filterUnits == SP_FILTER_UNITS_USERSPACEONUSE) {
        return; // No adjustment for dead box
    }

    auto region = get_automatic_filter_region(item);

    // Set the filter region into this filter object
    set_filter_region(region.left(), region.top(), region.width(), region.height());
}

/**
 * Generate a filter region based on the item and return it.
 *
 * @param item - The item whose coords are used as the basis for the area.
 */
Geom::Rect SPFilter::get_automatic_filter_region(SPItem const *item) const
{
    // Calling bbox instead of visualBound() avoids re-requesting filter regions
    Geom::OptRect v_box = item->bbox(Geom::identity(), SPItem::VISUAL_BBOX);
    Geom::OptRect g_box = item->bbox(Geom::identity(), SPItem::GEOMETRIC_BBOX);
    if (!v_box || !g_box) {
        return Geom::Rect(); // No adjustment for dead box
    }

    // Because the filter box is in geometric bounding box units, it must ALSO
    // take account of the visualBox, so even if the filter does NOTHING to the
    // size of an object, we must add the difference between the geometric and
    // visual boxes ourselves or find them cut off by renderers of all kinds.
    Geom::Rect inbox = *g_box;
    Geom::Rect outbox = *v_box;
    for (auto &primitive_obj : children) {
        auto primitive = cast<SPFilterPrimitive>(&primitive_obj);
        if (primitive) {
            // Update the region with the primitive's options
            outbox = primitive->calculate_region(outbox);
        }
    }

    // Include the original visual bounding-box in the result
    outbox.unionWith(v_box);
    // Scale outbox to width/height scale of input, this scales the geometric
    // into the visual bounding box requiring any changes to it to re-run this.
    outbox *= Geom::Translate(-inbox.left(), -inbox.top());
    outbox *= Geom::Scale(1.0 / inbox.width(), 1.0 / inbox.height());
    return outbox;
}

/**
 * Set the filter region attributes from a bounding box
 */
void SPFilter::set_filter_region(double x, double y, double width, double height)
{
    if (width != 0 && height != 0) {
        // TODO: set it in UserSpaceOnUse instead?
        auto repr = getRepr();
        repr->setAttributeSvgDouble("x", x);
        repr->setAttributeSvgDouble("y", y);
        repr->setAttributeSvgDouble("width", width);
        repr->setAttributeSvgDouble("height", height);
    }
}

/**
 * Check each filter primitive for conflicts with this object.
 */
bool SPFilter::valid_for(SPObject const *obj) const
{
    for (auto &primitive_obj : children) {
        auto primitive = cast<SPFilterPrimitive>(&primitive_obj);
        if (primitive && !primitive->valid_for(obj)) {
            return false;
        }
    }
    return true;
}

void SPFilter::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPObject::child_added(child, ref);

    if (auto f = cast<SPFilterPrimitive>(get_child_by_repr(child))) {
        for (auto &v : views) {
            f->show(v);
        }
    }

    invalidate_slots();
}

void SPFilter::remove_child(Inkscape::XML::Node *child)
{
    if (auto f = cast<SPFilterPrimitive>(get_child_by_repr(child))) {
        for (auto &v : views) {
            f->hide(v);
        }
    }

    SPObject::remove_child(child);

    invalidate_slots();
}

void SPFilter::order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_repr, Inkscape::XML::Node *new_repr)
{
    SPObject::order_changed(child, old_repr, new_repr);
    invalidate_slots();
}

void SPFilter::invalidate_slots()
{
    if (!slots_valid) return;
    slots_valid = false;
    requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPFilter::ensure_slots()
{
    if (slots_valid) return;
    slots_valid = true;

    SlotResolver resolver;

    for (auto &c : children) {
        if (auto prim = cast<SPFilterPrimitive>(&c)) {
            prim->resolve_slots(resolver);
        }
    }
}

std::unique_ptr<Inkscape::Filters::Filter> SPFilter::build_renderer(Inkscape::DrawingItem *item)
{
    auto nr_filter = std::make_unique<Inkscape::Filters::Filter>(primitive_count());

    ensure_slots();

    nr_filter->set_filter_units(filterUnits);
    nr_filter->set_primitive_units(primitiveUnits);
    nr_filter->set_x(x);
    nr_filter->set_y(y);
    nr_filter->set_width(width);
    nr_filter->set_height(height);

    if (filterRes.getNumber() >= 0) {
        if (filterRes.getOptNumber() >= 0) {
            nr_filter->set_resolution(filterRes.getNumber(), filterRes.getOptNumber());
        } else {
            nr_filter->set_resolution(filterRes.getNumber());
        }
    }

    nr_filter->clear_primitives();
    for (auto &primitive_obj : children) {
        if (auto primitive = cast<SPFilterPrimitive>(&primitive_obj)) {
            nr_filter->add_primitive(primitive->build_renderer(item));
        }
    }

    return nr_filter;
}

int SPFilter::primitive_count() const
{
    int count = 0;

    for (auto const &primitive_obj : children) {
        if (is<SPFilterPrimitive>(&primitive_obj)) {
            count++;
        }
    }

    return count;
}

Glib::ustring SPFilter::get_new_result_name() const
{
    int largest = 0;

    for (auto const &primitive_obj : children) {
        if (is<SPFilterPrimitive>(&primitive_obj)) {
            auto repr = primitive_obj.getRepr();
            auto result = repr->attribute("result");
            if (result) {
                int index;
                if (std::sscanf(result, "result%5d", &index) == 1) {
                    if (index > largest) {
                        largest = index;
                    }
                }
            }
        }
    }

    return "result" + Glib::Ascii::dtostr(largest + 1);
}

void SPFilter::show(Inkscape::DrawingItem *item)
{
    views.emplace_back(item);

    for (auto &c : children) {
        if (auto f = cast<SPFilterPrimitive>(&c)) {
            f->show(item);
        }
    }

    item->setFilterRenderer(build_renderer(item));
}

void SPFilter::hide(Inkscape::DrawingItem *item)
{
    auto it = std::find(views.begin(), views.end(), item);
    assert(it != views.end());
    views.erase(it);

    for (auto &c : children) {
        if (auto f = cast<SPFilterPrimitive>(&c)) {
            f->hide(item);
        }
    }

    item->setFilterRenderer(nullptr);
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
