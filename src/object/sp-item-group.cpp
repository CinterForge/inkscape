// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <g> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2006 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstring>
#include <glibmm/i18n.h>

#include "attributes.h"
#include "document-undo.h"
#include "document.h"
#include "enums.h"
#include "helper/geom.h"
#include "preferences.h"
#include "style.h"

#include "box3d.h"
#include "object-set.h"
#include "sp-clippath.h"
#include "sp-defs.h"
#include "sp-desc.h"
#include "sp-flowtext.h"
#include "sp-mask.h"
#include "sp-offset.h"
#include "sp-root.h"
#include "sp-switch.h"
#include "sp-textpath.h"
#include "sp-title.h"
#include "sp-use.h"

#include "display/drawing-group.h"
#include "live_effects/effect.h"
#include "live_effects/lpe-clone-original.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "svg/svg.h"
#include "xml/repr.h"

using Inkscape::DocumentUndo;

static void sp_group_perform_patheffect(SPGroup *group, SPGroup *top_group, Inkscape::LivePathEffect::Effect *lpe, bool write);

SPGroup::SPGroup() : SPLPEItem(),
    _insert_bottom(false),
    _layer_mode(SPGroup::GROUP)
{
}

SPGroup::~SPGroup() = default;

void SPGroup::build(SPDocument *document, Inkscape::XML::Node *repr) {
    this->readAttr(SPAttr::INKSCAPE_GROUPMODE);

    SPLPEItem::build(document, repr);
}

void SPGroup::release() {
    if (this->_layer_mode == SPGroup::LAYER) {
        this->document->removeResource("layer", this);
    }

    SPLPEItem::release();
}

void SPGroup::child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) {
    SPLPEItem::child_added(child, ref);

    SPObject *last_child = this->lastChild();
    if (last_child && last_child->getRepr() == child || adding_children) {
        // optimization for the common special case where the child is being added at the end
        // or if adding a set of contiguous children (we delay moving them).
        auto item = cast<SPItem>(get_child_by_repr(child));
        if ( item ) {
            /* TODO: this should be moved into SPItem somehow */
            for (auto &v : views) {
                auto ac = item->invoke_show(v.drawingitem->drawing(), v.key, v.flags);
                if (ac) {
                    v.drawingitem->appendChild(ac);
                }
            }
        }
    } else {    // general case
        auto item = cast<SPItem>(get_child_by_repr(child));
        if ( item ) {
            /* TODO: this should be moved into SPItem somehow */
            unsigned position = item->pos_in_parent();

            for (auto &v : views) {
                auto ac = item->invoke_show (v.drawingitem->drawing(), v.key, v.flags);
                if (ac) {
                    v.drawingitem->prependChild(ac);
                    ac->setZOrder(position);
                }
            }
        }
    }
    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

// Move drawing items into correct place.
void SPGroup::end_adding_children(unsigned count, unsigned position)
{
    for (auto &v : views) {
        v.drawingitem->shiftFromEnd(count, position);
    }
    adding_children = false;
}


/* fixme: hide (Lauris) */

void SPGroup::remove_child(Inkscape::XML::Node *child) {
    SPLPEItem::remove_child(child);
    if (this->hasPathEffectRecursive()) {
        sp_lpe_item_update_patheffect(this, true, true);
        this->requestModified(SP_OBJECT_MODIFIED_FLAG);
    }
}

void SPGroup::order_changed (Inkscape::XML::Node *child, Inkscape::XML::Node *old_ref, Inkscape::XML::Node *new_ref)
{
    SPLPEItem::order_changed(child, old_ref, new_ref);

    auto item = cast<SPItem>(get_child_by_repr(child));
    if ( item ) {
        /* TODO: this should be moved into SPItem somehow */
        unsigned position = item->pos_in_parent();
        for (auto &v : item->views) {
            v.drawingitem->setZOrder(position);
        }
    }

    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

void SPGroup::update(SPCtx *ctx, unsigned int flags) {
    // std::cout << "SPGroup::update(): " << (getId()?getId():"null") << std::endl;
    auto ictx = (SPItemCtx *)ctx;

    unsigned childflags = flags;

    if (flags & SP_OBJECT_MODIFIED_FLAG) {
      childflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }
    childflags &= SP_OBJECT_MODIFIED_CASCADE;
    std::vector<SPObject*> l=this->childList(true, SPObject::ActionUpdate);
    for(auto child : l){
        if (childflags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            auto cctx = *ictx; // reset context between siblings in case one of them alters it
            if (auto item = cast<SPItem>(child)) {
                cctx.i2doc = item->transform * ictx->i2doc;
                cctx.i2vp = item->transform * ictx->i2vp;
            }
            child->updateDisplay((SPCtx *)&cctx, childflags);
        }

        sp_object_unref(child);
    }

    // For a group, we need to update ourselves *after* updating children.
    // this is because the group might contain shapes such as rect or ellipse,
    // which recompute their equivalent path (a.k.a curve) in the update callback,
    // and this is in turn used when computing bbox.
    SPLPEItem::update(ctx, flags);

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (auto &v : views) {
            auto group = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
            if (parent) {
                context_style = parent->context_style;
            }
            group->setStyle(style, context_style);
        }
    }
}

void SPGroup::modified(guint flags) {
    //std::cout << "SPGroup::modified(): " << (getId()?getId():"null") << std::endl;
    SPLPEItem::modified(flags);
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (auto &v : views) {
            auto group = cast<Inkscape::DrawingGroup>(v.drawingitem.get());
            group->setStyle(this->style);
        }
    }

    std::vector<SPObject*> l=this->childList(true);
    for(auto child : l){
        if ((flags & SP_OBJECT_FLAGS_ALL) || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child);
    }
}

