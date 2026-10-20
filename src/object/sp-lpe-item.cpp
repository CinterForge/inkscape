// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Base class for live path effect items
 */
/*
 * Authors:
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Bastien Bouclet <bgkweb@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <utility>
#include <glibmm/i18n.h>
#include <sigc++/adaptors/bind.h>

#include "bad-uri-exception.h"
#include "attributes.h"
#include "desktop.h"
#include "inkscape.h"
#include "live_effects/effect.h"
#include "live_effects/lpe-lattice2.h"
#include "live_effects/lpe-measure-segments.h"
#include "live_effects/lpeobject-reference.h"
#include "message-stack.h"
#include "preferences.h"
#include "sp-clippath.h"
#include "sp-ellipse.h"
#include "sp-spiral.h"
#include "sp-star.h"
#include "sp-item-group.h"
#include "sp-mask.h"
#include "sp-path.h"
#include "sp-root.h"
#include "sp-symbol.h"
#include "svg/svg.h"

/* LPEItem base class */

static void lpeobject_ref_modified(SPObject *href, guint flags, SPLPEItem *lpeitem);
static void sp_lpe_item_create_original_path_recursive(SPLPEItem *lpeitem);
static SPLPEItem * sp_lpe_item_cleanup_original_path_recursive(SPLPEItem *lpeitem, bool keep_paths, bool force = false, bool is_clip_mask = false, bool is_flatten = false);

typedef std::list<std::string> HRefList;
static std::string patheffectlist_svg_string(PathEffectList const & list);
static std::string hreflist_svg_string(HRefList const & list);

namespace {

void clear_path_effect_list(PathEffectList* const l) {
    auto it = l->begin();
    while ( it !=  l->end()) {
        (*it)->unlink();
        it = l->erase(it);
    }
}

} // unnamed namespace

SPLPEItem::SPLPEItem()
    : SPItem()
    , path_effects_enabled(1)
    , path_effect_list(new PathEffectList())
    , current_path_effect(nullptr)
    , lpe_helperpaths()
{
}

SPLPEItem::~SPLPEItem() = default;

void SPLPEItem::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::INKSCAPE_PATH_EFFECT);
    onsymbol = isOnSymbol();
    SPItem::build(document, repr);
}

void SPLPEItem::release() {
    // disconnect all modified listeners:
    lpe_modified_connection_list.clear();

    clear_path_effect_list(this->path_effect_list);

    // delete the list itself
    delete this->path_effect_list;
    this->path_effect_list = nullptr;

    SPItem::release();
}

void SPLPEItem::set(SPAttr key, gchar const* value) {
    switch (key) {
        case SPAttr::INKSCAPE_PATH_EFFECT:
            this->current_path_effect = nullptr;

            // Disable the path effects while populating the LPE list
            sp_lpe_item_enable_path_effects(this, false);

            // disconnect all modified listeners:
            lpe_modified_connection_list.clear();

            clear_path_effect_list(this->path_effect_list);

            // Parse the contents of "value" to rebuild the path effect reference list
            if ( value ) {
                std::istringstream iss(value);
                std::string href;

                while (std::getline(iss, href, ';'))
                {
                    auto path_effect_ref = std::make_shared<Inkscape::LivePathEffect::LPEObjectReference>(this);

                    try {
                        path_effect_ref->link(href.c_str());
                    } catch (Inkscape::BadURIException const &e) {
                        g_warning("BadURIException when trying to find LPE: %s", e.what());
                        path_effect_ref->unlink();
                        continue;
                    }

                    if ( path_effect_ref->lpeobject && path_effect_ref->lpeobject->get_lpe() ) {
                        // connect modified-listener
                        lpe_modified_connection_list.emplace_back(
                            path_effect_ref->lpeobject->connectModified(sigc::bind(&lpeobject_ref_modified, this)));
                    } else {
                        // on clipboard we fix refs so in middle time of the operation, in LPE with multiples path
                        // effects can result middle updata and fire a warning, so we silent it
                        if (!isOnClipboard()) {
                            // something has gone wrong in finding the right patheffect.
                            g_warning("Unknown LPE type specified, LPE stack effectively disabled");
                            // keep the effect in the lpestack, so the whole stack is effectively disabled but
                            // maintained
                        }
                    }

                    this->path_effect_list->push_back(std::move(path_effect_ref));
                }
            }

            sp_lpe_item_enable_path_effects(this, true);
            break;

        default:
            SPItem::set(key, value);
            break;
    }
}

void SPLPEItem::update(SPCtx* ctx, unsigned int flags) {
    SPItem::update(ctx, flags);

    // update the helperpaths of all LPEs applied to the item
    // TODO: re-add for the new node tool
}

void SPLPEItem::modified(unsigned int flags) {
    //stop update when modified and make the effect update on the LPE transform method if the effect require it
    //if (is<SPGroup>(this) && (flags & SP_OBJECT_MODIFIED_FLAG) && (flags & SP_OBJECT_USER_MODIFIED_FLAG_B)) {
    //  sp_lpe_item_update_patheffect(this, true, false);
    //}
}

Inkscape::XML::Node* SPLPEItem::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if (flags & SP_OBJECT_WRITE_EXT) {
        if ( hasPathEffect() ) {
            repr->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(*this->path_effect_list));
        } else {
            repr->removeAttribute("inkscape:path-effect");
        }
    }

    SPItem::write(xml_doc, repr, flags);

    return repr;
}

/**
 * The lpeitem is on clipboard
 */
bool SPLPEItem::isOnClipboard()
{
    Inkscape::XML::Node *root = document->getReprRoot();
    Inkscape::XML::Node *clipnode = sp_repr_lookup_name(root, "inkscape:clipboard", 1);
    return clipnode != nullptr;
}

bool SPLPEItem::isOnSymbol() const {
    auto p = cast<SPLPEItem>(parent);
    return (p && p->onsymbol) || is<SPSymbol>(this);
}
/**
 * returns true when LPE was successful.
 */
bool SPLPEItem::performPathEffect(Geom::PathVector &curve, SPShape *current, bool is_clip_or_mask)
{
    if (!Inkscape::LivePathEffect::can_have_lpe(current)) {
        // Reject things that have curves put can't have path
        // effects: polygons, paths with offsets, etc.
        return false;
    }

    if (this->hasPathEffect() && this->pathEffectsEnabled()) {
        PathEffectList path_effect_list(*this->path_effect_list);
        auto const path_effect_list_size = path_effect_list.size();
        for (auto &lperef : path_effect_list) {
            LivePathEffectObject *lpeobj = lperef->lpeobject;
            if (!lpeobj) {
                /** \todo Investigate the cause of this.
                 * For example, this happens when copy pasting an object with LPE applied. Probably because the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                */
                g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                return false;
            }

            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (!lpe || !performOnePathEffect(curve, current, lpe, is_clip_or_mask)) {
                return false;
            }
            auto hreflist = lpeobj->hrefList;
            if (hreflist.size()) { // lpe can be removed on perform (eg: clone lpe on copy)
                if (path_effect_list_size != this->path_effect_list->size()) {
                    break;
                }
            }
        }
    }
    return true;
}

