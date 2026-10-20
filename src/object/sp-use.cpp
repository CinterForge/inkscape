// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <use> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2005 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>

#include <2geom/transforms.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>

#include "attributes.h"
#include "bad-uri-exception.h"
#include "document.h"
#include "enums.h"                                   // for SP_CLONE_COMPENS...
#include "preferences.h"
#include "print.h"
#include "sp-clippath.h"
#include "sp-factory.h"
#include "sp-flowregion.h"
#include "sp-flowtext.h"
#include "sp-mask.h"
#include "sp-root.h"
#include "sp-shape.h"
#include "sp-symbol.h"
#include "sp-text.h"
#include "snap-candidate.h"
#include "sp-use-reference.h"
#include "sp-use.h"
#include "style.h"
#include "uri.h"

#include "display/drawing-group.h"
#include "xml/document.h"                            // for Document
#include "xml/href-attribute-helper.h"               // for getHrefAttribute

namespace Inkscape {
class Drawing;
class SnapCandidatePoint;
class SnapPreferences;
} // namespace Inkscape

SPUse::SPUse()
    : SPItem(),
      SPDimensions(),
      child(nullptr),
      href(nullptr),
      ref(new SPUseReference(this)),
      _delete_connection(),
      _changed_connection(),
      _transformed_connection()
{
    this->x.unset();
    this->y.unset();
    this->width.unset(SVGLength::PERCENT, 1.0, 1.0);
    this->height.unset(SVGLength::PERCENT, 1.0, 1.0);

    this->_changed_connection = this->ref->changedSignal().connect(
        sigc::hide(sigc::hide(sigc::mem_fun(*this, &SPUse::href_changed)))
    );
}

SPUse::~SPUse() {
    if (this->child) {
        this->detach(this->child);
        this->child = nullptr;
    }

    this->ref->detach();
    delete this->ref;
    this->ref = nullptr;
}

void SPUse::build(SPDocument *document, Inkscape::XML::Node *repr) {
    SPItem::build(document, repr);

    this->readAttr(SPAttr::X);
    this->readAttr(SPAttr::Y);
    this->readAttr(SPAttr::WIDTH);
    this->readAttr(SPAttr::HEIGHT);
    this->readAttr(SPAttr::XLINK_HREF);

    // We don't need to create child here:
    // reading xlink:href will attach ref, and that will cause the changed signal to be emitted,
    // which will call SPUse::href_changed, and that will take care of the child
}

void SPUse::release() {
    if (this->child) {
        this->detach(this->child);
        this->child = nullptr;
    }

    this->_delete_connection.disconnect();
    this->_changed_connection.disconnect();
    this->_transformed_connection.disconnect();

    g_free(this->href);
    this->href = nullptr;

    this->ref->detach();

    SPItem::release();
}

void SPUse::set(SPAttr key, const gchar* value) {
    switch (key) {
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

        case SPAttr::XLINK_HREF: {
            if ( value && this->href && ( strcmp(value, this->href) == 0 ) ) {
                /* No change, do nothing. */
            } else {
                g_free(this->href);
                this->href = nullptr;

                if (value) {
                    // First, set the href field, because SPUse::href_changed will need it.
                    this->href = g_strdup(value);

                    // Now do the attaching, which emits the changed signal.
                    try {
                        this->ref->attach(Inkscape::URI(value));
                    } catch (Inkscape::BadURIException &e) {
                        g_warning("%s", e.what());
                        this->ref->detach();
                    }
                } else {
                    this->ref->detach();
                }
            }
            break;
        }

        default:
            SPItem::set(key, value);
            break;
    }
}