Inkscape::XML::Node* SPGroup::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    if (flags & SP_OBJECT_WRITE_BUILD) {
        std::vector<Inkscape::XML::Node *> l;

        if (!repr) {
            if (is<SPSwitch>(this)) {
                repr = xml_doc->createElement("svg:switch");
            } else {
                repr = xml_doc->createElement("svg:g");
            }
        }

        for (auto& child: children) {
            if (!is<SPTitle>(&child) && !is<SPDesc>(&child)) {
                Inkscape::XML::Node *crepr = child.updateRepr(xml_doc, nullptr, flags);

                if (crepr) {
                    l.push_back(crepr);
                }
            }
        }
        for (auto i=l.rbegin();i!=l.rend();++i) {
            repr->addChild(*i, nullptr);
            Inkscape::GC::release(*i);
        }
    } else {
        for (auto& child: children) {
            if (!is<SPTitle>(&child) && !is<SPDesc>(&child)) {
                child.updateRepr(flags);
            }
        }
    }

    if ( flags & SP_OBJECT_WRITE_EXT ) {
        const char *value;
        if ( _layer_mode == SPGroup::LAYER ) {
            value = "layer";
        } else if ( _layer_mode == SPGroup::MASK_HELPER ) {
            value = "maskhelper";
        } else if ( flags & SP_OBJECT_WRITE_ALL ) {
            value = "group";
        } else {
            value = nullptr;
        }

        repr->setAttribute("inkscape:groupmode", value);
    }

    SPLPEItem::write(xml_doc, repr, flags);

    return repr;
}

Geom::OptRect SPGroup::bbox(Geom::Affine const &transform, SPItem::BBoxType bboxtype) const
{
    Geom::OptRect bbox;

    // TODO CPPIFY: replace this const_cast later
    std::vector<SPObject*> l = const_cast<SPGroup*>(this)->childList(false, SPObject::ActionBBox);
    for(auto o : l){
        auto item = cast<SPItem>(o);
        if (item && !item->isHidden()) {
            Geom::Affine const ct(item->transform * transform);
            bbox |= item->bounds(bboxtype, ct);
        }
    }

    return bbox;
}

void SPGroup::print(SPPrintContext *ctx) {
    for(auto& child: children){
        SPObject *o = &child;
        auto item = cast<SPItem>(o);
        if (item) {
            item->invoke_print(ctx);
        }
    }
}

const char *SPGroup::typeName() const {
    switch (_layer_mode) {
        case SPGroup::LAYER:
            return "layer";
        case SPGroup::MASK_HELPER:
        case SPGroup::GROUP:
        default:
            return "group";
    }
}

const char *SPGroup::displayName() const {
    switch (_layer_mode) {
        case SPGroup::LAYER:
            return _("Layer");
        case SPGroup::MASK_HELPER:
            return _("Mask Helper");
        case SPGroup::GROUP:
        default:
            return C_("Noun", "Group");
    }
}

gchar *SPGroup::description() const {
    gint len = this->getItemCount();
    return g_strdup_printf(
        ngettext("of <b>%d</b> object", "of <b>%d</b> objects", len), len);
}

void SPGroup::set(SPAttr key, gchar const* value) {
    switch (key) {
        case SPAttr::INKSCAPE_GROUPMODE:
            if ( value && !strcmp(value, "layer") ) {
                this->setLayerMode(SPGroup::LAYER);
            } else if ( value && !strcmp(value, "maskhelper") ) {
                this->setLayerMode(SPGroup::MASK_HELPER);
            } else {
                this->setLayerMode(SPGroup::GROUP);
            }
            break;

        default:
            SPLPEItem::set(key, value);
            break;
    }
}

Inkscape::DrawingItem *SPGroup::show (Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) {
    // std::cout << "SPGroup::set_visible(true): " << (getId()?getId():"null") << std::endl;
    Inkscape::DrawingGroup *ai;

    ai = new Inkscape::DrawingGroup(drawing);
    ai->setPickChildren(this->effectiveLayerMode(key) == SPGroup::LAYER);
    if( this->parent ) {
        this->context_style = this->parent->context_style;
    }
    ai->setStyle(this->style, this->context_style);

    this->_showChildren(drawing, ai, key, flags);
    return ai;
}

