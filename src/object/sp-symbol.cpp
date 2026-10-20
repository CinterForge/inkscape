// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <symbol> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-symbol.h"

#include <glibmm/i18n.h>

#include <2geom/transforms.h>
#include <2geom/pathvector.h>

#include "attributes.h"
#include "desktop.h"
#include "document.h"
#include "inkscape.h"
#include "layer-manager.h"
#include "print.h"
#include "sp-use.h"

#include "display/drawing-group.h"
#include "svg/svg.h"
#include "xml/document.h"                            // for Document

namespace Inkscape {
class Drawing;
} // namespace Inkscape

SPSymbol::SPSymbol() : SPGroup(), SPViewBox() {
}

SPSymbol::~SPSymbol() = default;

void SPSymbol::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::REFX);
    this->readAttr(SPAttr::REFY);
    this->readAttr(SPAttr::X);
    this->readAttr(SPAttr::Y);
    this->readAttr(SPAttr::WIDTH);
    this->readAttr(SPAttr::HEIGHT);
    this->readAttr(SPAttr::VIEWBOX);
    this->readAttr(SPAttr::PRESERVEASPECTRATIO);

    SPGroup::build(document, repr);

    document->addResource("symbol", this);
}

void SPSymbol::release() {
    if (document) {
        document->removeResource("symbol", this);
    }

	SPGroup::release();
}

void SPSymbol::set(SPAttr key, const gchar* value) {
    switch (key) {
    case SPAttr::REFX:
        value = Inkscape::refX_named_to_percent(value);
        this->refX.readOrUnset(value);
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::REFY:
        value = Inkscape::refY_named_to_percent(value);
        this->refY.readOrUnset(value);
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::X:
        this->x.readOrUnset(value);
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::Y:
        this->y.readOrUnset(value);
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::WIDTH:
        this->width.readOrUnset(value, SVGLength::PERCENT, 1.0, 1.0);
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::HEIGHT:
        this->height.readOrUnset(value, SVGLength::PERCENT, 1.0, 1.0);
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::VIEWBOX:
        set_viewBox( value );
        // std::cout << "Symbol: ViewBox: " << viewBox << std::endl;
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::PRESERVEASPECTRATIO:
        set_preserveAspectRatio( value );
        // std::cout << "Symbol: Preserve aspect ratio: " << aspect_align << ", " << aspect_clip << std::endl;
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    default:
        SPGroup::set(key, value);
        break;
    }
}

void SPSymbol::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
	SPGroup::child_added(child, ref);
}

void SPSymbol::unSymbol()
{
    SPDocument *doc = this->document;
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    // Check if something is selected.

    doc->ensureUpToDate();

    // Create new <g> and insert in current layer
    Inkscape::XML::Node *group = xml_doc->createElement("svg:g");
    //TODO: Better handle if no desktop, currently go to defs without it
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if(desktop && desktop->doc() == doc) {
        desktop->layerManager().currentLayer()->getRepr()->appendChild(group);
    } else {
        parent->getRepr()->appendChild(group);
    }

    // Move all children of symbol to group
    std::vector<SPObject*> children = childList(false);

    // Converting a group to a symbol inserts a group for non-translational transform.
    // In converting a symbol back to a group we strip out the inserted group (or any other
    // group that only adds a transform to the symbol content).
    if( children.size() == 1 ) {
        SPObject *object = children[0];
        if (is<SPGroup>( object ) ) {
            if( object->getAttribute("style") == nullptr ||
                object->getAttribute("class") == nullptr ) {

                group->setAttribute("transform", object->getAttribute("transform"));
                children = object->childList(false);
            }
        }
    }
    for (std::vector<SPObject*>::const_reverse_iterator i=children.rbegin();i!=children.rend();++i){
        Inkscape::XML::Node *repr = (*i)->getRepr();
        repr->parent()->removeChild(repr);
        group->addChild(repr,nullptr);
    }

    // Copy relevant attributes
    group->setAttribute("style", getAttribute("style"));
    group->setAttribute("class", getAttribute("class"));
    group->setAttribute("title", getAttribute("title"));
    group->setAttribute("inkscape:transform-center-x",
                        getAttribute("inkscape:transform-center-x"));
    group->setAttribute("inkscape:transform-center-y",
                        getAttribute("inkscape:transform-center-y"));


    // Need to delete <symbol>; all <use> elements that referenced <symbol> should
    // auto-magically reference <g> (if <symbol> deleted after setting <g> 'id').
    Glib::ustring id = getAttribute("id");
    group->setAttribute("id", id);
    
    deleteObject(true);

    // Clean up
    Inkscape::GC::release(group);
}