Inkscape::XML::Node* SPUse::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:use");
    }

    SPItem::write(xml_doc, repr, flags);

    this->writeDimensions(repr);

    if (this->ref->getURI()) {
        auto uri_string = this->ref->getURI()->str();
        auto href_key = Inkscape::getHrefAttribute(*repr).first;
        repr->setAttributeOrRemoveIfEmpty(href_key, uri_string);
    }

    if (auto shape = cast<SPShape>(child)) {
        shape->set_shape(); // evaluate curve of child
    } else if (auto text = cast<SPText>(child)) {
        text->rebuildLayout(); // refresh Layout, LP Bug 1339305
    } else if (auto flowtext = cast<SPFlowtext>(child)) {
        if (auto flowregion = cast<SPFlowregion>(flowtext->firstChild())) {
            flowregion->updateComputed();
        }
        flowtext->rebuildLayout();
    }

    return repr;
}

Geom::OptRect SPUse::bbox(Geom::Affine const &transform, SPItem::BBoxType bboxtype) const {
    Geom::OptRect bbox;

    if (this->child) {
        Geom::Affine const ct(child->transform * Geom::Translate(this->x.computed, this->y.computed) * transform );

        bbox = child->bounds(bboxtype, ct);
    }

    return bbox;
}

std::optional<Geom::PathVector> SPUse::documentExactBounds() const
{
    std::optional<Geom::PathVector> result;
    auto *original = trueOriginal();
    if (!original) {
        return result;
    }
    result = original->documentExactBounds();

    Geom::Affine private_transform;
    if (is<SPSymbol>(original)) {
        private_transform = i2doc_affine();
    } else if (auto const *parent = cast<SPItem>(original->parent)) {
        private_transform = get_root_transform() * parent->transform.inverse() * parent->i2doc_affine();
    }
    result = result ? (*result // TODO: is there a simpler way to get the transform below?
                       * original->i2doc_affine().inverse()
                       * private_transform)
                    : result;
    return result;
}

void SPUse::print(SPPrintContext* ctx) {
    if (has_xy_offset()) {
        ctx->bind(Geom::Translate(this->x.computed, this->y.computed), 1.0);
    }

    if (this->child) {
        this->child->invoke_print(ctx);
    }

    if (has_xy_offset()) {
        ctx->release();
    }
}

const char* SPUse::typeName() const {
    if (is<SPSymbol>(child)) {
        return "symbol";
    } else {
        return "clone";
    }
}

const char* SPUse::displayName() const {
    if (is<SPSymbol>(child)) {
        return _("Symbol");
    } else {
        return _("Clone");
    }
}

gchar* SPUse::description() const {
    if (child) {
        if (is<SPSymbol>(child)) {
            if (child->title()) {
                return g_strdup_printf(_("called %s"), Glib::Markup::escape_text(Glib::ustring( g_dpgettext2(nullptr, "Symbol", child->title()))).c_str());
            } else if (child->getAttribute("id")) {
                return g_strdup_printf(_("called %s"), Glib::Markup::escape_text(Glib::ustring( g_dpgettext2(nullptr, "Symbol", child->getAttribute("id")))).c_str());
            } else {
                return g_strdup_printf(_("called %s"), _("Unnamed Symbol"));
            }
        }

        static unsigned recursion_depth = 0;

        if (recursion_depth >= 4) {
            /* TRANSLATORS: Used for statusbar description for long <use> chains:
             * "Clone of: Clone of: ... in Layer 1". */
            return g_strdup(_("..."));
            /* We could do better, e.g. chasing the href chain until we reach something other than
             * a <use>, and giving its description. */
        }

        ++recursion_depth;
        char *child_desc = this->child->detailedDescription();
        --recursion_depth;

        char *ret = g_strdup_printf(_("of: %s"), child_desc);
        g_free(child_desc);

        return ret;
    } else {
        return g_strdup(_("[orphaned]"));
    }
}

Inkscape::DrawingItem* SPUse::show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) {

    // std::cout << "SPUse::show: " << (getId()?getId():"null") << std::endl;
    Inkscape::DrawingGroup *ai = new Inkscape::DrawingGroup(drawing);
    ai->setPickChildren(false);
    this->context_style = this->style;
    ai->setStyle(this->style, this->context_style);
    
    if (this->child) {
        Inkscape::DrawingItem *ac = this->child->invoke_show(drawing, key, flags);

        if (ac) {
            ai->prependChild(ac);
        }

        Geom::Translate t(this->x.computed, this->y.computed);
        ai->setChildTransform(t);
    }

    return ai;
}