void SPGroup::hide (unsigned int key) {
    std::vector<SPObject*> l=this->childList(false, SPObject::ActionShow);
    for(auto o : l){
        auto item = cast<SPItem>(o);
        if (item) {
            item->invoke_hide(key);
        }
    }

//    SPLPEItem::onHide(key);
}

std::vector<SPItem*> SPGroup::item_list()
{
    std::vector<SPItem *> ret;
    for (auto& child: children) {
        if (auto item = cast<SPItem>(const_cast<SPObject *>(&child))) {
            ret.push_back(item);
        }
    }
    return ret;
}

void SPGroup::snappoints(std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs) const {
    for (auto& o: children)
    {
        SPItem const *item = cast<SPItem>(&o);
        if (item) {
            item->getSnappoints(p, snapprefs);
        }
    }
}

/**
 * Helper function for ungrouping. Compensates the transform of linked items
 * (clones, linked offset, text-on-path, text with shape-inside) who's source is a
 * direct child of the group being ungrouped (or will be moved to a different
 * group or layer).
 *
 * @param item An object which may be linked to `expected_source`
 * @param expected_source An object who's transform attribute (but not its
 * i2doc transform) will change (later) due to moving to a different group
 * @param source_transform A transform which will be applied to
 * `expected_source` (later) and needs to be compensated in its linked items
 *
 * @post item and its representation are updated
 */
static void _ungroup_compensate_source_transform(SPItem *item, SPItem const *const expected_source,
                                                 Geom::Affine const &source_transform)
{
    if (!item || item->cloned) {
        return;
    }

    SPItem *source = nullptr;
    SPText *item_text = nullptr;
    SPOffset *item_offset = nullptr;
    SPUse *item_use = nullptr;
    auto lpeitemclone = cast<SPLPEItem>(item);

    bool override = false;
    if ((item_offset = cast<SPOffset>(item))) {
        source = sp_offset_get_source(item_offset);
    } else if ((item_text = cast<SPText>(item))) {
        source = item_text->get_first_shape_dependency();
    } else if (auto textpath = cast<SPTextPath>(item)) {
        item_text = cast<SPText>(textpath->parent);
        if (!item_text)
            return;
        item = item_text;
        source = sp_textpath_get_path_item(textpath);
    } else if ((item_use = cast<SPUse>(item))) {
        source = item_use->get_original();
    } else if (lpeitemclone && lpeitemclone->hasPathEffectOfType(Inkscape::LivePathEffect::CLONE_ORIGINAL)) {
        override = true;
    }

    if (source != expected_source && !override) {
        return;
    }

    // FIXME: constructing a transform that would fully preserve the appearance of a
    // textpath if it is ungrouped with its path seems to be impossible in general
    // case. E.g. if the group was squeezed, to keep the ungrouped textpath squeezed
    // as well, we'll need to relink it to some "virtual" path which is inversely
    // stretched relative to the actual path, and then squeeze the textpath back so it
    // would both fit the actual path _and_ be squeezed as before. It's a bummer.

    auto const adv = item->transform.inverse() * source_transform * item->transform;
    double const scale = source_transform.descrim();

    if (item_text) {
        item_text->_adjustFontsizeRecursive(item_text, scale);
    } else if (item_offset) {
        item_offset->rad *= scale;
    } else if (item_use) {
        item->transform = Geom::Translate(item_use->x.computed, item_use->y.computed) * item->transform;
        item_use->x = 0;
        item_use->y = 0;
    }

    if (!item_use) {
        item->adjust_stroke_width_recursive(scale);
        item->adjust_paint_recursive(adv, Geom::identity(), SPItem::PATTERN);
        item->adjust_paint_recursive(adv, Geom::identity(), SPItem::HATCH);
        item->adjust_paint_recursive(adv, Geom::identity(), SPItem::GRADIENT);
    }

    item->transform = source_transform.inverse() * item->transform;
    item->updateRepr();
}

void sp_item_group_ungroup_handle_clones(SPItem *parent, Geom::Affine const g)
{
    // copy the list because the original may get invalidated
    auto hrefListCopy = parent->hrefList;

    for (auto *cobj : hrefListCopy) {
        _ungroup_compensate_source_transform(cast<SPItem>(cobj), parent, g);
    }
}

/*
 * Get bbox of clip/mask if is a rect to fix PDF import issues
 */
Geom::OptRect bbox_on_rect_clip (SPObject *object) {
    auto shape = cast<SPShape>(object);
    Geom::OptRect bbox_clip = Geom::OptRect();
    if (shape) {
        auto curve = shape->curve();
        if (curve) {
            Geom::PathVector pv = *curve;
            std::vector<Geom::Point> nodes = pv.nodes();
            if (pv.size() == 1 && nodes.size() == 4) {
                if (Geom::are_near(nodes[0][Geom::X],nodes[3][Geom::X]) &&
                    Geom::are_near(nodes[1][Geom::X],nodes[2][Geom::X]) &&
                    Geom::are_near(nodes[0][Geom::Y],nodes[1][Geom::Y]) &&
                    Geom::are_near(nodes[2][Geom::Y],nodes[3][Geom::Y]))
                {
                    bbox_clip = shape->visualBounds();
                    bbox_clip->expandBy(1);
                }
            }
        }
    }
    return bbox_clip;
}