std::optional<Geom::PathVector> SPSymbol::documentExactBounds() const
{
    Geom::PathVector shape;
    bool is_empty = true;
    for (auto &child : children) {
        if (auto const item = cast<SPItem>(&child)) {
            if (auto bounds = item->documentExactBounds()) {
                shape.insert(shape.end(), bounds->begin(), bounds->end());
                is_empty = false;
            }
        }
    }
    std::optional<Geom::PathVector> result;
    if (!is_empty) {
        result = shape * i2doc_affine();
    }
    return result;
}

void SPSymbol::update(SPCtx *ctx, guint flags) {
    if (this->cloned) {

        SPItemCtx *ictx = (SPItemCtx *) ctx;

        // Calculate x, y, width, height from parent/initial viewport
        this->calcDimsFromParentViewport(ictx, false, cast<SPUse>(parent));

        SPItemCtx rctx = *ictx;
        rctx.viewport = Geom::Rect::from_xywh(x.computed, y.computed, width.computed, height.computed);
        rctx = get_rctx(&rctx);

        // Shift according to refX, refY
        if (refX._set && refY._set) {
            refX.update(1, 1, viewBox.width());
            refY.update(1, 1, viewBox.height());
            auto ref = Geom::Point(refX.computed, refY.computed) * c2p;
            c2p *= Geom::Translate(-ref);
        }

        // And invoke parent method
        SPGroup::update((SPCtx *) &rctx, flags);

        // As last step set additional transform of drawing group
        for (auto &v : views) {
            auto g = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
            g->setChildTransform(this->c2p);
        }
    } else {
        // No-op
        SPGroup::update(ctx, flags);
    }
}

void SPSymbol::modified(unsigned int flags) {
	SPGroup::modified(flags);
}


Inkscape::XML::Node* SPSymbol::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:symbol");
    }

    if (refX._set) {
        repr->setAttribute("refX", sp_svg_length_write_with_units(refX));
    }
    if (refY._set) {
        repr->setAttribute("refY", sp_svg_length_write_with_units(refY));
    }

    this->writeDimensions(repr);
    this->write_viewBox(repr);
    this->write_preserveAspectRatio(repr);

    SPGroup::write(xml_doc, repr, flags);

    return repr;
}

Inkscape::DrawingItem* SPSymbol::show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags)
{
    Inkscape::DrawingItem *ai = nullptr;

    if (cloned) {
        // Cloned <symbol> is actually renderable
        ai = SPGroup::show(drawing, key, flags);

        if (auto g = cast<Inkscape::DrawingGroup>(ai)) {
			g->setChildTransform(this->c2p);
		}
    }

    return ai;
}

void SPSymbol::hide(unsigned int key) {
    if (this->cloned) {
        /* Cloned <symbol> is actually renderable */
        SPGroup::hide(key);
    }
}


Geom::OptRect SPSymbol::bbox(Geom::Affine const &transform, SPItem::BBoxType type) const
{
    Geom::Affine const a = cloned ? c2p * transform : Geom::identity();
    return SPGroup::bbox(a, type);
}

void SPSymbol::print(SPPrintContext* ctx) {
    if (this->cloned) {
        // Cloned <symbol> is actually renderable

        ctx->bind(this->c2p, 1.0);

        SPGroup::print(ctx);

        ctx->release ();
    }
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
