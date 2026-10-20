// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <mask> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2003 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-mask.h"

#include <string>
#include <2geom/transforms.h>

#include "display/drawing-group.h"

#include "attributes.h"                              // for SPAttr
#include "document.h"                                // for SPDocument
#include "enums.h"                                   // for SP_CONTENT_UNITS...
#include "sp-defs.h"                                 // for SPDefs
#include "sp-item.h"                                 // for SPItem, SP_ITEM_...
#include "xml/document.h"                            // for Document

SPMask::SPMask()
{
    maskUnits_set = false;
    maskUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;

    maskContentUnits_set = false;
    maskContentUnits = SP_CONTENT_UNITS_USERSPACEONUSE;
}

SPMask::~SPMask() = default;

void SPMask::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
    SPObjectGroup::build(doc, repr);

    readAttr(SPAttr::MASKUNITS);
    readAttr(SPAttr::MASKCONTENTUNITS);
    readAttr(SPAttr::STYLE);

    doc->addResource("mask", this);
}

void SPMask::release()
{
    if (document) {
        document->removeResource("mask", this);
    }

    views.clear();

    SPObjectGroup::release();
}

void SPMask::set(SPAttr key, char const *value)
{
    switch (key) {
        case SPAttr::MASKUNITS:
            maskUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;
            maskUnits_set = false;

            if (value) {
                if (!std::strcmp(value, "userSpaceOnUse")) {
                    maskUnits = SP_CONTENT_UNITS_USERSPACEONUSE;
                    maskUnits_set = true;
                } else if (!std::strcmp(value, "objectBoundingBox")) {
                    maskUnits_set = true;
                }
            }

            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        case SPAttr::MASKCONTENTUNITS:
            maskContentUnits = SP_CONTENT_UNITS_USERSPACEONUSE;
            maskContentUnits_set = false;

            if (value) {
                if (!std::strcmp(value, "userSpaceOnUse")) {
                    maskContentUnits_set = true;
                } else if (!std::strcmp(value, "objectBoundingBox")) {
                    maskContentUnits = SP_CONTENT_UNITS_OBJECTBOUNDINGBOX;
                    maskContentUnits_set = true;
                }
            }

            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;

        default:
            SPObjectGroup::set(key, value);
            break;
    }
}

Geom::OptRect SPMask::geometricBounds(Geom::Affine const &transform) const
{
    Geom::OptRect bbox;

    for (auto &c : children) {
        if (auto item = cast<SPItem>(&c)) {
            bbox.unionWith(item->geometricBounds(item->transform * transform));
        }
    }

    return bbox;
}

Geom::OptRect SPMask::visualBounds(Geom::Affine const &transform) const
{
    Geom::OptRect bbox;

    for (auto &c : children) {
        if (auto item = cast<SPItem>(&c)) {
            bbox.unionWith(item->visualBounds(item->transform * transform));
        }
    }

    return bbox;
}

void SPMask::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPObjectGroup::child_added(child, ref);

    if (auto item = cast<SPItem>(document->getObjectByRepr(child))) {
        for (auto &v : views) {
            auto ac = item->invoke_show(v.drawingitem->drawing(), v.key, SP_ITEM_REFERENCE_FLAGS);
            if (ac) {
                // Fixme: Must take position into account.
                v.drawingitem->prependChild(ac);
            }
        }
    }
}

void SPMask::update(SPCtx *ctx, unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto child : childList(true)) {
        if (cflags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->updateDisplay(ctx, cflags);
        }
        sp_object_unref(child);
    }

    for (auto &v : views) {
        update_view(v);
    }
}

void SPMask::update_view(View &v)
{
    if (maskContentUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX && v.bbox) {
        v.drawingitem->setChildTransform(Geom::Scale(v.bbox->dimensions()) * Geom::Translate(v.bbox->min()));
    } else {
        v.drawingitem->setChildTransform(Geom::identity());
    }
}

void SPMask::modified(unsigned flags)
{
    auto const cflags = cascade_flags(flags);

    for (auto child : childList(true)) {
        if (cflags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(cflags);
        }
        sp_object_unref(child);
    }
}

Inkscape::XML::Node *SPMask::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:mask");
    }

    SPObjectGroup::write(xml_doc, repr, flags);

    return repr;
}

// Create a mask element (using passed elements), add it to <defs>
char const *SPMask::create(std::vector<Inkscape::XML::Node*> &reprs, SPDocument *document)
{
    auto defsrepr = document->getDefs()->getRepr();

    auto xml_doc = document->getReprDoc();
    auto repr = xml_doc->createElement("svg:mask");
    repr->setAttribute("maskUnits", "userSpaceOnUse");
    
    defsrepr->appendChild(repr);
    char const *mask_id = repr->attribute("id");
    auto mask_object = document->getObjectById(mask_id);
    
    for (auto node : reprs) {
        mask_object->appendChildRepr(node);
    }

    if (repr != defsrepr->lastChild()) {
        defsrepr->changeOrder(repr, defsrepr->lastChild()); // workaround for bug 989084
    }
    
    Inkscape::GC::release(repr);
    return mask_id;
}

Inkscape::DrawingItem *SPMask::show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox)
{
    views.emplace_back(make_drawingitem<Inkscape::DrawingGroup>(drawing), bbox, key);
    auto &v = views.back();
    auto root = v.drawingitem.get();

    for (auto &child : children) {
        if (auto item = cast<SPItem>(&child)) {
            auto ac = item->invoke_show(drawing, key, SP_ITEM_REFERENCE_FLAGS);
            if (ac) {
                root->appendChild(ac);
            }
        }
    }

    update_view(v);

    return root;
}

void SPMask::hide(unsigned key)
{
    for (auto &child : children) {
        if (auto item = cast<SPItem>(&child)) {
            item->invoke_hide(key);
        }
    }

    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });
    assert(it != views.end());

    views.erase(it);
}

void SPMask::setBBox(unsigned key, Geom::OptRect const &bbox)
{
    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });
    assert(it != views.end());
    auto &v = *it;

    v.bbox = bbox;
    update_view(v);
}

bool SPMaskReference::_acceptObject(SPObject *obj) const
{
    if (!is<SPMask>(obj)) {
        return false;
    }

    if (URIReference::_acceptObject(obj)) {
        return true;
    }

    auto const owner = getOwner();
    //XML Tree being used directly here while it shouldn't be...
    auto const owner_repr = owner->getRepr();
    //XML Tree being used directly here while it shouldn't be...
    auto const obj_repr = obj->getRepr();
    char const *owner_name = "";
    char const *owner_mask = "";
    char const *obj_name = "";
    char const *obj_id = "";
    if (owner_repr) {
        owner_name = owner_repr->name();
        owner_mask = owner_repr->attribute("mask");
    }
    if (obj_repr) {
        obj_name = obj_repr->name();
        obj_id = obj_repr->attribute("id");
    }
    std::cerr << "WARNING: Ignoring recursive mask reference "
        << "<" << (owner_name ? owner_name : "(null)") << " mask=\"" <<  (owner_mask ? owner_mask : "(null)") << "\">"
        << " in <" << (obj_name ? obj_name : "(null)") << " id=\"" << (obj_id ? obj_id : "(null)") << "\">" << std::endl;

    return false;
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