/*
 * Get clip and item has the same path, PDF fix
 */
bool equal_clip (SPItem *item, SPObject *clip) {
    auto shape = cast<SPShape>(item);
    auto shape_clip = cast<SPShape>(clip);
    bool equal = false;
    if (shape && shape_clip) {
        auto filter = shape->style->getFilter();
        auto stroke = shape->style->getFillOrStroke(false);
        if (!filter && (!stroke || stroke->isNone())) {
            auto curve = shape->curve();
            auto curve_clip = shape_clip->curve();
            if (curve && curve_clip) {
                equal = pathv_similar(*curve, *curve_clip, 0.01);
            }
        }
    }
    return equal;
}

/**
 * Unlink all clones of the group.
 *
 * @note Should get the same result with `group->_delete_signal.emit(group)` if
 * "/options/cloneorphans" is SP_CLONE_ORPHANS_UNLINK.
 */
static void unlink_clones_of(SPGroup *group)
{
    // copy the list because the original may get invalidated
    auto hrefListCopy = group->hrefList;

    for (auto *cobj : hrefListCopy) {
        if (auto clone = cast<SPUse>(cobj)) {
            clone->unlink();
        }
    }
}

void
sp_item_group_ungroup (SPGroup *group, std::vector<SPItem*> &children)
{
    g_return_if_fail (group != nullptr);

    SPDocument *doc = group->document;
    SPRoot *root = doc->getRoot();
    SPObject *defs = root->defs;
    auto const prefs = Inkscape::Preferences::get();
    // TODO handle better ungrouping
    // now is used in clipmask LPE on remove
    prefs->setBool("/options/onungroup", true);

    // this converts the gradient/pattern fill/stroke on the group, if any, to userSpaceOnUse
    group->adjust_paint_recursive(Geom::identity(), Geom::identity());

    auto pitem = cast<SPItem>(group->parent);
    g_assert(pitem);
    Inkscape::XML::Node *prepr = pitem->getRepr();

    if (auto box = cast<SPBox3D>(group)) {
        group = box->convert_to_group();
    }

    group->removeAllPathEffects(false);
    bool maskonungroup = prefs->getBool("/options/maskobject/maskonungroup", true);
    bool topmost = prefs->getBool("/options/maskobject/topmost", true);
    int grouping = prefs->getInt("/options/maskobject/grouping", PREFS_MASKOBJECT_GROUPING_NONE);
    SPObject *clip = nullptr;
    SPObject *mask = nullptr;
    if (maskonungroup) {
        Inkscape::ObjectSet tmp_clip_set(doc);
        tmp_clip_set.add(group);
        Inkscape::ObjectSet tmp_mask_set(doc);
        tmp_mask_set.add(group);
        auto *clip_obj = group->getClipObject();
        auto *mask_obj = group->getMaskObject();
        prefs->setBool("/options/maskobject/topmost", true);
        prefs->setInt("/options/maskobject/grouping", PREFS_MASKOBJECT_GROUPING_NONE);
        if (clip_obj) {
            tmp_clip_set.unsetMask(true, false, true);
            tmp_clip_set.remove(group);
            clip = tmp_clip_set.singleItem();
            // TODO: handle multiple children of a <clipPath>. Currently,
            // ObjectSet::setMask() can only set a single item as clip.
        }
        if (mask_obj) {
            tmp_mask_set.unsetMask(false, false, true);
            tmp_mask_set.remove(group);
            tmp_mask_set.group();
            mask = tmp_mask_set.singleItem();
        }
    }
    /* Step 1 - generate lists of children objects */
    std::vector<Inkscape::XML::Node *> items;
    std::vector<Inkscape::XML::Node *> objects;
    Geom::Affine const g = i2anc_affine(group, group->parent);

    // Unlink clones of group before modifying any transforms
    unlink_clones_of(group);

    if (!g.isIdentity()) {
        for (auto &child : group->children) {
            if (auto citem = cast<SPItem>(&child)) {
                auto lpeitem = cast<SPLPEItem>(citem);
                if (lpeitem) {
                    for (auto lpe :
                         lpeitem->getPathEffectsOfType(Inkscape::LivePathEffect::EffectType::CLONE_ORIGINAL)) {
                        auto clonelpe = dynamic_cast<Inkscape::LivePathEffect::LPECloneOriginal*>(lpe);
                        if (clonelpe) {
                            SPObject *linked = clonelpe->linkeditem.getObject();
                            if (linked) {
                                bool breakparent = false;
                                for (auto &child2 : group->children) {
                                    if (cast<SPItem>(&child2) == linked) {
                                        _ungroup_compensate_source_transform(citem, cast<SPItem>(linked), g);
                                        breakparent = true;
                                        break;
                                    }
                                }
                                if (breakparent) {
                                    break;
                                }
                            }
                        }
                    }
                }
                sp_item_group_ungroup_handle_clones(citem, g);
            }
        }
    }

    for (auto& child: group->children) {
        auto citem = cast<SPItem>(&child);
        if (citem) {
            /* Merging of style */
            // this converts the gradient/pattern fill/stroke, if any, to userSpaceOnUse; we need to do
            // it here _before_ the new transform is set, so as to use the pre-transform bbox
            citem->adjust_paint_recursive(Geom::identity(), Geom::identity());

            child.style->merge( group->style );
            /*
             * fixme: We currently make no allowance for the case where child is cloned
             * and the group has any style settings.
             *
             * (This should never occur with documents created solely with the current
             * version of inkscape without using the XML editor: we usually apply group
             * style changes to children rather than to the group itself.)
             *
             * If the group has no style settings, then style->merge() should be a no-op. Otherwise
             * (i.e. if we change the child's style to compensate for its parent going away)
             * then those changes will typically be reflected in any clones of child,
             * whereas we'd prefer for Ungroup not to affect the visual appearance.
             *
             * The only way of preserving styling appearance in general is for child to
             * be put into a new group -- a somewhat surprising response to an Ungroup
             * command.  We could add a new groupmode:transparent that would mostly
             * hide the existence of such groups from the user (i.e. editing behaves as
             * if the transparent group's children weren't in a group), though that's
             * extra complication & maintenance burden and this case is rare.
             */

            // Merging transform
            citem->transform *= g;

            child.updateRepr();

            Inkscape::XML::Node *nrepr = child.getRepr()->duplicate(prepr->document());
            items.push_back(nrepr);

        } else {
            Inkscape::XML::Node *nrepr = child.getRepr()->duplicate(prepr->document());
            objects.push_back(nrepr);
        }
    }

    /* Step 2 - clear group */
    // remember the position of the group
    auto insert_after = group->getRepr()->prev();
    unsigned position = group->pos_in_parent();
    // the group is leaving forever, no heir, clones should take note; its children however are going to reemerge
    group->deleteObject(true, false);

    /* Step 3 - add nonitems */
    if (!objects.empty()) {
        Inkscape::XML::Node *last_def = defs->getRepr()->lastChild();
        for (auto i=objects.rbegin();i!=objects.rend();++i) {
            Inkscape::XML::Node *repr = *i;
            if (!sp_repr_is_meta_element(repr)) {
                defs->getRepr()->addChild(repr, last_def);
            }
            Inkscape::GC::release(repr);
        }
    }
    Inkscape::ObjectSet result_mask_set(doc);
    Inkscape::ObjectSet result_clip_set(doc);
    Geom::OptRect bbox_clip = Geom::OptRect();
    if (clip) { // if !maskonungroup is always null
        bbox_clip = bbox_on_rect_clip(clip);
    }

    /* Step 4 - add items */
    dynamic_cast<SPGroup *>(pitem)->start_adding_children(); // Postpone shifting drawing items into correct place.
    std::vector<SPLPEItem *> lpeitems;
    for (auto *repr : items) {
        // add item
        prepr->addChild(repr, insert_after);
        insert_after = repr;

        // fill in the children list if non-null
        SPItem *item = static_cast<SPItem *>(doc->getObjectByRepr(repr));
        auto lpeitem = cast<SPLPEItem>(item);
        if (item) {
            if (lpeitem) {
                lpeitems.push_back(lpeitem);
                sp_lpe_item_enable_path_effects(lpeitem, false);
                children.insert(children.begin(), item);
            } else {
                item->doWriteTransform(item->transform, nullptr, false);
                children.insert(children.begin(), item);
                item->requestModified(SP_OBJECT_MODIFIED_FLAG);
            }
        } else {
            g_assert_not_reached();
        }

        Inkscape::GC::release(repr);
        if (!lpeitem && clip && item) { // if !maskonungroup is always null
            Geom::OptRect bbox_item = item->visualBounds();
            if (bbox_item && !equal_clip(item, clip)) {
                if (!bbox_clip || !(*bbox_clip).contains(*bbox_item)) {
                    result_clip_set.add(item);
                }
            }
        }
        if (mask && item) {
            result_mask_set.add(item);
        }
    }
    dynamic_cast<SPGroup *>(pitem)->end_adding_children(items.size(), position); // Shift drawing items into correct place.

    if (mask) {
        result_mask_set.add(mask);
        result_mask_set.setMask(false, false, false);
        mask->deleteObject(true, false);
    }
    for (auto lpeitem : lpeitems) {
        sp_lpe_item_enable_path_effects(lpeitem, true);
        lpeitem->doWriteTransform(lpeitem->transform, nullptr, false);
        lpeitem->requestModified(SP_OBJECT_MODIFIED_FLAG);
        if (clip && lpeitem) { // if !maskonungroup is always null
            Geom::OptRect bbox_item = lpeitem->visualBounds();
            if (bbox_item && !equal_clip(lpeitem, clip)) {
                if (!bbox_clip || !(*bbox_clip).contains(*bbox_item)) {
                    result_clip_set.add(lpeitem);
                }
            }
        }
    }
    if (clip) { // if !maskonungroup is always null
        if (result_clip_set.size()) {
            result_clip_set.add(clip);
            result_clip_set.setMask(true, false, false);
        }
        clip->deleteObject(true, false);
    }
    prefs->setBool("/options/maskobject/topmost", topmost);
    prefs->setInt("/options/maskobject/grouping", grouping);
    prefs->setBool("/options/onungroup", false);
}

