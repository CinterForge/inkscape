// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-powerclip.h"

#include <glibmm/i18n.h>

#include <2geom/intersection-graph.h>
#include <2geom/path-intersection.h>

#include "preferences.h"
#include "selection.h"
#include "style.h"

#include "inkscape.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "object/sp-clippath.h"
#include "object/sp-defs.h"
#include "object/sp-item-group.h"
#include "object/sp-item.h"
#include "object/sp-shape.h"
#include "object/sp-use.h"
#include "svg/svg.h"

namespace Inkscape {
namespace LivePathEffect {

LPEPowerClip::LPEPowerClip(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , hide_clip(_("Hide clip"), _("Hide clip"), "hide_clip", &wr, this, false)
    , inverse(_("Inverse clip"), _("Inverse clip"), "inverse", &wr, this, true)
    , flatten(_("Flatten clip"), _("Flatten clip, see fill rule once convert to paths"), "flatten", &wr, this, false)
    , message(
          _("Info Box"), _("Important messages"), "message", &wr, this,
          _("Use fill-rule evenodd on <b>fill and stroke</b> dialog if no flatten result after convert clip to paths."))
{
    registerParameter(&inverse);
    registerParameter(&flatten);
    registerParameter(&hide_clip);
    registerParameter(&message);
    message.param_set_min_height(55);
    message.write_to_SVG(); // resert old legacy uneeded data
    _updating = false;
    _legacy = false;
    // legazy fix between 0.92.4 launch and 1.0beta1
    if (this->getRepr()->attribute("is_inverse")) {
        this->getRepr()->removeAttribute("is_inverse");
        _legacy = true;
    }
}

LPEPowerClip::~LPEPowerClip() = default;

Geom::Path sp_bbox_without_clip(SPLPEItem *lpeitem)
{
    Geom::OptRect bbox = lpeitem->visualBounds(Geom::identity(), true, false, true);
    if (bbox) {
        (*bbox).expandBy(5);
        return Geom::Path(*bbox);
    }
    return Geom::Path();
}

Geom::PathVector sp_get_recursive_pathvector(SPLPEItem *item, Geom::PathVector res, bool dir, bool inverse)
{
    auto group = cast<SPGroup>(item);
    if (group) {
        std::vector<SPItem *> item_list = group->item_list();
        for (auto child : item_list) {
            if (child) {
                auto childitem = cast<SPLPEItem>(child);
                if (childitem) {
                    res = sp_get_recursive_pathvector(childitem, res, dir, inverse);
                }
            }
        }
    }
    auto shape = cast<SPShape>(item);
    if (shape && shape->curve()) {
        for (auto path : *shape->curve()) {
            if (!path.empty()) {
                bool pathdir = Geom::path_direction(path);
                if (pathdir == dir && inverse) {
                    path = path.reversed();
                }
                res.push_back(path * shape->i2dt_affine());
            }
        }
    }
    return res;
}

Geom::PathVector LPEPowerClip::getClipPathvector()
{
    Geom::PathVector res;
    Geom::PathVector res_hlp;
    if (!sp_lpe_item) {
        return res;
    }

    SPObject *clip_path = sp_lpe_item->getClipObject();
    if (clip_path) {
        std::vector<SPObject*> clip_path_list = clip_path->childList(true);
        clip_path_list.pop_back();
        if (clip_path_list.size()) {
            for (auto clip : clip_path_list) {
                auto childitem = cast<SPLPEItem>(clip);
                if (childitem) {
                    res_hlp = sp_get_recursive_pathvector(childitem, res_hlp, false, inverse);
                    if (is_load && _legacy) {
                        childitem->doWriteTransform(Geom::Translate(0, -999999));
                    }
                    if (!childitem->style || !childitem->style->display.set ||
                        childitem->style->display.value != SP_CSS_DISPLAY_NONE) {
                        childitem->setAttribute("display", "none");
                        childitem->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
                    }
                }
            }
            if (is_load && _legacy) {
                res_hlp *= Geom::Translate(0, -999999);
                _legacy = false;
            }
        }
    }
    Geom::Path bbox = sp_bbox_without_clip(sp_lpe_item);
    if (hide_clip) {
        return bbox;
    }
    if (inverse && isVisible()) {
        res.push_back(bbox);
    }
    for (auto path : res_hlp) {
        res.push_back(path);
    }
    return res;
}

void LPEPowerClip::add()
{
    SPDocument *document = getSPDoc();
    if (!document || !sp_lpe_item) {
        return;
    }
    SPObject *clip_path = sp_lpe_item->getClipObject();
    SPObject *elemref = NULL;
    if (clip_path) {
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *parent = clip_path->getRepr();
        auto childitems = clip_path->childList(true);
        auto childitem = cast<SPLPEItem>(childitems.empty() ? nullptr : childitems.back());
        if (childitem) {
           if (const gchar *powerclip = childitem->getRepr()->attribute("class")) {
                if (!strcmp(powerclip, "powerclip")) {
                    Glib::ustring newclip = Glib::ustring("clipath_") + getId();
                    Glib::ustring uri = Glib::ustring("url(#") + newclip + Glib::ustring(")");
                    parent = clip_path->getRepr()->duplicate(xml_doc);
                    parent->setAttribute("id", newclip);
                    clip_path = document->getDefs()->appendChildRepr(parent);
                    Inkscape::GC::release(parent);
                    sp_lpe_item->setAttribute("clip-path", uri);
                    auto childitems2 = clip_path->childList(true);
                    if (auto childitemdel = cast<SPLPEItem>(childitems2.empty() ? nullptr : childitems2.back())) {
                        childitemdel->setAttribute("id", getId());
                        return;
                    }
                }
            }
        }
        Inkscape::XML::Node *clip_path_node = xml_doc->createElement("svg:path");
        parent->appendChild(clip_path_node);
        Inkscape::GC::release(clip_path_node);
        elemref = document->getObjectByRepr(clip_path_node);
        if (elemref) {
            if (childitem) {
                elemref->setAttribute("style", childitem->getAttribute("style"));
            } else {
                elemref->setAttribute("style", "fill-rule:evenodd");
            }        
            elemref->setAttribute("class", "powerclip");
            elemref->setAttribute("id", getId());
            elemref->setAttribute("d", sp_svg_write_path(getClipPathvector()));
        } else {
            sp_lpe_item->removeCurrentPathEffect(false);
        }
    } else {
        sp_lpe_item->removeCurrentPathEffect(false);
    }
}

Glib::ustring LPEPowerClip::getId() { return Glib::ustring("lpe_") + Glib::ustring(getLPEObj()->getId()); }

void LPEPowerClip::upd()
{
    SPDocument *document = getSPDoc();
    if (!document || !sp_lpe_item) {
        return;
    }
    SPObject *elemref = document->getObjectById(getId().c_str());
    if (elemref && sp_lpe_item) {
        elemref->setAttribute("d", sp_svg_write_path(getClipPathvector()));
        elemref->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    } else {
        add();
    }
}


void LPEPowerClip::doBeforeEffect(SPLPEItem const *lpeitem)
{
    if (!_updating) {
        upd();
    }
}

void 
LPEPowerClip::doOnRemove (SPLPEItem const* /*lpeitem*/)
{
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (keep_paths || prefs->getBool("/options/onungroup", false)) {
        SPObject *clip_path = sp_lpe_item->getClipObject();
        if (clip_path) {
            auto childitem = cast<SPLPEItem>(clip_path->childList(true).front());
            childitem->deleteObject();
        }
        return;
    }
    _updating = true;
    SPObject *elemref = document->getObjectById(getId().c_str());
    if (elemref) {
        elemref->deleteObject();
    }
    SPObject *clip_path = sp_lpe_item->getClipObject();
    if (clip_path) {
        std::vector<SPObject *> clip_path_list = clip_path->childList(true);
        for (auto clip : clip_path_list) {
            auto childitem = cast<SPLPEItem>(clip);
            if (childitem) {
                if (!childitem->style || childitem->style->display.set ||
                    childitem->style->display.value == SP_CSS_DISPLAY_NONE) {
                    childitem->setAttribute("display", "block");
                    childitem->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
                }
            }
        }
    }
}

Geom::PathVector
LPEPowerClip::doEffect_path(Geom::PathVector const & path_in){
    Geom::PathVector path_out = path_in;
    if (flatten) {
        Geom::PathVector c_pv = getClipPathvector();
        std::unique_ptr<Geom::PathIntersectionGraph> pig(new Geom::PathIntersectionGraph(c_pv, path_out));
        if (pig && !c_pv.empty() && !path_out.empty()) {
            path_out = pig->getIntersection();
        }
    }
    return path_out;
}

void LPEPowerClip::doOnVisibilityToggled(SPLPEItem const *lpeitem) { upd(); }

void sp_remove_powerclip(Inkscape::Selection *sel)
{
    if (!sel->isEmpty()) {
        for (auto lpeitem : sel->objects_of_type<SPLPEItem>() | std::views::reverse) {
            if (lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled()) {
                PathEffectList path_effect_list(*lpeitem->path_effect_list);
                for (auto &lperef : path_effect_list) {
                    LivePathEffectObject *lpeobj = lperef->lpeobject;
                    if (!lpeobj) {
                        /** \todo Investigate the cause of this.
                         * For example, this happens when copy pasting an object with LPE applied. Probably because
                         * the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                         */
                        g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                        return;
                    }
                    if (LPETypeConverter.get_key(lpeobj->effecttype) == "powerclip") {
                        lpeitem->setCurrentPathEffect(lperef);
                        lpeitem->removeCurrentPathEffect(false);
                        break;
                    }
                }
            }
        }
    }
}

void sp_inverse_powerclip(Inkscape::Selection *sel) {
    if (!sel->isEmpty()) {
        for (auto lpeitem : sel->objects_of_type<SPLPEItem>() | std::views::reverse) {
            if (auto clip_path = lpeitem->getClipObject()) {
                for (auto iter : clip_path->childList(true)) {
                    if (is<SPUse>(iter)) {
                        g_warning("We can`t add inverse clip on clones");
                        return;
                    }
                }
                Effect::createAndApply(POWERCLIP, SP_ACTIVE_DOCUMENT, lpeitem);
                if (auto lpe = lpeitem->getCurrentLPE()) {
                    lpe->getRepr()->setAttribute("inverse", "true");
                }
            }
        }
    }
}

} // namespace LivePathEffect
} // namespace Inkscape

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
