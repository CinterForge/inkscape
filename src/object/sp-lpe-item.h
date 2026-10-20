// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Base class for live path effect items
 */
/*
 * Authors:
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Bastien Bouclet <bgkweb@gmail.com>
 *
 * Copyright (C) 2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_LPE_ITEM_H_SEEN
#define SP_LPE_ITEM_H_SEEN

#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <sigc++/scoped_connection.h>
#include "sp-item.h"

class LivePathEffectObject;
class SPShape;
class SPDesktop;

namespace Inkscape {

namespace Display {
class TemporaryItem;
} // namespace Display

namespace LivePathEffect{
class LPEObjectReference;
class Effect;
} // namespace LivePathEffect

} // namespace Inkscape

using PathEffectSharedPtr = std::shared_ptr<Inkscape::LivePathEffect::LPEObjectReference>;
using PathEffectList = std::list<PathEffectSharedPtr>;

class SPLPEItem : public SPItem {
    // this list contains the connections for listening to lpeobject parameter changes
    std::list<sigc::scoped_connection> lpe_modified_connection_list;

public:
    SPLPEItem();
    ~SPLPEItem() override;

    int tag() const override { return tag_of<decltype(*this)>; }

    // TODO: Data members should be private whererever possible.

    int path_effects_enabled;
    // TODO: Avoid indirection/`new`:
    PathEffectList* path_effect_list;
    PathEffectSharedPtr current_path_effect;
    std::vector<Inkscape::Display::TemporaryItem*> lpe_helperpaths;

    void replacePathEffects( std::vector<LivePathEffectObject const *> const &old_lpeobjs,
                             std::vector<LivePathEffectObject const *> const &new_lpeobjs );


    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;

    void set(SPAttr key, char const* value) override;

    void update(SPCtx* ctx, unsigned int flags) override;
    void modified(unsigned int flags) override;
    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void remove_child(Inkscape::XML::Node* child) override;

    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

    virtual void update_patheffect(bool write);
    bool optimizeTransforms();
    void notifyTransform(Geom::Affine const &postmul);
    bool performPathEffect(Geom::PathVector &curve, SPShape *current, bool is_clip_or_mask = false);
    bool performOnePathEffect(Geom::PathVector &curve, SPShape *current, Inkscape::LivePathEffect::Effect *lpe, bool is_clip_or_mask = false);
    bool pathEffectsEnabled() const;
    bool hasPathEffect() const;
    bool hasPathEffectOfType(int const type, bool is_ready = true) const;
    bool hasPathEffectOfTypeRecursive(int const type, bool is_ready = true) const;
    bool hasPathEffectRecursive() const;
    SPLPEItem const * getTopPathEffect() const;
    bool hasPathEffectOnClipOrMask(SPLPEItem * shape) const;
    bool hasPathEffectOnClipOrMaskRecursive(SPLPEItem * shape) const;
    std::size_t getLPEIndex(Inkscape::LivePathEffect::Effect *lpe) const;
    std::size_t countLPEOfType(int const type, bool inc_hidden = true, bool is_ready = true) const;
    std::size_t getLPEReferenceIndex(PathEffectSharedPtr const &lperef) const;
    Inkscape::LivePathEffect::Effect *getFirstPathEffectOfType(int type);
    Inkscape::LivePathEffect::Effect const *getFirstPathEffectOfType(int type) const;
    std::vector<Inkscape::LivePathEffect::Effect *> getPathEffectsOfType(int type);
    std::vector<Inkscape::LivePathEffect::Effect const *> getPathEffectsOfType(int type) const;
    std::vector<Inkscape::LivePathEffect::Effect *> getPathEffects();
    std::vector<Inkscape::LivePathEffect::Effect const *> getPathEffects() const;
    std::vector<SPObject *> get_satellites(bool force = true, bool recursive = false, bool onchilds = false);
    bool isOnClipboard();
    bool isOnSymbol() const;
    bool onsymbol = false;
    bool hasBrokenPathEffect() const;
    bool lpe_initialized = false;
    PathEffectList getEffectList();
    PathEffectList const getEffectList() const;

    void duplicateCurrentPathEffect();
    void downCurrentPathEffect();
    void upCurrentPathEffect();
    void removePathEffect(Inkscape::LivePathEffect::Effect* lpe, bool keep_paths);
    void movePathEffect(gint origin, gint dest, bool select_moved = false);
    SPLPEItem * flattenCurrentPathEffect();
    PathEffectSharedPtr getCurrentLPEReference();
    Inkscape::LivePathEffect::Effect* getCurrentLPE();
    PathEffectSharedPtr getPrevLPEReference(PathEffectSharedPtr const &lperef);
    Inkscape::LivePathEffect::Effect* getPrevLPE(Inkscape::LivePathEffect::Effect* lpe);
    PathEffectSharedPtr getNextLPEReference(PathEffectSharedPtr const &lperef);
    Inkscape::LivePathEffect::Effect* getNextLPE(Inkscape::LivePathEffect::Effect* lpe);
    PathEffectSharedPtr getLastLPEReference();
    Inkscape::LivePathEffect::Effect* getLastLPE();
    bool setCurrentPathEffect(PathEffectSharedPtr const &lperef);
    bool setCurrentPathEffect(LivePathEffectObject const *lpeobj);
    SPLPEItem * removeCurrentPathEffect(bool keep_paths);
    SPLPEItem * removeAllPathEffects(bool keep_paths, bool recursive = false);
    void addPathEffect(std::string value, bool reset);
    void addPathEffect(LivePathEffectObject * new_lpeobj);
    void resetClipPathAndMaskLPE(bool fromrecurse = false);
    void applyToMask(SPItem* to, Inkscape::LivePathEffect::Effect *lpe = nullptr);
    void applyToClipPath(SPItem* to, Inkscape::LivePathEffect::Effect *lpe = nullptr);
    void applyToClipPathOrMask(SPItem * clip_mask, SPItem* to, Inkscape::LivePathEffect::Effect *lpe = nullptr);
    bool forkPathEffectsIfNecessary(unsigned int nr_of_allowed_users = 1, bool recursive = true, bool force = false);
    void editNextParamOncanvas(SPDesktop *dt);
    void update_satellites(bool recursive = true);
};
void sp_lpe_item_update_patheffect (SPLPEItem *lpeitem, bool wholetree, bool write, bool with_satellites = false); // careful, class already has method with *very* similar name!
void sp_lpe_item_enable_path_effects(SPLPEItem *lpeitem, bool enable);

#endif /* !SP_LPE_ITEM_H_SEEN */

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