/*
 * some API for list aspect of SPGroup
 */

SPObject *sp_item_group_get_child_by_name(SPGroup *group, SPObject *ref, const gchar *name)
{
    SPObject *child = (ref) ? ref->getNext() : group->firstChild();
    while ( child && strcmp(child->getRepr()->name(), name) ) {
        child = child->getNext();
    }
    return child;
}

void SPGroup::setLayerMode(LayerMode mode) {
    if ( _layer_mode != mode ) {
        if ( mode == LAYER ) {
            this->document->addResource("layer", this);
        } else if ( _layer_mode == LAYER ) {
            this->document->removeResource("layer", this);
        }
        _layer_mode = mode;
        _updateLayerMode();
    }
}

SPGroup::LayerMode SPGroup::layerDisplayMode(unsigned int dkey) const {
    std::map<unsigned int, LayerMode>::const_iterator iter;
    iter = _display_modes.find(dkey);
    if ( iter != _display_modes.end() ) {
        return (*iter).second;
    } else {
        return GROUP;
    }
}

void SPGroup::setInsertBottom(bool insertbottom) {
    if ( _insert_bottom != insertbottom) {
        _insert_bottom = insertbottom;
    }
}

void SPGroup::setLayerDisplayMode(unsigned int dkey, SPGroup::LayerMode mode) {
    if ( layerDisplayMode(dkey) != mode ) {
        _display_modes[dkey] = mode;
        _updateLayerMode(dkey);
    }
}

