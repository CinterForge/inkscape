// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Johan Engelen 2007-2008 <j.b.c.engelen@utwente.nl>
 *   Abhishek Sharma
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpeobject.h"

#include "attributes.h"
#include "document.h"
#include "live_effects/effect.h"
#include "object/sp-defs.h"

//#define LIVEPATHEFFECT_VERBOSE

LivePathEffectObject::LivePathEffectObject()
{
#ifdef LIVEPATHEFFECT_VERBOSE
    g_message("Init livepatheffectobject");
#endif
}

LivePathEffectObject::~LivePathEffectObject() = default;

/**
 * Virtual build: set livepatheffect attributes from its associated XML node.
 */
void LivePathEffectObject::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPObject::build(document, repr);

    this->readAttr(SPAttr::PATH_EFFECT);

    if (repr) {
        repr->addObserver(nodeObserver());
    }
    setOnClipboard();
    /* Register ourselves, is this necessary? */
    // document->addResource("path-effect", object);
}

/**
 * Virtual release of livepatheffect members before destruction.
 */
void LivePathEffectObject::release()
{
    getRepr()->removeObserver(nodeObserver());
    if (this->lpe) {
        delete this->lpe;
        this->lpe = nullptr;
    }

    this->effecttype = Inkscape::LivePathEffect::INVALID_LPE;

    SPObject::release();
}

/**
 * Virtual set: set attribute to value.
 */
void LivePathEffectObject::set(SPAttr key, gchar const *value) {
#ifdef LIVEPATHEFFECT_VERBOSE
    g_print("Set livepatheffect");
#endif

    switch (key) {
        case SPAttr::PATH_EFFECT:
            if (this->lpe) {
                delete this->lpe;
                this->lpe = nullptr;
            }
            if ( value && Inkscape::LivePathEffect::LPETypeConverter.is_valid_key(value) ) {
                this->effecttype = Inkscape::LivePathEffect::LPETypeConverter.get_id_from_key(value);
                this->lpe = Inkscape::LivePathEffect::Effect::New(this->effecttype, this);
                this->effecttype_set = true;
                this->deleted = false;
            } else {
                this->effecttype = Inkscape::LivePathEffect::INVALID_LPE;
                this->lpe = nullptr;
                this->effecttype_set = false;
            }

            this->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
    }

    SPObject::set(key, value);
}

/**
 * Virtual write: write object attributes to repr.
 */
Inkscape::XML::Node* LivePathEffectObject::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("inkscape:path-effect");
    }

    if ((flags & SP_OBJECT_WRITE_ALL) || this->lpe) {
        repr->setAttributeOrRemoveIfEmpty("effect", Inkscape::LivePathEffect::LPETypeConverter.get_key(this->effecttype));

        this->lpe->writeParamsToSVG();
    }

    SPObject::write(xml_doc, repr, flags);

    return repr;
}

// Caution using this function, just compare id and same type of
// effect, we use on clipboard to do not fork in same doc on pastepatheffect
bool LivePathEffectObject::is_similar(LivePathEffectObject *that)
{
    if (that) {
        const char *thisid = this->getId();
        const char *thatid = that->getId();
        if (!thisid || !thatid || strcmp(thisid, thatid) != 0) {
            return false;
        }
        Inkscape::LivePathEffect::Effect *thislpe = this->get_lpe();
        Inkscape::LivePathEffect::Effect *thatlpe = that->get_lpe();
        if (thatlpe && thislpe && thislpe->getName() != thatlpe->getName()) {
            return false;
        }
    }
    return true;
}

/**
 * Set lpeobject is on clipboard
 */
void LivePathEffectObject::setOnClipboard()
{
    // when no document we are intermedite state between clipboard
    if (!document) { 
        _isOnClipboard = true;
        return;
    }
    
    Inkscape::XML::Node *root = document->getReprRoot();
    Inkscape::XML::Node *clipnode = sp_repr_lookup_name(root, "inkscape:clipboard", 1);
    _isOnClipboard = clipnode != nullptr;
}

/**
 * If this has other users, create a new private duplicate and return it
 * returns 'this' when no forking was necessary (and therefore no duplicate was made)
 * Check out SPLPEItem::forkPathEffectsIfNecessary !
 */
LivePathEffectObject *LivePathEffectObject::fork_private_if_necessary(unsigned int nr_of_allowed_users)
{
    if (hrefcount > nr_of_allowed_users) {
        SPDocument *doc = this->document;
        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *dup_repr = this->getRepr()->duplicate(xml_doc);

        doc->getDefs()->getRepr()->addChild(dup_repr, nullptr);
        auto lpeobj_new = cast<LivePathEffectObject>(doc->getObjectByRepr(dup_repr));
        Inkscape::GC::release(dup_repr);
        // To regenerate ID
        sp_object_ref(lpeobj_new, nullptr);
        auto id = generate_unique_id();
        lpeobj_new->setAttribute("id", id);
        // Load all volatile vars of forked item
        sp_object_unref(lpeobj_new, nullptr);
        return lpeobj_new;
    }
    return this;
}

void LPENodeObserver::notifyAttributeChanged(Inkscape::XML::Node &, GQuark key_, Inkscape::Util::ptr_shared,
                                             Inkscape::Util::ptr_shared newval)
{
#ifdef LIVEPATHEFFECT_VERBOSE
    g_print("LPENodeObserver::notifyAttributeChanged()\n");
#endif
    auto lpeobj = static_cast<LivePathEffectObject *>(this);
    auto const key = g_quark_to_string(key_);
    if (!lpeobj->get_lpe()) {
        return;
    }
    lpeobj->get_lpe()->setParameter(key, newval);
    lpeobj->requestModified(SP_OBJECT_MODIFIED_FLAG);
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
