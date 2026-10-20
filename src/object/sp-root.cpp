// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG \<svg\> implementation.
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-root.h"

#include <2geom/affine.h>                            // for identity, Affine
#include "attributes.h"                              // for SPAttr
#include "display/drawing-group.h"                   // for DrawingGroup
#include "display/drawing-item-ptr.h"                // for DrawingItemPtr
#include "document.h"                                // for SPDocument
#include "inkscape-version.h"
#include "print.h"                                   // for SPPrintContext
#include <2geom/rect.h>                              // for Rect
#include "sp-defs.h"                                 // for SPDefs
#include "sp-namedview.h"                            // for SPNamedView
#include "sp-use.h"                                  // for SPUse
#include "sp-item-group.h"                           // for SPGroup
#include "sp-item.h"                                 // for SPItemCtx, SPIte...
#include "sp-object.h"                               // for SP_OBJECT_VIEWPO...
#include "viewbox.h"                                 // for SPViewBox

#include "svg/svg-length.h"                          // for SVGLength
#include "svg/svg.h"                                 // for sp_svg_length_wr...
#include "util/units.h"                              // for Quantity
#include "xml/document.h"                            // for Document

namespace Inkscape {
class Drawing;
} // namespace Inkscape

SPRoot::SPRoot() : SPGroup(), SPViewBox()
{
    this->onload = nullptr;

    this->unset_x_and_y();
    this->width.unset(SVGLength::PERCENT, 1.0, 1.0);
    this->height.unset(SVGLength::PERCENT, 1.0, 1.0);

    this->defs = nullptr;
}

SPRoot::~SPRoot()
= default;

void SPRoot::unset_x_and_y()
{
    this->x.unset(SVGLength::PERCENT, 0.0, 0.0); // Ignored for root SVG element
    this->y.unset(SVGLength::PERCENT, 0.0, 0.0);
}

void SPRoot::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    //XML Tree being used directly here while it shouldn't be.
    if (!this->getRepr()->attribute("version")) {
        repr->setAttribute("version", SVG_VERSION);
    }

    this->readAttr(SPAttr::VERSION);
    this->readAttr(SPAttr::INKSCAPE_VERSION);
    /* It is important to parse these here, so objects will have viewport build-time */
    this->readAttr(SPAttr::X);
    this->readAttr(SPAttr::Y);
    this->readAttr(SPAttr::WIDTH);
    this->readAttr(SPAttr::HEIGHT);
    this->readAttr(SPAttr::VIEWBOX);
    this->readAttr(SPAttr::PRESERVEASPECTRATIO);
    this->readAttr(SPAttr::ONLOAD);

    SPGroup::build(document, repr);

    // Search for first <defs> node
    for (auto& o: children) {
        if (is<SPDefs>(&o)) {
            this->defs = cast<SPDefs>(&o);
            break;
        }
    }

    // clear transform, if any was read in - SVG does not allow transform= on <svg>
    this->transform = Geom::identity();
}

void SPRoot::release()
{
    this->defs = nullptr;

    SPGroup::release();
}


void SPRoot::set(SPAttr key, const gchar *value)
{
    switch (key) {
    case SPAttr::VERSION:
        if (auto version = Inkscape::Version::from_string(value)) {
            svg_version = *version;
        } else {
            svg_version = *Inkscape::Version::from_string(SVG_VERSION);
        }
        break;

    case SPAttr::INKSCAPE_VERSION:
        if (auto version = Inkscape::Version::from_string(value)) {
            inkscape_version = *version;
        }
        break;

    case SPAttr::X:
        /* Valid for non-root SVG elements; ex, em not handled correctly. */
        if (!this->x.read(value)) {
            this->x.unset(SVGLength::PERCENT, 0.0, 0.0);
        }
        root_x = x;

        /* fixme: I am almost sure these do not require viewport flag (Lauris) */
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::Y:
        /* Valid for non-root SVG elements; ex, em not handled correctly. */
        if (!this->y.read(value)) {
            this->y.unset(SVGLength::PERCENT, 0.0, 0.0);
        }
        root_y = y;

        /* fixme: I am almost sure these do not require viewport flag (Lauris) */
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::WIDTH:
        if (!this->width.read(value) || !(this->width.computed > 0.0)) {
            this->width.unset(SVGLength::PERCENT, 1.0, 1.0);
        }
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::HEIGHT:
        if (!this->height.read(value) || !(this->height.computed > 0.0)) {
            this->height.unset(SVGLength::PERCENT, 1.0, 1.0);
        }
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::VIEWBOX:
        set_viewBox( value );
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::PRESERVEASPECTRATIO:
        set_preserveAspectRatio( value );
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
        break;

    case SPAttr::ONLOAD:
        this->onload = (char *) value;
        break;

    default:
        /* Pass the set event to the parent */
        SPGroup::set(key, value);
        break;
    }
}

void SPRoot::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref)
{
    SPGroup::child_added(child, ref);

    SPObject *co = this->document->getObjectByRepr(child);
    // NOTE: some XML nodes do not have corresponding SP objects,
    // for instance inkscape:clipboard used in the clipboard code.
    // See LP bug #1227827
    //g_assert (co != NULL || !strcmp("comment", child->name())); // comment repr node has no object

    if (co && is<SPDefs>(co)) {
        // We search for first <defs> node - it is not beautiful, but works
        for (auto& c: children) {
            if (is<SPDefs>(&c)) {
                this->defs = cast<SPDefs>(&c);
                break;
            }
        }
    }
}