void SPUse::hide(unsigned int key) {
    if (this->child) {
        this->child->invoke_hide(key);
    }

//  SPItem::onHide(key);
}


/**
 * Returns the ultimate original of a SPUse (i.e. the first object in the chain of its originals
 * which is not an SPUse). If no original is found, NULL is returned (it is the responsibility
 * of the caller to make sure that this is handled correctly).
 *
 * Note that the returned is the clone object, i.e. the child of an SPUse (of the argument one for
 * the trivial case) and not the "true original". If you want the true original, use trueOriginal().
 */
SPItem *SPUse::root() {
    SPItem *orig = this->child;

    auto use = cast<SPUse>(orig);
    while (orig && use) {
        orig = use->child;
        use = cast<SPUse>(orig);
    }

    return orig;
}

SPItem const *SPUse::root() const {
	return const_cast<SPUse*>(this)->root();
}

/**
 * Returns the ultimate original of a SPUse, i.e., the first object in the chain of uses
 * which is not itself an SPUse. If the chain of references is broken or no original is found,
 * the return value will be nullptr.
 */
SPItem *SPUse::trueOriginal() const
{
    int const depth = cloneDepth();
    if (depth < 0) {
        return nullptr;
    }

    SPItem *original_item = (SPItem *)this;
    for (int i = 0; i < depth; ++i) {
        if (auto const *intermediate_clone = cast<SPUse>(original_item)) {
            original_item = intermediate_clone->get_original();
        } else {
            return nullptr;
        }
    }
    return original_item;
}

/**
 * @brief Test the passed predicate on all items in a chain of uses.
 *
 * The chain includes this item, all of its intermediate ancestors in a chain of uses, as well as
 * the ultimate original item.
 *
 * @return Whether any of the items in the chain satisfies the predicate.
 */