void SPGroup::_updateLayerMode(unsigned int display_key) {
    for (auto &v : views) {
        if (!display_key || v.key == display_key) {
            if (auto g = cast<Inkscape::DrawingGroup>(v.drawingitem.get())) {
                g->setPickChildren(effectiveLayerMode(v.key) == SPGroup::LAYER);
            }
        }
    }
}

void SPGroup::translateChildItems(Geom::Translate const &tr)
{
    if (hasChildren()) {
        for (auto &o: children) {
            if (auto item = cast<SPItem>(&o)) {
                item->move_rel(tr);
            }
        }
    }
}

// Recursively (or not) scale child items around a point
void SPGroup::scaleChildItemsRec(Geom::Scale const &sc, Geom::Point const &p, bool noRecurse)
{
    if ( hasChildren() ) {
        for (auto& o: children) {
            if ( auto defs = cast<SPDefs>(&o) ) { // select symbols from defs, ignore clips, masks, patterns
                for (auto& defschild: defs->children) {
                    auto defsgroup = cast<SPGroup>(&defschild);
                    if (defsgroup)
                        defsgroup->scaleChildItemsRec(sc, p, false);
                }
            } else if (auto item = cast<SPItem>(&o)) {
                auto group = cast<SPGroup>(item);
                if (group && !is<SPBox3D>(item)) {
                    /* Using recursion breaks clipping because transforms are applied
                       in coordinates for draws but nothing in defs is changed
                       instead change the transform on the entire group, and the transform
                       is applied after any references to clipping paths.  However NOT using
                       recursion apparently breaks as of r13544 other parts of Inkscape
                       involved with showing/modifying units.  So offer both for use
                       in different contexts.
                    */
                    if(noRecurse) {
                        // used for EMF import
                        Geom::Translate const s(p);
                        Geom::Affine final = s.inverse() * sc * s;
                        Geom::Affine tAff = item->i2dt_affine() * final;
                        item->set_i2d_affine(tAff);
                        tAff = item->transform;
                        // Eliminate common rounding error affecting EMF/WMF input.
                        // When the rounding error persists it converts the simple
                        //    transform=scale() to transform=matrix().
                        if(std::abs(tAff[4]) < 1.0e-5 && std::abs(tAff[5]) < 1.0e-5){
                           tAff[4] = 0.0;
                           tAff[5] = 0.0;
                        }
                        item->doWriteTransform(tAff, nullptr, true);
                    } else {
                        // used for other import
                        SPItem *sub_item = nullptr;
                        if (item->getClipObject()) {
                            sub_item = cast<SPItem>(item->getClipObject()->firstChild());
                        }
                        if (sub_item != nullptr) {
                            sub_item->doWriteTransform(sub_item->transform*sc, nullptr, true);
                        }
                        sub_item = nullptr;
                        if (item->getMaskObject()) {
                            sub_item = cast<SPItem>(item->getMaskObject()->firstChild());
                        }
                        if (sub_item != nullptr) {
                            sub_item->doWriteTransform(sub_item->transform*sc, nullptr, true);
                        }
                        item->doWriteTransform(sc.inverse()*item->transform*sc, nullptr, true);
                        group->scaleChildItemsRec(sc, p, false);
                    }
                } else {
//                    Geom::OptRect bbox = item->desktopVisualBounds();
//                    if (bbox) {  // test not needed, this was causing a failure to scale <circle> and <rect> in the clipboard, see LP Bug 1365451
                        // Scale item
                        Geom::Translate const s(p);
                        Geom::Affine final = s.inverse() * sc * s;

                        gchar const *conn_type = nullptr;
                        auto text_item = cast<SPText>(item);
                        bool is_text_path = text_item && text_item->firstChild() && cast<SPTextPath>(text_item->firstChild());
                        if (is_text_path) {
                            text_item->optimizeTextpathText();
                        } else {
                            auto flowText = cast<SPFlowtext>(item);
                            if (flowText) {
                                flowText->optimizeScaledText();
                            } else {
                                auto box = cast<SPBox3D>(item);
                                if (box) {
                                    // Force recalculation from perspective
                                    box->position_set();
                                } else if (item->getAttribute("inkscape:connector-type") != nullptr
                                           && (item->getAttribute("inkscape:connection-start") == nullptr
                                               || item->getAttribute("inkscape:connection-end") == nullptr)) {
                                    // Remove and store connector type for transform if disconnected
                                    conn_type = item->getAttribute("inkscape:connector-type");
                                    item->removeAttribute("inkscape:connector-type");
                                }
                            }
                        }

                        if (is_text_path && !item->transform.isIdentity()) {
                            // Save and reset current transform
                            Geom::Affine tmp(item->transform);
                            item->transform = Geom::Affine();
                            // Apply scale
                            item->set_i2d_affine(item->i2dt_affine() * sc);
                            item->doWriteTransform(item->transform, nullptr, true);
                            // Scale translation and restore original transform
                            tmp[4] *= sc[0];
                            tmp[5] *= sc[1];
                            item->doWriteTransform(tmp, nullptr, true);
                        } else if (is<SPUse>(item)) {
                            // calculate the matrix we need to apply to the clone
                            // to cancel its induced transform from its original
                            Geom::Affine move = final.inverse() * item->transform * final;
                            item->doWriteTransform(move, &move, true);
                        } else {
                            item->doWriteTransform(item->transform*sc, nullptr, true);
                        }

                        if (conn_type != nullptr) {
                            item->setAttribute("inkscape:connector-type", conn_type);
                        }

                        if (item->isCenterSet() && !(final.isTranslation() || final.isIdentity())) {
                            item->scaleCenter(sc); // All coordinates have been scaled, so also the center must be scaled
                            item->updateRepr();
                        }
//                    }
                }
            }
        }
    }
}