/**
 * returns true when LPE was successful.
 */
bool SPLPEItem::performOnePathEffect(Geom::PathVector &curve, SPShape *current, Inkscape::LivePathEffect::Effect *lpe, bool is_clip_or_mask) {
    if (!lpe) {
        /** \todo Investigate the cause of this.
         * Not sure, but I think this can happen when an unknown effect type is specified...
         */
        g_warning("SPLPEItem::performPathEffect - lpeobj with invalid lpe in the stack!");
        return false;
    }
    if (document->isSeeking()) {
        lpe->refresh_widgets = true;
    }
    if (lpe->isVisible()) {
        if (lpe->acceptsNumClicks() > 0 && !lpe->isReady()) {
            // if the effect expects mouse input before being applied and the input is not finished
            // yet, we don't alter the path
            return false;
        }
        //if is not clip or mask or LPE apply to clip and mask
        if (!is_clip_or_mask || lpe->apply_to_clippath_and_mask) {
            // Uncomment to get updates
            // g_debug("LPE running:: %s",Inkscape::LivePathEffect::LPETypeConverter.get_key(lpe->effectType()).c_str());
            lpe->setCurrentShape(current);
            if (!is<SPGroup>(this)) {
                lpe->pathvector_before_effect = curve;
            }
            // To Calculate BBox on shapes and nested LPE
            current->setCurveInsync(curve);
            // Groups have their doBeforeEffect called elsewhere
            if (lpe->lpeversion.param_getSVGValue() != "0") { // we are on 1 or up
                current->bbox_vis_cache_is_valid = false;
                current->bbox_geom_cache_is_valid = false;
            }
            auto group = cast<SPGroup>(this);
            if (!group && !is_clip_or_mask) {
                lpe->doBeforeEffect_impl(this);
            }

            try {
                lpe->doEffect(curve);
                lpe->has_exception = false;
            }

            catch (std::exception & e) {
                g_warning("Exception during LPE %s execution. \n %s", lpe->getName().c_str(), e.what());
                if (SP_ACTIVE_DESKTOP && SP_ACTIVE_DESKTOP->messageStack()) {
                    SP_ACTIVE_DESKTOP->messageStack()->flash( Inkscape::WARNING_MESSAGE,
                                    _("An exception occurred during execution of the Path Effect.") );
                }
                lpe->doOnException(this);
                return false;
            }

            if (!group) {
                // To have processed the shape to doAfterEffect
                current->setCurveInsync(curve);
                lpe->pathvector_after_effect = curve;
                lpe->doAfterEffect_impl(this, &curve);
            }
        }
    }
    return true;
}

/**
 * returns false when LPE write unoptimiced
 */
bool SPLPEItem::optimizeTransforms()
{
    if (is<SPGroup>(this)) {
        return false;
    }

    if (is<SPSpiral>(this) && !this->transform.isUniformScale()) {
        return false;
    }
    if (is<SPStar>(this) && !this->transform.isUniformScale()) {
        return false;
    }
    auto* mask_path = this->getMaskObject();
    if(mask_path) {
        return false;
    }
    auto* clip_path = this->getClipObject();
    if(clip_path) {
        return false;
    }
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        if (!lperef) {
            continue;
        }
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (lpe) {
                if (dynamic_cast<Inkscape::LivePathEffect::LPEMeasureSegments*>(lpe) ||
                    dynamic_cast<Inkscape::LivePathEffect::LPELattice2*>(lpe))
                {
                    return false;
                }
            }
        }
    }

    if (unoptimized()) {
        return false;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    return !prefs->getBool("/options/preservetransform/value", false);
}

/**
 * notify tranbsform applied to a LPE
 */
void SPLPEItem::notifyTransform(Geom::Affine const &postmul)
{
    if (!pathEffectsEnabled())
        return;

    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        if (!lperef) {
            continue;
        }
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (lpe && !lpe->is_load) {
                lpe->transform_multiply_impl(postmul, this);
            }
        }
    }
}

// CPPIFY: make pure virtual
void SPLPEItem::update_patheffect(bool /*write*/) {
    //throw;
}

/**
 * Calls any registered handlers for the update_patheffect action
 */
void
sp_lpe_item_update_patheffect (SPLPEItem *lpeitem, bool wholetree, bool write, bool with_satellites)
{
#ifdef SHAPE_VERBOSE
    g_message("sp_lpe_item_update_patheffect: %p\n", lpeitem);
#endif
    g_return_if_fail (lpeitem != nullptr);

    // Do not check for LPE item to allow LPE work on clips/mask
    if (!lpeitem->pathEffectsEnabled())
        return;

    SPLPEItem *top = nullptr;

    if (wholetree) {
        SPLPEItem *prev_parent = lpeitem;
        auto parent = cast<SPLPEItem>(prev_parent->parent);
        while (parent && parent->hasPathEffectRecursive()) {
            prev_parent = parent;
            parent = cast<SPLPEItem>(prev_parent->parent);
        }
        top = prev_parent;
    }
    else {
        top = lpeitem;
    }
    top->update_patheffect(write);
    if (with_satellites) {
        lpeitem->update_satellites();
    }
}

/**
 * Gets called when any of the lpestack's lpeobject repr contents change: i.e. parameter change in any of the stacked LPEs
 */
static void
lpeobject_ref_modified(SPObject */*href*/, guint flags, SPLPEItem *lpeitem)
{
#ifdef SHAPE_VERBOSE
    g_message("lpeobject_ref_modified");
#endif
    if (!lpeitem->document->isSeeking() && flags != 29 && flags != 253 && !(flags & SP_OBJECT_STYLESHEET_MODIFIED_FLAG))
    {
        sp_lpe_item_update_patheffect(lpeitem, false, true, true);
    }
}

static void
sp_lpe_item_create_original_path_recursive(SPLPEItem *lpeitem)
{
    g_return_if_fail(lpeitem != nullptr);

    SPClipPath *clip_path = lpeitem->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto iter : clip_path_list) {
            auto clip_data = cast<SPLPEItem>(iter);
            sp_lpe_item_create_original_path_recursive(clip_data);
            sp_object_unref(iter);
        }
    }

    SPMask *mask_path = lpeitem->getMaskObject();
    if(mask_path) {
        std::vector<SPObject*> mask_path_list = mask_path->childList(true);
        for (auto iter : mask_path_list) {
            auto mask_data = cast<SPLPEItem>(iter);
            sp_lpe_item_create_original_path_recursive(mask_data);
            sp_object_unref(iter);
        }
    }
    if (is<SPGroup>(lpeitem)) {
        std::vector<SPItem*> item_list = cast<SPGroup>(lpeitem)->item_list();
        for (auto subitem : item_list) {
            if (is<SPLPEItem>(subitem)) {
                sp_lpe_item_create_original_path_recursive(cast<SPLPEItem>(subitem));
            }
        }
    } else if (auto path = cast<SPPath>(lpeitem)) {
        if (!path->getAttribute("inkscape:original-d") ) {
            if (gchar const * value = path->getAttribute("d")) {
                path->setAttribute("inkscape:original-d", value);
            }
        }
    } else if (auto shape = cast<SPShape>(lpeitem)) {
        if (!shape->curveBeforeLPE()) {
            shape->setCurveBeforeLPE(shape->curve());
        }
    }
}