bool SPUse::anyInChain(bool (*predicate)(SPItem const *)) const
{
    int const depth = cloneDepth();
    if (depth < 0) {
        return predicate(this);
    }

    SPItem const *item = this;
    if (predicate(item)) {
        return true;
    }

    for (int i = 0; i < depth; ++i) {
        if (auto const *intermediate_clone = cast<SPUse>(item)) {
            item = intermediate_clone->get_original();
            if (predicate(item)) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

/**
 * Get the number of dereferences or calls to get_original() needed to get an object
 * which is not an svg:use. Returns -1 if there is no original object.
 */
int SPUse::cloneDepth() const {
    unsigned depth = 1;
    SPItem *orig = this->child;

    while (orig && cast<SPUse>(orig)) {
        ++depth;
        orig = cast<SPUse>(orig)->child;
    }

    if (!orig) {
        return -1;
    } else {
        return depth;
    }
}

/**
 * Returns the effective transform that goes from the ultimate original to given SPUse, both ends
 * included.
 */
Geom::Affine SPUse::get_root_transform() const
{
    //track the ultimate source of a chain of uses
    SPObject *orig = this->child;

    std::vector<SPItem const *> chain;
    chain.push_back(this);

    while (cast<SPUse>(orig)) {
        chain.push_back(cast<SPItem>(orig));
        orig = cast<SPUse>(orig)->child;
    }

    chain.push_back(cast<SPItem>(orig));

    // calculate the accumulated transform, starting from the original
    Geom::Affine t(Geom::identity());

    for (auto i=chain.rbegin(); i!=chain.rend(); ++i) {
        auto *i_tem = *i;

        // "An additional transformation translate(x,y) is appended to the end (i.e.,
        // right-side) of the transform attribute on the generated 'g', where x and y
        // represent the values of the x and y attributes on the 'use' element." - http://www.w3.org/TR/SVG11/struct.html#UseElement
        auto *i_use = cast<SPUse>(i_tem);
        if (i_use && i_use->has_xy_offset()) {
            t = t * i_use->get_xy_offset();
        }

        t *= i_tem->transform;
    }
    return t;
}

/**
 * Returns the transform that leads to the use from its immediate original.
 * Does not include the original's transform if any.
 */
Geom::Affine SPUse::get_parent_transform() const
{
    Geom::Affine t(Geom::identity());

    if (has_xy_offset()) {
        t *= get_xy_offset();
    }

    t *= this->transform;
    return t;
}

bool SPUse::has_xy_offset() const {
    return (this->x._set && this->x.computed != 0) || (this->y._set && this->y.computed != 0);
}

Geom::Translate SPUse::get_xy_offset() const {
    return Geom::Translate(this->x._set ? this->x.computed : 0, this->y._set ? this->y.computed : 0);
}

/**
 * Sensing a movement of the original, this function attempts to compensate for it in such a way
 * that the clone stays unmoved or moves in parallel (depending on user setting) regardless of the
 * clone's transform.
 */
void SPUse::move_compensate(Geom::Affine const *mp) {
    // the clone is orphaned; or this is not a real use, but a clone of another use;
    // we skip it, otherwise duplicate compensation will occur
    if (this->cloned) {
        return;
    }

    // never compensate uses which are used in flowtext
    if (parent && cast<SPFlowregion>(parent)) {
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint mode = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_PARALLEL);
    // user wants no compensation
    if (mode == SP_CLONE_COMPENSATION_NONE)
        return;

    Geom::Affine m(*mp);
    Geom::Affine t = this->get_parent_transform();
    Geom::Affine clone_move = t.inverse() * m * t;

    // this is not a simple move, do not try to compensate
    if (!(m.isTranslation())){
    	//BUT move clippaths accordingly.
        //if clone has a clippath, move it accordingly
        if (getClipObject()) {
            for (auto &clip : getClipObject()->children) {
                SPItem *item = (SPItem*) &clip;
            	if(item){
                    item->transform *= m;
                    Geom::Affine identity;
                    item->doWriteTransform(item->transform, &identity);
            	}
            }
        }
        if (getMaskObject()) {
            for (auto &mask : getMaskObject()->children) {
                SPItem *item = (SPItem*) &mask;
            	if(item){
                    item->transform *= m;
                    Geom::Affine identity;
                    item->doWriteTransform(item->transform, &identity);
            	}
            }
        }
        return;
    }

    // restore item->transform field from the repr, in case it was changed by seltrans
    this->readAttr (SPAttr::TRANSFORM);


    // calculate the compensation matrix and the advertized movement matrix
    Geom::Affine advertized_move;
    if (mode == SP_CLONE_COMPENSATION_PARALLEL) {
        clone_move = clone_move.inverse() * m;
        advertized_move = m;
    } else if (mode == SP_CLONE_COMPENSATION_UNMOVED) {
        clone_move = clone_move.inverse();
        advertized_move.setIdentity();
    } else {
        g_assert_not_reached();
    }

    //if clone has a clippath, move it accordingly
    if (getClipObject()) {
        for (auto &clip : getClipObject()->children) {
        	SPItem *item = (SPItem*) &clip;
        	if(item){
                item->transform *= clone_move.inverse();
                Geom::Affine identity;
                item->doWriteTransform(item->transform, &identity);
        	}
        }
    }
    if (getMaskObject()) {
        for (auto &mask : getMaskObject()->children) {
            SPItem *item = (SPItem*) &mask;
        	if(item){
                item->transform *= clone_move.inverse();
                Geom::Affine identity;
                item->doWriteTransform(item->transform, &identity);
        	}
        }
    }


    // commit the compensation
    this->transform *= clone_move;
    this->doWriteTransform(this->transform, &advertized_move);
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPUse::href_changed() {
    this->_delete_connection.disconnect();
    this->_transformed_connection.disconnect();

    if (this->child) {
        this->detach(this->child);
        this->child = nullptr;
    }

    if (this->href) {
        SPItem *refobj = this->ref->getObject();

        if (refobj) {
            Inkscape::XML::Node *childrepr = refobj->getRepr();

            SPObject* obj = SPFactory::createObject(NodeTraits::get_type_string(*childrepr));

            auto item = cast<SPItem>(obj);
            if (item) {
                child = item;

                this->attach(this->child, this->lastChild());
                sp_object_unref(this->child, this);

                this->child->invoke_build(refobj->document, childrepr, TRUE);

                for (auto &v : views) {
                    auto ai = this->child->invoke_show(v.drawingitem->drawing(), v.key, v.flags);
                    if (ai) {
                        v.drawingitem->prependChild(ai);
                    }
                }

                this->_delete_connection = refobj->connectDelete(
                    sigc::hide(sigc::mem_fun(*this, &SPUse::delete_self))
                );

                this->_transformed_connection = refobj->connectTransformed(
                    sigc::hide(sigc::mem_fun(*this, &SPUse::move_compensate))
                );
            } else {
                delete obj;
            }
        }
    }
}

void SPUse::delete_self() {
    // always delete uses which are used in flowtext
    if (parent && cast<SPFlowregion>(parent)) {
        deleteObject();
        return;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    guint const mode = prefs->getInt("/options/cloneorphans/value",
                                               SP_CLONE_ORPHANS_UNLINK);

    if (mode == SP_CLONE_ORPHANS_UNLINK) {
        this->unlink();
    } else if (mode == SP_CLONE_ORPHANS_DELETE) {
        this->deleteObject();
    }
}

void SPUse::update(SPCtx *ctx, unsigned flags) {
    // std::cout << "SPUse::update: " << (getId()?getId():"null") << std::endl;
    SPItemCtx *ictx = (SPItemCtx *) ctx;
    SPItemCtx cctx = *ictx;

    unsigned childflags = flags;
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        childflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    childflags &= SP_OBJECT_MODIFIED_CASCADE;

    /* Set up child viewport */
    this->calcDimsFromParentViewport(ictx);

    childflags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B;

    if (this->child) {
        sp_object_ref(this->child);

        if (childflags || (this->child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            g_assert(child);
            cctx.i2doc = child->transform * ictx->i2doc;
            cctx.i2vp = child->transform * ictx->i2vp;
            child->updateDisplay(&cctx, childflags);
        }

        sp_object_unref(this->child);
    }

    SPItem::update(ctx, flags);

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (auto &v : views) {
            auto g = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
            context_style = style;
            g->setStyle(style, context_style);
        }
    }

    /* As last step set additional transform of arena group */
    for (auto &v : views) {
        auto g = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
        auto t = Geom::Translate(x.computed, y.computed);
        g->setChildTransform(t);
    }
}

void SPUse::modified(unsigned flags)
{
    // std::cout << "SPUse::modified: " << (getId()?getId():"null") << std::endl;
    flags = cascade_flags(flags);

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (auto &v : views) {
            auto g = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
            context_style = style;
            g->setStyle(style, context_style);
        }
    }

    if (child) {
        sp_object_ref(child);

        if (flags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child);
    }
}

SPItem *SPUse::unlink() {
    Inkscape::XML::Node *repr = this->getRepr();

    if (!repr) {
        return nullptr;
    }

    Inkscape::XML::Node *parent = repr->parent();
    SPDocument *document = this->document;
    Inkscape::XML::Document *xml_doc = document->getReprDoc();

    // Track the ultimate source of a chain of uses.
    SPItem *orig = this->root();
    SPItem *origtrue = this->trueOriginal();
    if (!orig) {
        return nullptr;
    }

    // Calculate the accumulated transform, starting from the original.
    Geom::Affine t = this->get_root_transform();

    Inkscape::XML::Node *copy = nullptr;

    if (auto symbol = cast<SPSymbol>(orig)) {
        // make a group, copy children
        copy = xml_doc->createElement("svg:g");
        copy->setAttribute("display","none");

        for (Inkscape::XML::Node *child = orig->getRepr()->firstChild() ; child != nullptr; child = child->next()) {
            Inkscape::XML::Node *newchild = child->duplicate(xml_doc);
            copy->appendChild(newchild);
        }

        // viewBox transformation
        t = symbol->c2p * t;
    } else { // just copy
        copy = orig->getRepr()->duplicate(xml_doc);
        copy->setAttribute("display","none");
    }
    // Add the duplicate repr just after the existing one.
    parent->addChild(copy, repr);

    // Retrieve the SPItem of the resulting repr.
    SPObject *unlinked = document->getObjectByRepr(copy);
    if (origtrue) {
        if (unlinked) {
            origtrue->setTmpSuccessor(unlinked);
        }
        auto newLPEObj = cast<SPLPEItem>(unlinked);
        if (newLPEObj) {
            // force always fork
            newLPEObj->forkPathEffectsIfNecessary(1, true, true);
        }
        origtrue->fixTmpSuccessors();
        origtrue->unsetTmpSuccessor();
    }
    
    // Merge style from the use.
    unlinked->style->merge( this->style );
    unlinked->style->cascade( unlinked->parent->style );
    unlinked->updateRepr();
    unlinked->removeAttribute("display");
    
    // Hold onto our SPObject and repr for now.
    sp_object_ref(this);
    Inkscape::GC::anchor(repr);

    // Remove ourselves, not propagating delete events to avoid a
    // chain-reaction with other elements that might reference us.
    this->deleteObject(false);

    // Give the copy our old id and let go of our old repr.
    copy->setAttribute("id", repr->attribute("id"));
    Inkscape::GC::release(repr);

    // Remove tiled clone attrs.
    copy->removeAttribute("inkscape:tiled-clone-of");
    copy->removeAttribute("inkscape:tile-w");
    copy->removeAttribute("inkscape:tile-h");
    copy->removeAttribute("inkscape:tile-cx");
    copy->removeAttribute("inkscape:tile-cy");

    // Establish the succession and let go of our object.
    this->setSuccessor(unlinked);
    sp_object_unref(this);

    auto item = cast<SPItem>(unlinked);
    g_assert(item != nullptr);

    // Set the accummulated transform.
    {
        Geom::Affine nomove(Geom::identity());
        // Advertise ourselves as not moving.
        item->doWriteTransform(t, &nomove);
    }

    return item;
}

SPItem *SPUse::get_original() const
{
    SPItem *ref = nullptr;

    if (this->ref){
        ref = this->ref->getObject();
    }

    return ref;
}

void SPUse::snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const {
    SPItem const *child = this->child;

    if (!child) {
        return;
    }

    // Collect the candidate snap points from the original item (i.e. from the child)
    std::vector<Inkscape::SnapCandidatePoint> vec_pts;
    child->snappoints(vec_pts, snapprefs);

    // Offset these snap candidate points if the X/Y attributes have been set for this item
    // (see https://gitlab.com/inkscape/inkscape/-/issues/2765)
    if (has_xy_offset()) {
        // All snappoints are in desktop coordinates, but the item's transformation is in document coordinates
        Geom::Point offset_dt = get_xy_offset().vector() * i2dt_affine().withoutTranslation();
        for (auto& it: vec_pts) {
            it.movePoint(offset_dt);
        }
    }

    // Return the offsetted snap candidate points in vector p
    std::move(vec_pts.begin(), vec_pts.end(), std::back_inserter(p));
}

void SPUse::getLinked(std::vector<SPObject *> &objects, LinkedObjectNature direction) const
{
    if (direction == LinkedObjectNature::ANY || direction == LinkedObjectNature::DEPENDENCY) {
        if (auto linked = (ref ? ref->getObject() : nullptr)) {
            objects.emplace_back(linked);
        }
    }
    SPObject::getLinked(objects, direction);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