void SPRoot::remove_child(Inkscape::XML::Node *child)
{
    if (this->defs && (this->defs->getRepr() == child)) {
        SPObject *iter = nullptr;

        // We search for first remaining <defs> node - it is not beautiful, but works
        for (auto& child: children) {
            iter = &child;
            if (is<SPDefs>(iter) && (SPDefs *)iter != this->defs) {
                this->defs = (SPDefs *)iter;
                break;
            }
        }

        if (!iter) {
            /* we should probably create a new <defs> here? */
            this->defs = nullptr;
        }
    }

    SPGroup::remove_child(child);
}

void SPRoot::setRootDimensions()
{
    /*
     * This is the root SVG element:
     *
     * x, y, width, and height apply to positioning the SVG element inside a parent.
     * For the root SVG in Inkscape there is no parent, thus special rules apply:
     *   If width, height not set, width = 100%, height = 100% (as always).
     *   If width and height are in percent, they are percent of viewBox width/height.
     *   If width, height, and viewBox are not set... pick "random" width/height.
     *   x, y are ignored.
     *   initial viewport = (0 0 width height)
     */
    if( this->viewBox_set ) {

        if( this->width._set ) {
            // Check if this is necessary
            if (this->width.unit == SVGLength::PERCENT) {
                this->width.computed  = this->width.value  * this->viewBox.width();
            }
        } else {
            this->width.set( SVGLength::PX, this->viewBox.width(),  this->viewBox.width()  );
        }

        if( this->height._set ) {
            if (this->height.unit == SVGLength::PERCENT) {
                this->height.computed = this->height.value * this->viewBox.height();
            }
        } else {
            this->height.set(SVGLength::PX, this->viewBox.height(), this->viewBox.height() );
        }

    } else {

        if( !this->width._set || this->width.unit == SVGLength::PERCENT) {
            this->width.set(  SVGLength::PX, 300,  300 ); // CSS/SVG default
        }

        if( !this->height._set || this->height.unit == SVGLength::PERCENT) {
            this->height.set( SVGLength::PX, 150,  150 ); // CSS/SVG default
        }
    }

    // Ignore x, y values for root element
    this->unset_x_and_y();
}

void SPRoot::update(SPCtx *ctx, guint flags)
{
    SPItemCtx const *ictx = (SPItemCtx const *) ctx;

    if( !this->parent ) {
        this->setRootDimensions();
    }

    // Calculate x, y, width, height from parent/initial viewport
    this->calcDimsFromParentViewport(ictx, false, cloned ? cast<SPUse>(parent) : nullptr);

    // std::cout << "SPRoot::update: final:"
    //           << " x: " << x.computed
    //           << " y: " << y.computed
    //           << " width: " << width.computed
    //           << " height: " << height.computed << std::endl;

    // Calculate new viewport
    SPItemCtx rctx = *ictx;
    rctx.viewport = Geom::Rect::from_xywh( this->x.computed, this->y.computed,
                                           this->width.computed, this->height.computed );
    rctx = get_rctx( &rctx, Inkscape::Util::Quantity::convert(1, this->document->getDisplayUnit(), "px") );

    /* And invoke parent method */
    SPGroup::update((SPCtx *) &rctx, flags);

    /* As last step set additional transform of drawing group */
    for (auto &v : views) {
        auto g = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
        g->setChildTransform(c2p);
    }
}

void SPRoot::modified(unsigned int flags)
{
    SPGroup::modified(flags);

    if (!this->parent && (flags & SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        // Size of viewport has changed.
        document->getNamedView()->updateViewPort();
    }
}


Inkscape::XML::Node *SPRoot::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:svg");
    }

    if (!repr->attribute("version")) {
        repr->setAttribute("version", svg_version.str());
    }

    // Inkscape_version doesn't need to be written as it's always
    // replaced by the SVG output extension.

    if (fabs(this->x.computed) > 1e-9) {
        repr->setAttributeSvgDouble("x", this->x.computed);
    } else if (this->root_x) {
        // Maintain data for root svgs, even when we don't use it
        repr->setAttributeSvgDouble("x", this->root_x.computed);
    }

    if (fabs(this->y.computed) > 1e-9) {
        repr->setAttributeSvgDouble("y", this->y.computed);
    } else if (this->root_y) {
        // Maintain data for root svgs, even when we don't use it
        repr->setAttributeSvgDouble("y", this->root_y.computed);
    }

    /* Unlike all other SPObject, here we want to preserve absolute units too (and only here,
     * according to the recommendation in http://www.w3.org/TR/SVG11/coords.html#Units).
     */
    repr->setAttribute("width", sp_svg_length_write_with_units(this->width));
    repr->setAttribute("height", sp_svg_length_write_with_units(this->height));

    this->write_viewBox(repr);
    this->write_preserveAspectRatio(repr);

    SPGroup::write(xml_doc, repr, flags);

    return repr;
}

Inkscape::DrawingItem *SPRoot::show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags)
{
    Inkscape::DrawingItem *ai = SPGroup::show(drawing, key, flags);

    if (ai) {
        auto g = cast<Inkscape::DrawingGroup>(ai);
        g->setChildTransform(this->c2p);
    }

    // Uncomment to print out XML tree
    // getRepr()->recursivePrintTree(0);

    // Uncomment to print out SP Object tree
    // recursivePrintTree(0);

    // Uncomment to print out Display Item tree
    // ai->recursivePrintTree(0);

    return ai;
}

void SPRoot::print(SPPrintContext *ctx)
{
    ctx->bind(this->c2p, 1.0);

    SPGroup::print(ctx);

    ctx->release();
}

const char *SPRoot::typeName() const {
    return "image";
}

const char *SPRoot::displayName() const {
    return "SVG";  // Do not translate
}

void SPRoot::updateDocVersion()
{
    getRepr()->setAttribute("inkscape:version", Inkscape::version_string);
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