static SPLPEItem *
sp_lpe_item_cleanup_original_path_recursive(SPLPEItem *lpeitem, bool keep_paths, bool force, bool is_clip_mask, bool is_flatten)
{
    if (!lpeitem) {
        return nullptr;
    }
    auto group = cast<SPGroup>(lpeitem);
    auto shape = cast<SPShape>(lpeitem);
    auto path = cast<SPPath>(lpeitem);
    SPClipPath *clip_path = lpeitem->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto iter : clip_path_list) {
            auto clip_data = cast<SPLPEItem>(iter);
            if (clip_data) {
                sp_lpe_item_cleanup_original_path_recursive(clip_data, keep_paths, lpeitem && !lpeitem->hasPathEffectRecursive(), true);
            }
            sp_object_unref(iter);
        }
    }

    SPMask *mask_path = lpeitem->getMaskObject();
    if(mask_path) {
        std::vector<SPObject*> mask_path_list = mask_path->childList(true);
        for (auto iter : mask_path_list) {
            auto mask_data = cast<SPLPEItem>(iter);
            if (mask_data) {
                sp_lpe_item_cleanup_original_path_recursive(mask_data, keep_paths, lpeitem && !lpeitem->hasPathEffectRecursive(), true);
            }
            sp_object_unref(iter);
        }
    }

    if (group) {
        std::vector<SPItem*> item_list = cast<SPGroup>(lpeitem)->item_list();
        for (auto iter : item_list) {
            if (auto subitem = cast<SPLPEItem>(iter)) {
                if (auto shape = cast<SPShape>(iter)) {
                    if (gchar const * value = shape->getAttribute("d")) {
                        shape->setCurve(sp_svg_read_pathv(value));
                    }
                }
                sp_lpe_item_cleanup_original_path_recursive(subitem, keep_paths, false, false, is_flatten);
            }
        }
    } else if (path) {
        Inkscape::XML::Node *repr = lpeitem->getRepr();
        if (repr->attribute("inkscape:original-d") &&
            !lpeitem->hasPathEffectRecursive() &&
            (!is_clip_mask ||
            ( is_clip_mask && force)))
        {
            if (!keep_paths) {
                repr->setAttribute("d", repr->attribute("inkscape:original-d"));
            }
            repr->removeAttribute("inkscape:original-d");
            path->setCurveBeforeLPE(nullptr);
            if (!shape->curve()->curveCount()) {
                repr->parent()->removeChild(repr); // will delete lpeitem
                return nullptr;
            }
        } else {
            if (!keep_paths) {
                // we dont want to update the whole tree when we flatten
                sp_lpe_item_update_patheffect(lpeitem, !is_flatten, true);
            }
        }
    } else if (shape) {
        Inkscape::XML::Node *repr = lpeitem->getRepr();
        auto const *c_lpe = shape->curve();
        Glib::ustring d_str;
        if (c_lpe) {
            d_str = sp_svg_write_path(*c_lpe);
        } else if (shape->getAttribute("d")) {
            d_str = shape->getAttribute("d");
        } else {
            return lpeitem;
        }
        if (!lpeitem->hasPathEffectRecursive() &&
            (!is_clip_mask ||
            ( is_clip_mask && force)))
        {
            if (!keep_paths) {
                repr->removeAttribute("d");
                shape->setCurveBeforeLPE(nullptr);
            } else {
                const char * id = repr->attribute("id");
                const char * style = repr->attribute("style");
                // remember the position of the item
                gint pos = shape->getRepr()->position();
                // remember parent
                Inkscape::XML::Node *parent = shape->getRepr()->parent();
                // remember class
                char const *class_attr = shape->getRepr()->attribute("class");
                // remember title
                gchar *title = shape->title();
                // remember description
                gchar *desc = shape->desc();
                // remember transformation
                gchar const *transform_str = shape->getRepr()->attribute("transform");
                // Mask
                gchar const *mask_str = (gchar *) shape->getRepr()->attribute("mask");
                // Clip path
                gchar const *clip_str = (gchar *) shape->getRepr()->attribute("clip-path");

                /* Rotation center */
                gchar const *transform_center_x = shape->getRepr()->attribute("inkscape:transform-center-x");
                gchar const *transform_center_y = shape->getRepr()->attribute("inkscape:transform-center-y");

                // It's going to resurrect, so we delete without notifying listeners.
                SPDocument * doc = shape->document;
                shape->deleteObject(false);
                Inkscape::XML::Document *xml_doc = doc->getReprDoc();
                Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
                // restore id
                repr->setAttribute("id", id);
                // restore class
                repr->setAttribute("class", class_attr);
                // restore transform
                repr->setAttribute("transform", transform_str);
                // restore clip
                repr->setAttribute("clip-path", clip_str);
                // restore mask
                repr->setAttribute("mask", mask_str);
                // restore transform_center_x
                repr->setAttribute("inkscape:transform-center-x", transform_center_x);
                // restore transform_center_y
                repr->setAttribute("inkscape:transform-center-y", transform_center_y);
                //restore d
                repr->setAttribute("d", d_str);
                //restore style
                repr->setAttribute("style", style);
                // add the new repr to the parent
                parent->appendChild(repr);
                SPObject* newObj = doc->getObjectByRepr(repr);
                if (title && newObj) {
                    newObj->setTitle(title);
                    g_free(title);
                }
                if (desc && newObj) {
                    newObj->setDesc(desc);
                    g_free(desc);
                }
                // move to the saved position
                repr->setPosition(pos > 0 ? pos : 0);
                Inkscape::GC::release(repr);
                lpeitem = cast<SPLPEItem>(newObj);
            }
        } else {
            if (!keep_paths) {
                // we dont want to update the whole tree when we flatten
                sp_lpe_item_update_patheffect(lpeitem, !is_flatten, true);
            }
        }
    }
    if (lpeitem->getRepr() && !lpeitem->getAttribute("inkscape:path-effect") && lpeitem->path_effect_list) {
        clear_path_effect_list(lpeitem->path_effect_list);
    }
    return lpeitem;
}