gint SPGroup::getItemCount() const {
    gint len = 0;
    for (auto& child: children) {
        if (is<SPItem>(&child)) {
            len++;
        }
    }

    return len;
}

void SPGroup::_showChildren (Inkscape::Drawing &drawing, Inkscape::DrawingItem *ai, unsigned int key, unsigned int flags) {
    Inkscape::DrawingItem *ac = nullptr;
    std::vector<SPObject*> l=this->childList(false, SPObject::ActionShow);
    for(auto o : l){
        auto child = cast<SPItem>(o);
        if (child) {
            ac = child->invoke_show (drawing, key, flags);
            if (ac) {
                ai->appendChild(ac);
            }
        }
    }
}

std::vector<SPItem*> SPGroup::get_expanded(std::vector<SPItem*> const &items)
{
    std::vector<SPItem*> result;

    for (auto item : items) {
        if (auto group = cast<SPGroup>(item)) {
            auto sub = get_expanded(group->item_list());
            result.insert(result.end(), sub.begin(), sub.end());
        } else {
            result.emplace_back(item);
        }
    }

    return result;
}

void SPGroup::update_patheffect(bool write) {
#ifdef GROUP_VERBOSE
    g_message("sp_group_update_patheffect: %p\n", lpeitem);
#endif
    for (auto sub_item : item_list()) {
        if (sub_item) {
            // don't need lpe version < 1 (issue only reply on lower LPE on nested LPEs
            // this doesn't happen because it's done at very first stage
            // we need to be sure performed to inform lpe original bounds ok,
            // if not original_bbox function fail on update groups
            auto sub_shape = cast<SPShape>(sub_item);
            if (sub_shape && sub_shape->hasPathEffectRecursive()) {
                sub_shape->bbox_vis_cache_is_valid = false;
                sub_shape->bbox_geom_cache_is_valid = false;
            }
            auto lpe_item = cast<SPLPEItem>(sub_item);
            if (lpe_item) {
                lpe_item->update_patheffect(write);
            }
        }
    }

    // avoid update lpe in each selection
    // must be set also to non effect items (satellites or parents)
    lpe_initialized = true;
    if (hasPathEffect() && pathEffectsEnabled()) {
        if (!document->getRoot()->inkscape_version.isInsideRangeExclusive({0, 1}, {0, 92})) {
            resetClipPathAndMaskLPE();
        }
        PathEffectList path_effect_list(*this->path_effect_list);
        for (auto &lperef : path_effect_list) {
            LivePathEffectObject *lpeobj = lperef->lpeobject;
            if (lpeobj) {
                Inkscape::LivePathEffect::Effect *lpe = lpeobj->get_lpe();
                if (lpe && lpe->isVisible()) {
                    lpeobj->get_lpe()->doBeforeEffect_impl(this);
                    sp_group_perform_patheffect(this, this, lpe, write);
                    lpeobj->get_lpe()->doAfterEffect_impl(this, nullptr);
                }
            }
        }
    }
}