void SPLPEItem::addPathEffect(std::string value, bool reset)
{
    if (!value.empty()) {
        // Apply the path effects here because in the casse of a group, lpe->resetDefaults
        // needs that all the subitems have their effects applied
        auto group = cast<SPGroup>(this); 
        if (group) {
            sp_lpe_item_update_patheffect(this, false, true);
        }
        // Disable the path effects while preparing the new lpe
        sp_lpe_item_enable_path_effects(this, false);

        // Add the new reference to the list of LPE references
        HRefList hreflist;
        for (auto const &it : *this->path_effect_list) {
            hreflist.emplace_back(it->lpeobject_href);
        }
        hreflist.push_back(std::move(value));

        this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist));
        // Make sure that ellipse is stored as <svg:path>
        if( is<SPGenericEllipse>(this)) {
            cast<SPGenericEllipse>(this)->write( this->getRepr()->document(), this->getRepr(), SP_OBJECT_WRITE_EXT );
        }
        // make sure there is an original-d for paths!!!
        sp_lpe_item_create_original_path_recursive(this);

        LivePathEffectObject *lpeobj = this->path_effect_list->back()->lpeobject;
        if (lpeobj && lpeobj->get_lpe()) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            // Ask the path effect to reset itself if it doesn't have parameters yet
            if (reset) {
                // has to be called when all the subitems have their lpes applied
                lpe->resetDefaults(this);
            }
            // perform this once when the effect is applied
            lpe->doOnApply_impl(this);
        }

        //Enable the path effects now that everything is ready to apply the new path effect
        sp_lpe_item_enable_path_effects(this, true);

        // Apply the path effect
        sp_lpe_item_update_patheffect(this, true, true);
    }
}

void SPLPEItem::addPathEffect(LivePathEffectObject * new_lpeobj)
{
    const gchar * repr_id = new_lpeobj->getRepr()->attribute("id");
    gchar *hrefstr = g_strdup_printf("#%s", repr_id);
    this->addPathEffect(hrefstr, false);
    g_free(hrefstr);
}

/**
 *  If keep_path is true, the item should not be updated, effectively 'flattening' the LPE.
 */
SPLPEItem * SPLPEItem::removeCurrentPathEffect(bool keep_paths)
{
    auto const lperef = this->getCurrentLPEReference();
    if (!lperef) {
        return nullptr;
    }
    if (Inkscape::LivePathEffect::Effect* effect_ = this->getCurrentLPE()) {
        effect_->keep_paths = keep_paths;
        effect_->on_remove_all = false; 
        if (effect_->getHolderRemove()) {
            this->deleteObject(true);
            return nullptr;
        }
        effect_->doOnRemove_impl(this);
    }
    if (!this->path_effect_list) {
        // We got deleted ourselves during the removal of the LPE
        return nullptr;
    }
    this->path_effect_list->remove(lperef); //current lpe ref is always our 'own' pointer from the path_effect_list
    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(*this->path_effect_list));
    if (!keep_paths) {
        // Make sure that ellipse is stored as <svg:circle> or <svg:ellipse> if possible.
        if (auto ell = cast<SPGenericEllipse>(this)) {
            ell->write(getRepr()->document(), getRepr(), SP_OBJECT_WRITE_EXT);
        }
    }
    return sp_lpe_item_cleanup_original_path_recursive(this, keep_paths);
}

/**
 *  If keep_path is true, the item should not be updated, effectively 'flattening' the LPE.
 */
SPLPEItem * SPLPEItem::removeAllPathEffects(bool keep_paths, bool recursive)
{
    if (recursive) {
        auto grp = cast<SPGroup>(this);
        if (grp) {
            std::vector<SPItem *> item_list = grp->item_list();
            for (auto iter : item_list) {
                sp_object_ref(iter);
            }
            for (auto iter : item_list) {
                auto subitem = cast<SPLPEItem>(iter);
                if (subitem && subitem->document) {
                    subitem->removeAllPathEffects(keep_paths, recursive);
                }
            }
            for (auto iter : item_list) {
                sp_object_unref(iter);
            }
        }
    }
    if (!hasPathEffect()) {
        return nullptr;
    }
    if (keep_paths) {
        if (path_effect_list->empty()) {
            return nullptr;
        }
    }
    PathEffectList a_path_effect_list(*path_effect_list);
    for (auto &lperef : a_path_effect_list) {
        if (!lperef) {
            continue;
        }
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect * lpe = lpeobj->get_lpe();
            if (lpe) {
                lpe->keep_paths = keep_paths;
                lpe->on_remove_all = true;
                if (lpe->getHolderRemove()) {
                    this->deleteObject(true);
                    return nullptr;
                }
                lpe->doOnRemove_impl(this);
            }
        }
        if (!path_effect_list) {
            // We got deleted ourselves during the removal of the LPE
            return nullptr;
        }
        // this allow to keep references and propely delete satellites
        path_effect_list->remove(lperef);
    }
    this->removeAttribute("inkscape:path-effect");
    if (!keep_paths) {
        // Make sure that ellipse is stored as <svg:circle> or <svg:ellipse> if possible.
        if (auto ell = cast<SPGenericEllipse>(this)) {
            ell->write(getRepr()->document(), getRepr(), SP_OBJECT_WRITE_EXT);
        }
    }
    // SPItem can be changed on remove all LPE items (Shape to Path) We return generated item
    return sp_lpe_item_cleanup_original_path_recursive(this, keep_paths);
}

void SPLPEItem::downCurrentPathEffect()
{
    auto const lperef = getCurrentLPEReference();
    if (!lperef)
        return;

    PathEffectList new_list = *this->path_effect_list;
    auto const cur_it = std::find(new_list.begin(), new_list.end(), lperef);
    if (cur_it != new_list.cend()) {
        auto down_it = cur_it;
        ++down_it;
        if (down_it != new_list.end()) { // perhaps current effect is already last effect
            std::iter_swap(cur_it, down_it);
        }
    }

    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(new_list));

    sp_lpe_item_cleanup_original_path_recursive(this, false);
}

void SPLPEItem::duplicateCurrentPathEffect()
{
    auto const lperef = getCurrentLPEReference();
    if (!lperef)
        return;

    HRefList hreflist;
    auto const cur_it = std::find(this->path_effect_list->cbegin(), this->path_effect_list->cend(), lperef);
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto it = this->path_effect_list->cbegin(); it != this->path_effect_list->cend(); ++it) {
        hreflist.emplace_back((*it)->lpeobject_href);
        LivePathEffectObject *lpeobj = (*it)->lpeobject;
        if (it == cur_it) {
            auto *duple = lpeobj->fork_private_if_necessary(0);
            hreflist.push_back(std::string{"#"} += duple->getId());
        }
    }
    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist));

    sp_lpe_item_cleanup_original_path_recursive(this, false);
    update_satellites(true);
}

SPLPEItem *SPLPEItem::flattenCurrentPathEffect()
{
    auto const lperef = getCurrentLPEReference();
    if (!lperef)
        return nullptr;

    HRefList hreflist;
    HRefList hreflist2;
    auto const cur_it = std::find(this->path_effect_list->cbegin(), this->path_effect_list->cend(), lperef);
    PathEffectList path_effect_list(*this->path_effect_list);
    bool done = false;
    for (auto it = this->path_effect_list->cbegin(); it != this->path_effect_list->cend(); ++it) {
        if (done) {
            hreflist2.emplace_back((*it)->lpeobject_href);
        } else {
            hreflist.emplace_back((*it)->lpeobject_href);
        }
        if (it == cur_it) {
            done = true;
        }
    }
    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist));

    sp_lpe_item_cleanup_original_path_recursive(this, false, false, false, true);
    sp_lpe_item_update_patheffect(this, true, true);

    auto lpeitem = removeAllPathEffects(true);
    if (lpeitem && hreflist2.size()) {
        sp_lpe_item_enable_path_effects(lpeitem, false);
        lpeitem->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist2));
        sp_lpe_item_create_original_path_recursive(lpeitem);
        sp_lpe_item_enable_path_effects(lpeitem, true);
        sp_lpe_item_update_patheffect(lpeitem, true, true);
        lpeitem->update_satellites(true);
    }
    return lpeitem;
}

void SPLPEItem::removePathEffect(Inkscape::LivePathEffect::Effect *lpe, bool keep_paths)
{
    PathEffectList path_effect_list(*this->path_effect_list);
    if (!lpe)
        return;

    bool exist = false;
    for (auto &lperef : path_effect_list) {
        if (lperef->lpeobject == lpe->getLPEObj()) {
            setCurrentPathEffect(lperef);
            exist = true;
            break;
        }
    }
    if (exist) {
        // this function is called only with FILLET_CHAMFER if do with holderRemove LPE`s (clones LPE) need to rework
        removeCurrentPathEffect(keep_paths);
    } else {
        g_warning("LPE dont exist to remove");
    }
}

void SPLPEItem::movePathEffect(gint origin, gint dest, bool select_moved)
{
    PathEffectList new_list = *this->path_effect_list;
    auto lpe = getCurrentLPE();
    if (!lpe)
        return;

    LivePathEffectObject *lpeobj = lpe->getLPEObj();
    if (lpeobj) {
        auto const nlpe = new_list.size();
        if (!nlpe ||
            origin == dest || 
            origin > nlpe -1 || 
            dest > nlpe -1)
        {
            return;
        }
        gint selectme = 0;
        auto insertme = new_list.begin();
        auto insertto = new_list.begin();
        std::advance(insertme, origin);
        if (origin > dest) {
            std::advance(insertto, dest);
            selectme = dest;
        } else {
            std::advance(insertto, dest + 1);
            selectme = dest + 1;
        }
        new_list.insert(insertto, *insertme);
        auto removeme = new_list.begin();
        if (origin > dest) {
            std::advance(removeme, origin + 1);
        } else {
            std::advance(removeme, origin);
            selectme = dest;
        }
        new_list.erase(removeme);
        this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(new_list));
        sp_lpe_item_cleanup_original_path_recursive(this, false);
        auto select = this->path_effect_list->begin();
        std::advance(select, selectme);
        if (select_moved) {
            setCurrentPathEffect(*select);
        } else {
            PathEffectList path_effect_list(*this->path_effect_list);
            for (auto &lperef : path_effect_list) {
                if (lperef->lpeobject == lpeobj) {
                    setCurrentPathEffect(lperef);
                    break;
                }
            }
        }
    }
}


void SPLPEItem::upCurrentPathEffect()
{
    auto const lperef = getCurrentLPEReference();
    if (!lperef)
        return;

    PathEffectList new_list = *this->path_effect_list;
    auto const cur_it = std::find(new_list.begin(), new_list.end(), lperef);
    if (cur_it != new_list.end() && cur_it != new_list.begin()) {
        auto up_it = cur_it;
        --up_it;
        std::iter_swap(cur_it, up_it);
    }

    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", patheffectlist_svg_string(new_list));

    sp_lpe_item_cleanup_original_path_recursive(this, false);
}

void
SPLPEItem::update_satellites(bool recursive) {
    if (path_effect_list->empty()) {
        return;
    }
    auto grp = cast<SPGroup>(this);
    if (recursive && grp) {
        std::vector<SPItem *> item_list = grp->item_list();
        for (auto iter : item_list) {
            auto subitem = cast<SPLPEItem>(iter);
            if (subitem) {
                subitem->update_satellites(recursive);
            }
        }
    }

    // go through the list; if some are unknown or invalid, return true
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (lpeobj) {
            if (auto *lpe = lpeobj->get_lpe()) {
                lpe->update_satellites();
            }
        }
    }
}

/** used for shapes so they can see if they should also disable shape calculation and read from d= */
bool SPLPEItem::hasBrokenPathEffect() const
{
    if (path_effect_list->empty()) {
        return false;
    }

    // go through the list; if some are unknown or invalid, return true
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (!lpeobj || !lpeobj->get_lpe()) {
            return true;
        }
    }

    return false;
}

bool SPLPEItem::hasPathEffectOfTypeRecursive(int const type, bool is_ready) const
{
    auto parent_lpe_item = cast<SPLPEItem>(parent);
    if (parent_lpe_item) {
        return hasPathEffectOfType(type, is_ready) || parent_lpe_item->hasPathEffectOfTypeRecursive(type, is_ready);
    } else {
        return hasPathEffectOfType(type, is_ready);
    }
}

bool SPLPEItem::hasPathEffectOfType(int const type, bool is_ready) const
{
    if (path_effect_list->empty()) {
        return false;
    }

    for (auto const &it : *path_effect_list) {
        auto const lpeobj = it->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect const* lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type)) {
                if (is_ready || lpe->isReady()) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * returns true when any LPE apply to clip or mask.
 */
bool SPLPEItem::hasPathEffectOnClipOrMask(SPLPEItem * shape) const
{
    if (shape->hasPathEffectRecursive()) {
        return true;
    }
    if (!path_effect_list || path_effect_list->empty()) {
        return false;
    }

    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (!lpeobj) {
            continue;
        }
        Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
        if (lpe && lpe->apply_to_clippath_and_mask) {
            return true;
        }
    }
    return false;
}

/**
 * returns true when any LPE apply to clip or mask. recursive mode
 */
bool SPLPEItem::hasPathEffectOnClipOrMaskRecursive(SPLPEItem * shape) const
{
    auto parent_lpe_item = cast<SPLPEItem>(parent);
    if (parent_lpe_item) {
        return hasPathEffectOnClipOrMask(shape) || parent_lpe_item->hasPathEffectOnClipOrMaskRecursive(shape);
    }
    else {
        return hasPathEffectOnClipOrMask(shape);
    }
}

bool SPLPEItem::hasPathEffect() const
{
    if (!path_effect_list || path_effect_list->empty()) {
        return false;
    }

    // go through the list; if some are unknown or invalid, we are not an LPE item!
    PathEffectList path_effect_list(*this->path_effect_list);
    for (auto &lperef : path_effect_list) {
        LivePathEffectObject *lpeobj = lperef->lpeobject;
        if (!lpeobj || !lpeobj->get_lpe()) {
            return false;
        }
    }

    return true;
}