static void
sp_group_perform_patheffect(SPGroup *group, SPGroup *top_group, Inkscape::LivePathEffect::Effect *lpe, bool write)
{
    std::vector<SPItem*> const item_list = group->item_list();
    for (auto sub_item : item_list) {

        if (!Inkscape::LivePathEffect::can_have_lpe(sub_item)) {
            // Reject things with curves that can't have path
            // effects: polygons, paths with offsets, etc.
            continue;
        }

        auto sub_group = cast<SPGroup>(sub_item);
        if (sub_group) {
            sp_group_perform_patheffect(sub_group, top_group, lpe, write);
        } else {
            auto sub_shape = cast<SPShape>(sub_item);
            //auto sub_path = cast<SPPath>(sub_item);
            auto clipmaskto = sub_item;
            if (clipmaskto) {
                top_group->applyToClipPath(clipmaskto, lpe);
                top_group->applyToMask(clipmaskto, lpe);
            }
            if (sub_shape) {
                bool success = false;
                // only run LPEs when the shape has a curve defined
                if (sub_shape->curve()) {
                    auto c = *sub_shape->curve();
                    lpe->pathvector_before_effect = c;
                    c *= i2anc_affine(sub_shape, top_group);
                    sub_shape->setCurveInsync(c);
                    success = top_group->performOnePathEffect(c, sub_shape, lpe);
                    c *= i2anc_affine(sub_shape, top_group).inverse();
                    Inkscape::XML::Node *repr = sub_item->getRepr();
                    if (success) {
                        sub_shape->setCurveInsync(c);
                        if (lpe->lpeversion.param_getSVGValue() != "0") { // we are on 1 or up
                            sub_shape->bbox_vis_cache_is_valid = false;
                            sub_shape->bbox_geom_cache_is_valid = false;
                        }
                        lpe->pathvector_after_effect = c;
                        if (write) {
                            repr->setAttribute("d", sp_svg_write_path(lpe->pathvector_after_effect));
#ifdef GROUP_VERBOSE
                            g_message("sp_group_perform_patheffect writes 'd' attribute");
#endif
                        }
                    }
                }
            }
        }
    }
    auto clipmaskto = group;
    if (clipmaskto) {
        top_group->applyToClipPath(clipmaskto, lpe);
        top_group->applyToMask(clipmaskto, lpe);
    }
}

// This is really a UI feature and doesn't belong here.

// A list of default highlight colours to use when one isn't set.
std::vector<guint32> default_highlights;

/**
 * Should we choose from the layer default highlight colors, or go with the base SPItem
 * inheritance/default?
 */
bool SPGroup::_should_choose_layer_highlight() const
{
    // Parent must not be a layer (root, or similar) and this group must also be a layer
    return !SP_IS_LAYER(parent) && isLayer() && !default_highlights.empty();
}

/**
 * Generate a highlight colour if one isn't set and return it.
 */
Inkscape::Colors::Color SPGroup::highlight_color() const {
    if (!isHighlightSet() && _should_choose_layer_highlight()) {
        char const * oid = defaultLabel();
        if (oid && *oid) {
            // Color based on the last few bits of the label or object id.
            return Inkscape::Colors::Color(default_highlights[oid[(strlen(oid) - 1)] % default_highlights.size()]);
        }
    }
    return SPItem::highlight_color();
}

/**
 * Ensure that the current highlight color of the group is set (not defaulted) to its current
 * color, so that any change to the group (like changing its layer mode, or renaming a layer)
 * doesn't change the color by changing the default.
 * @param group The group or layer to preserve the highlight for.
 */
void SPGroup::preserveGroupHighlight()
{
    if (!isHighlightSet()) {
        setHighlight(highlight_color());
    }
}

/**
 * Same as preserveGroupHighlight, but only set the highlight if this is a layer, and not any other
 * type of group.
 * @param group The group or layer to preserve the highlight for.
 */
void SPGroup::preserveLayerHighlight()
{
    if (!isHighlightSet() && _should_choose_layer_highlight()) {
        preserveGroupHighlight();
    }
}

void set_default_highlight_colors(std::vector<guint32> colors) {
    std::swap(default_highlights, colors);
}

void SPGroup::removeTransformsRecursively(SPObject const *root)
{
    for (auto item : item_list()) {
        item->removeTransformsRecursively(root);
    }
    removeAttribute("transform");
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