bool SPLPEItem::hasPathEffectRecursive() const
{
    auto parent_lpe_item = cast<SPLPEItem>(parent);
    if (parent_lpe_item) {
        return hasPathEffect() || parent_lpe_item->hasPathEffectRecursive();
    }
    else {
        return hasPathEffect();
    }
}

/**
 * returns top most LPE item with LPE
 */
SPLPEItem const * SPLPEItem::getTopPathEffect() const
{
    auto parent_lpe_item = cast<SPLPEItem>(parent);
    if (parent_lpe_item && !hasPathEffectRecursive()) {
        return hasPathEffect() ? parent_lpe_item : this;
    } else {
        return parent_lpe_item ? parent_lpe_item->getTopPathEffect() : this;
    }
}

void
SPLPEItem::resetClipPathAndMaskLPE(bool fromrecurse)
{
    if (fromrecurse) {
        auto group = cast<SPGroup>(this);
        auto shape = cast<SPShape>(this);
        if (group) {
            std::vector<SPItem*> item_list = group->item_list();
            for (auto iter2 : item_list) {
                auto subitem = cast<SPLPEItem>(iter2);
                if (subitem) {
                    subitem->resetClipPathAndMaskLPE(true);
                }
            }
        } else if (shape) {
            shape->setCurveInsync(shape->curveForEdit());
            if (!hasPathEffectOnClipOrMaskRecursive(shape)) {
                shape->removeAttribute("inkscape:original-d");
                shape->setCurveBeforeLPE(nullptr);
            } else {
                // make sure there is an original-d for paths!!!
                sp_lpe_item_create_original_path_recursive(shape);
            }
        }
        return;
    }
    SPClipPath *clip_path = this->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto iter : clip_path_list) {
            auto group = cast<SPGroup>(iter);
            auto shape = cast<SPShape>(iter);
            if (group) {
                std::vector<SPItem*> item_list = group->item_list();
                for (auto iter2 : item_list) {
                    auto subitem = cast<SPLPEItem>(iter2);
                    if (subitem) {
                        subitem->resetClipPathAndMaskLPE(true);
                    }
                }
            } else if (shape) {
                shape->setCurveInsync(shape->curveForEdit());
                if (!hasPathEffectOnClipOrMaskRecursive(shape)) {
                    shape->removeAttribute("inkscape:original-d");
                    shape->setCurveBeforeLPE(nullptr);
                } else {
                    // make sure there is an original-d for paths!!!
                    sp_lpe_item_create_original_path_recursive(shape);
                }
            }
            sp_object_unref(iter);
        }
    }
    SPMask *mask = this->getMaskObject();
    if(mask) {
        std::vector<SPObject*> mask_list = mask->childList(true);
        for (auto iter : mask_list) {
            auto group = cast<SPGroup>(iter);
            auto shape = cast<SPShape>(iter);
            if (group) {
                std::vector<SPItem*> item_list = group->item_list();
                for (auto iter2 : item_list) {
                    auto subitem = cast<SPLPEItem>(iter2);
                    if (subitem) {
                        subitem->resetClipPathAndMaskLPE(true);
                    }
                }
            } else if (shape) {
                shape->setCurveInsync(shape->curveForEdit());
                if (!hasPathEffectOnClipOrMaskRecursive(shape)) {
                    shape->removeAttribute("inkscape:original-d");
                    shape->setCurveBeforeLPE(nullptr);
                } else {
                    // make sure there is an original-d for paths!!!
                    sp_lpe_item_create_original_path_recursive(shape);
                }
            }
            sp_object_unref(iter);
        }
    }
}

void
SPLPEItem::applyToClipPath(SPItem* to, Inkscape::LivePathEffect::Effect *lpe)
{
    if (lpe && !lpe->apply_to_clippath_and_mask) {
        return;
    }
    SPClipPath *clip_path = to->getClipObject();
    if(clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        for (auto clip_data : clip_path_list) {
            applyToClipPathOrMask(cast<SPItem>(clip_data), to, lpe);
            sp_object_unref(clip_data);
        }
    }
}

void
SPLPEItem::applyToMask(SPItem* to, Inkscape::LivePathEffect::Effect *lpe)
{
    if (lpe && !lpe->apply_to_clippath_and_mask) {
        return;
    }
    SPMask *mask = to->getMaskObject();
    if(mask) {
        std::vector<SPObject*> mask_list = mask->childList(true);
        for (auto mask_data : mask_list) {
            applyToClipPathOrMask(cast<SPItem>(mask_data), to, lpe);
            sp_object_unref(mask_data);
        }
    }
}

void
SPLPEItem::applyToClipPathOrMask(SPItem *clip_mask, SPItem* to, Inkscape::LivePathEffect::Effect *lpe)
{
    auto group = cast<SPGroup>(clip_mask);
    auto shape = cast<SPShape>(clip_mask);
    SPRoot *root = this->document->getRoot();
    if (group) {
        std::vector<SPItem*> item_list = group->item_list();
        for (auto subitem : item_list) {
            applyToClipPathOrMask(subitem, to, lpe);
        }
    } else if (shape) {
        if (root->inkscape_version.isInsideRangeExclusive({0, 1}, {0, 92})) {
            shape->removeAttribute("inkscape:original-d");
        } else {
            if (shape->curve()) {
                auto c = *shape->curve();
                bool success = false;
                try {
                    if (lpe) {
                        success = this->performOnePathEffect(c, shape, lpe, true);
                    } else {
                        success = this->performPathEffect(c, shape, true);
                    }
                } catch (std::exception & e) {
                    g_warning("Exception during LPE execution. \n %s", e.what());
                    if (SP_ACTIVE_DESKTOP && SP_ACTIVE_DESKTOP->messageStack()) {
                        SP_ACTIVE_DESKTOP->messageStack()->flash( Inkscape::WARNING_MESSAGE,
                                        _("An exception occurred during execution of the Path Effect.") );
                    }
                    success = false;
                }
                if (success) {
                    auto str = sp_svg_write_path(c);
                    shape->setCurveInsync(std::move(c));
                    shape->setAttribute("d", str);
                } else {
                     // LPE was unsuccessful or doeffect stack return null.. Read the old 'd'-attribute.
                    if (gchar const * value = shape->getAttribute("d")) {
                        shape->setCurve(sp_svg_read_pathv(value));
                    }
                }
                shape->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
    }
}

Inkscape::LivePathEffect::Effect *SPLPEItem::getFirstPathEffectOfType(int type)
{
    return const_cast<Inkscape::LivePathEffect::Effect *>(
        std::as_const(*this).getFirstPathEffectOfType(type));
}

Inkscape::LivePathEffect::Effect const *SPLPEItem::getFirstPathEffectOfType(int type) const
{
    for (const auto & i : *path_effect_list) {
        LivePathEffectObject const *lpeobj = i->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect const *lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type)) {
                return lpe;
            }
        }
    }
    return nullptr;
}

template <bool as_const>
auto getPathEffectsOfTypeImpl(PathEffectList const &path_effect_list, std::optional<int> const type)
{
    using Inkscape::LivePathEffect::Effect;
    using Ptr = std::add_pointer_t<std::conditional_t<as_const, std::add_const_t<Effect>, Effect>>;
    std::vector<Ptr> effects;
    if (!type) effects.reserve(path_effect_list.size());
    for (auto const &lperef : path_effect_list) {
        if (auto const lpeobj = lperef->lpeobject) {
            if (auto const lpe = lpeobj->get_lpe(); lpe && (!type || lpe->effectType() == *type)) {
                effects.push_back(lpe);
            }
        }
    }
    return effects;
}

// TODO: Do these really need to copy the member vector to a new one? Why? And we should not shadow

std::vector<Inkscape::LivePathEffect::Effect *> SPLPEItem::getPathEffectsOfType(int type)
{
    PathEffectList path_effect_list(*this->path_effect_list);
    return getPathEffectsOfTypeImpl<false>(path_effect_list, type);
}

std::vector<Inkscape::LivePathEffect::Effect const *> SPLPEItem::getPathEffectsOfType(int type) const
{
    PathEffectList path_effect_list(*this->path_effect_list);
    return getPathEffectsOfTypeImpl<true>(path_effect_list, type);
}

std::vector<Inkscape::LivePathEffect::Effect *> SPLPEItem::getPathEffects()
{
    PathEffectList path_effect_list(*this->path_effect_list);
    return getPathEffectsOfTypeImpl<false>(path_effect_list, std::nullopt);
}

std::vector<Inkscape::LivePathEffect::Effect const *> SPLPEItem::getPathEffects() const
{
    PathEffectList path_effect_list(*this->path_effect_list);
    return getPathEffectsOfTypeImpl<true>(path_effect_list, std::nullopt);
}

void SPLPEItem::editNextParamOncanvas(SPDesktop *dt)
{
    auto const lperef = this->getCurrentLPEReference();
    if (lperef && lperef->lpeobject && lperef->lpeobject->get_lpe()) {
        lperef->lpeobject->get_lpe()->editNextParamOncanvas(this, dt);
    }
}

void SPLPEItem::child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) {
    SPItem::child_added(child, ref);

    if (this->hasPathEffectRecursive()) {
        SPObject *ochild = this->get_child_by_repr(child);

        if ( ochild && is<SPLPEItem>(ochild) ) {
            sp_lpe_item_create_original_path_recursive(cast<SPLPEItem>(ochild));
        }
    }
}
void SPLPEItem::remove_child(Inkscape::XML::Node * child) {
    SPObject *ochild = this->get_child_by_repr(child);
    if (ochild && is<SPLPEItem>(ochild) && cast<SPLPEItem>(ochild)->hasPathEffectRecursive()) {
        // we not need to update item because keep paths is false
        sp_lpe_item_cleanup_original_path_recursive(cast<SPLPEItem>(ochild), false);
    }

    SPItem::remove_child(child);
}

static std::string patheffectlist_svg_string(PathEffectList const & list)
{
    HRefList hreflist;

    for (auto const &it : list) {
        hreflist.emplace_back(it->lpeobject_href);
    }

    return hreflist_svg_string(hreflist);
}

/**
 *  THE function that should be used to generate any patheffectlist string.
 * one of the methods to change the effect list:
 *  - create temporary href list
 *  - populate the templist with the effects from the old list that you want to have and their order
 *  - call this function with temp list as param
 */
static std::string hreflist_svg_string(HRefList const & list)
{
    std::string r;
    bool semicolon_first = false;

    for (auto const &it : list) {
        if (semicolon_first) {
            r += ';';
        }

        semicolon_first = true;

        r += it;
    }

    return r;
}

// Return a copy of the effect list
PathEffectList SPLPEItem::getEffectList()
{
    return *path_effect_list;
}

// Return a copy of the effect list
PathEffectList const SPLPEItem::getEffectList() const
{
    return *path_effect_list;
}

PathEffectSharedPtr
SPLPEItem::getPrevLPEReference(PathEffectSharedPtr const &lperef)
{
    PathEffectSharedPtr prev = nullptr;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            break;
        }
        prev = it;
    }
    return prev;
}

PathEffectSharedPtr
SPLPEItem::getNextLPEReference(PathEffectSharedPtr const &lperef)
{
    bool match = false;
    for (auto & it : *path_effect_list) {
        if (match) {
            return it;
        }
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            match = true;
        }
    }
    return nullptr;
}

PathEffectSharedPtr
SPLPEItem::getLastLPEReference()
{
    return path_effect_list->back();
}

std::size_t
SPLPEItem::getLPEReferenceIndex(PathEffectSharedPtr const &lperef) const
{
    std::size_t counter = 0;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            return counter;
        }
        counter++;
    }
    return Glib::ustring::npos;
}

PathEffectSharedPtr SPLPEItem::getCurrentLPEReference()
{
    if (!this->current_path_effect && !this->path_effect_list->empty()) {
        setCurrentPathEffect(this->path_effect_list->back());
    }
    if (this->path_effect_list->empty()) {
        current_path_effect = nullptr;
    }
    return current_path_effect;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getCurrentLPE()
{
    auto const lperef = getCurrentLPEReference();

    if (lperef && lperef->lpeobject)
        return lperef->lpeobject->get_lpe();
    else
        return nullptr;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getPrevLPE(Inkscape::LivePathEffect::Effect* lpe)
{
    Inkscape::LivePathEffect::Effect* prev = nullptr;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject == lpe->getLPEObj()) {
            break;
        }
        prev = it->lpeobject->get_lpe();
    }
    return prev;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getNextLPE(Inkscape::LivePathEffect::Effect* lpe)
{
    bool match = false;
    for (auto & it : *path_effect_list) {
        if (match) {
            return it->lpeobject->get_lpe();
        }
        if (it->lpeobject == lpe->getLPEObj()) {
            match = true;
        }
    }
    return nullptr;
}

Inkscape::LivePathEffect::Effect* SPLPEItem::getLastLPE()
{
    Inkscape::LivePathEffect::Effect* last = nullptr;
    for (auto & it : *path_effect_list) {
        last = it->lpeobject->get_lpe();
    }
    return last;
}

std::size_t
SPLPEItem::countLPEOfType(int const type, bool const inc_hidden, bool const is_ready) const
{
    std::size_t counter = 0;
    if (path_effect_list->empty()) {
        return counter;
    }

    for (auto const &it : *path_effect_list) {
        auto const lpeobj = it->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect const* lpe = lpeobj->get_lpe();
            if (lpe && (lpe->effectType() == type) && (lpe->is_visible || inc_hidden)) {
                if (is_ready || lpe->isReady()) {
                    counter++;
                }
            }
        }
    }

    return counter;
}

std::size_t
SPLPEItem::getLPEIndex(Inkscape::LivePathEffect::Effect* lpe) const
{
    std::size_t counter = 0;
    for (auto & it : *path_effect_list) {
        if (it->lpeobject == lpe->getLPEObj()) {
            return counter;
        }
        counter++;
    }
    return Glib::ustring::npos;
}

bool SPLPEItem::setCurrentPathEffect(PathEffectSharedPtr const &lperef)
{
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lperef->lpeobject_repr) {
            this->current_path_effect = it;  // current_path_effect should always be a pointer from the path_effect_list !
            return true;
        }
    }

    return false;
}

bool SPLPEItem::setCurrentPathEffect(LivePathEffectObject const * lopeobj)
{
    for (auto & it : *path_effect_list) {
        if (it->lpeobject_repr == lopeobj->getRepr()) {
            this->current_path_effect = it;  // current_path_effect should always be a pointer from the path_effect_list !
            return true;
        }
    }

    return false;
}

std::vector<SPObject *> SPLPEItem::get_satellites(bool force, bool recursive, bool onchilds)
{
    std::vector<SPObject *> satellites;
    if (onchilds) {
        auto group = cast<SPGroup>(this);
        if (group) {
            std::vector<SPItem*> item_list = group->item_list();
            for (auto child:item_list) {
                auto lpechild = cast<SPLPEItem>(child);
                if (lpechild) {
                    std::vector<SPObject *> tmp = lpechild->get_satellites(force, recursive);
                    satellites.insert( satellites.end(), tmp.begin(), tmp.end() );
                }
            }
        }
    }
    for (auto &it : *path_effect_list) {
        LivePathEffectObject *lpeobj = it->lpeobject;
        if (lpeobj) {
            Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
            if (lpe) {
                std::vector<SPObject *> tmp = lpe->effect_get_satellites(force);
                satellites.insert(satellites.begin(), tmp.begin(), tmp.end());
            }
        }
    }
    if (recursive) {
        std::vector<SPObject *> allsatellites;
        for (auto satellite : satellites) {
            SPLPEItem *lpeitem = nullptr;
            if ( satellite && ( lpeitem = cast<SPLPEItem>(satellite) )) {
                std::vector<SPObject *> tmp = lpeitem->get_satellites(force, recursive);
                allsatellites.insert(allsatellites.begin(), tmp.begin(), tmp.end());
            }
        }
        satellites.insert(satellites.begin(), allsatellites.begin(), allsatellites.end());
    }
    return satellites;
}

/**
 * Writes a new "inkscape:path-effect" string to xml, where the old_lpeobjects are substituted by the new ones.
 *  Note that this method messes up the item's \c PathEffectList.
 */
void SPLPEItem::replacePathEffects( std::vector<LivePathEffectObject const *> const &old_lpeobjs,
                                    std::vector<LivePathEffectObject const *> const &new_lpeobjs )
{
    HRefList hreflist;
    for (auto const &it : *this->path_effect_list) {
        auto const current_lpeobj = it->lpeobject;
        auto const found_it = std::find(old_lpeobjs.cbegin(), old_lpeobjs.cend(), current_lpeobj);
        if (found_it != old_lpeobjs.cend()) {
            auto const found_index = std::distance(old_lpeobjs.cbegin(), found_it);
            const gchar * repr_id = new_lpeobjs[found_index]->getRepr()->attribute("id");
            gchar *hrefstr = g_strdup_printf("#%s", repr_id);
            hreflist.emplace_back(hrefstr);
            g_free(hrefstr);
        } else {
            hreflist.emplace_back(it->lpeobject_href);
        }
    }

    this->setAttributeOrRemoveIfEmpty("inkscape:path-effect", hreflist_svg_string(hreflist));
}

/**
 *  Check all effects in the stack if they are used by other items, and fork them if so.
 *  It is not recommended to fork the effects by yourself calling LivePathEffectObject::fork_private_if_necessary,
 *  use this method instead.
 *  Returns true if one or more effects were forked; returns false if nothing was done.
 */
bool SPLPEItem::forkPathEffectsIfNecessary(unsigned int nr_of_allowed_users, bool recursive, bool force)
{
    bool forked = false;
    auto group = cast<SPGroup>(this);
    if (group && recursive) {
        std::vector<SPItem*> item_list = group->item_list();
        for (auto child:item_list) {
            auto lpeitem = cast<SPLPEItem>(child);
            if (lpeitem && lpeitem->forkPathEffectsIfNecessary(nr_of_allowed_users, recursive)) {
                forked = true;
            }
        }
    }

    if ( this->hasPathEffect() ) {
        // If one of the path effects is used by 2 or more items, fork it
        // so that each object has its own independent copy of the effect.
        // Note: replacing path effects messes up the path effect list

        // Clones of the LPEItem will increase the refcount of the lpeobjects.
        // Therefore, nr_of_allowed_users should be increased with the number of clones (i.e. refs to the lpeitem)
        // is not well handled forker because is based in hrefcount 
        // to handle clones and this can be wrong with other references
        // for this I add a new parameter to allow force fork
        nr_of_allowed_users += this->hrefcount;
        if (force) {
            nr_of_allowed_users = 1;
        }
        std::vector<LivePathEffectObject const*> old_lpeobjs, new_lpeobjs;
        std::vector<LivePathEffectObject *> upd_lpeobjs;
        PathEffectList effect_list = this->getEffectList();
        for (auto & it : effect_list)
        {
            LivePathEffectObject *lpeobj = it->lpeobject;
            if (lpeobj) {
                LivePathEffectObject *forked_lpeobj = lpeobj->fork_private_if_necessary(nr_of_allowed_users);
                if (forked_lpeobj && forked_lpeobj != lpeobj) {
                    forked = true;
                    forked_lpeobj->get_lpe()->is_load = true;
                    forked_lpeobj->get_lpe()->sp_lpe_item = this;
                    old_lpeobjs.push_back(lpeobj);
                    new_lpeobjs.push_back(forked_lpeobj);
                    upd_lpeobjs.push_back(forked_lpeobj);
                }
            }
        }

        if (forked) {
            this->replacePathEffects(old_lpeobjs, new_lpeobjs);
            for (auto &forked_lpeobj : upd_lpeobjs) {
                forked_lpeobj->get_lpe()->read_from_SVG();
            }
        }
    }

    return forked;
}

// Enable or disable the path effects of the item.
void sp_lpe_item_enable_path_effects(SPLPEItem *lpeitem, bool enable)
{
    if (enable) {
        lpeitem->path_effects_enabled++;
    }
    else {
        lpeitem->path_effects_enabled--;
    }
}

// Are the path effects enabled on this item ?
bool SPLPEItem::pathEffectsEnabled() const
{
    return !onsymbol && path_effects_enabled > 0;
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
