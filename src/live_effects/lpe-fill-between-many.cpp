// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "live_effects/lpe-fill-between-many.h"

#include "inkscape.h"
#include "selection.h"

#include "path/path-curve.h"
#include "object/sp-defs.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<Filllpemethod> FilllpemethodData[] = {
    { FLM_ORIGINALD, N_("Without LPEs"), "originald" }, 
    { FLM_BSPLINESPIRO, N_("With Spiro or BSpline"), "bsplinespiro" },
    { FLM_D, N_("With all LPEs"), "d" }
};
static const Util::EnumDataConverter<Filllpemethod> FLMConverter(FilllpemethodData, FLM_END);

LPEFillBetweenMany::LPEFillBetweenMany(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , linked_paths(_("Linked path:"), _("Paths from which to take the original path data"), "linkedpaths", &wr, this)
    , method(_("LPEs:"), _("Which LPEs of the linked paths should be considered"), "method", FLMConverter, &wr, this, FLM_BSPLINESPIRO)
    , join(_("Join subpaths"), _("Join subpaths"), "join", &wr, this, true)
    , close(_("Close"), _("Close path"), "close", &wr, this, true)
    , autoreverse(_("Autoreverse"), _("Autoreverse"), "autoreverse", &wr, this, true)
{
    registerParameter(&linked_paths);
    registerParameter(&method);
    registerParameter(&join);
    registerParameter(&close);
    registerParameter(&autoreverse);
    previous_method = FLM_END;
    linked_paths.setUpdating(true);
}

LPEFillBetweenMany::~LPEFillBetweenMany() = default;

void
LPEFillBetweenMany::doOnApply(SPLPEItem const* lpeitem)
{
    lpeversion.param_setValue("1.2", true);
}

bool 
LPEFillBetweenMany::doOnOpen(SPLPEItem const *lpeitem)
{
    if (!is_load || is_applied) {
        return false;
    }

    linked_paths.setUpdating(false);
    linked_paths.start_listening();
    linked_paths.connect_selection_changed();
    std::vector<SPLPEItem *> lpeitems = getCurrrentLPEItems();
    if (lpeitems.size() == 1) {
        sp_lpe_item = lpeitems[0];
        prevaffine = i2anc_affine(sp_lpe_item, sp_lpe_item->document->getRoot());
    }
    return false;
}

void 
LPEFillBetweenMany::doBeforeEffect (SPLPEItem const* lpeitem)
{
    legacytest = false;
    std::vector<SPLPEItem *> lpeitems = getCurrrentLPEItems();
    if (lpeitems.size() == 1) {
        sp_lpe_item = lpeitems[0];
    }
    if (!is_load) {
        transform_multiply_nested(i2anc_affine(sp_lpe_item, sp_lpe_item->document->getRoot()).inverse() * prevaffine);
        prevaffine = i2anc_affine(sp_lpe_item, sp_lpe_item->document->getRoot());
    } else {
        linked_paths.setUpdating(false);
        linked_paths.start_listening();
        linked_paths.connect_selection_changed();
    }
    Glib::ustring version = lpeversion.param_getSVGValue();
    if (version < "1.2") {
        legacytest = true;
    }
}

void 
LPEFillBetweenMany::transform_multiply_nested(Geom::Affine const &postmul)
{
    if (is_visible && sp_lpe_item->pathEffectsEnabled() && !isOnClipboard() && !postmul.isIdentity()) {
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        Inkscape::Selection *selection = nullptr;
        if (desktop) {
            selection = desktop->getSelection();
        }
        std::vector<SPLPEItem *> lpeitems = getCurrrentLPEItems();
        if (lpeitems.size() == 1) {
            sp_lpe_item = lpeitems[0];
        }
        for (auto & iter : linked_paths._vector) {
            SPItem *item;
            if (iter->ref.isAttached() && (( item = cast<SPItem>(iter->ref.getObject()) )) &&
                !iter->_pathvector.empty() && iter->visibled) {
                if (iter->_pathvector.front().closed() && linked_paths._vector.size() > 1) {
                    continue;
                }
                if (item->document->isSensitive() && selection && !selection->includes(item, true) && selection->includes(sp_lpe_item, true)) {
                    item->transform *= i2anc_affine(item->parent, item->document->getRoot());
                    item->transform *=  postmul.inverse();
                    item->transform *= i2anc_affine(item->parent, item->document->getRoot()).inverse();
                    item->doWriteTransform(item->transform, nullptr, false);
                    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                }
            }
        }
    }
}

void LPEFillBetweenMany::doEffect(Geom::PathVector &curve)
{
    if (previous_method != method) {
        if (method == FLM_BSPLINESPIRO) {
            linked_paths.allowOnlyBsplineSpiro(true);
            linked_paths.setFromOriginalD(false);
        } else if(method == FLM_ORIGINALD) {
            linked_paths.allowOnlyBsplineSpiro(false);
            linked_paths.setFromOriginalD(true);
        } else {
            linked_paths.allowOnlyBsplineSpiro(false);
            linked_paths.setFromOriginalD(false);
        }
        previous_method = method;
    }
    Geom::PathVector res_pathv;
    if (!autoreverse) {
        for (auto & iter : linked_paths._vector) {
            SPItem *item;
            if (iter->ref.isAttached() && (( item = cast<SPItem>(iter->ref.getObject()) )) &&
                !iter->_pathvector.empty() && iter->visibled) {
                for (auto linked_path : iter->_pathvector) {
                    if (iter->reversed) {
                        linked_path = linked_path.reversed();
                    }
                    linked_path *= item->getRelativeTransform(sp_lpe_item);
                    if (!res_pathv.empty() && join) {
                        if (!are_near(res_pathv.front().finalPoint(), linked_path.initialPoint(), 0.1)) {
                            res_pathv.front().appendNew<Geom::LineSegment>(linked_path.initialPoint());
                        } else {
                            linked_path.setInitial(res_pathv.front().finalPoint());
                        }
                        res_pathv.front().append(linked_path);
                    } else {
                        if (close && !join) {
                            linked_path.close();
                        }
                        res_pathv.push_back(linked_path);
                    }
                }
            }
        }
    } else {
        unsigned int counter = 0;
        Geom::Point current = Geom::Point();
        counter = 0;
        std::vector<unsigned int> done;
        for (auto & iter : linked_paths._vector) {
            SPItem *item;
            if (iter->ref.isAttached() && (( item = cast<SPItem>(iter->ref.getObject()) )) &&
                !iter->_pathvector.empty() && iter->visibled) {
                Geom::Path linked_path;
                if (iter->_pathvector.front().closed() && linked_paths._vector.size() > 1) {
                    counter++;
                    continue;
                }
                if (counter == 0) {
                    Geom::Path initial_path = iter->_pathvector.front();
                    if (!legacytest && iter->reversed) {
                        initial_path = initial_path.reversed();
                    }
                    done.push_back(0);
                    if (close && !join) {
                        initial_path.close();
                    }
                    initial_path *= item->getRelativeTransform(sp_lpe_item);
                    res_pathv.push_back(initial_path);
                    current = res_pathv.front().finalPoint();
                }
                Geom::Coord distance = Geom::infinity();
                unsigned int counter2 = 0;
                unsigned int added = 0;
                PathAndDirectionAndVisible *nearest = nullptr;
                for (auto & iter2 : linked_paths._vector) {
                    SPItem *item2;
                    if (iter2->ref.isAttached() && (( item2 = cast<SPItem>(iter2->ref.getObject()) )) &&
                        !iter2->_pathvector.empty() && iter2->visibled) {
                        if (item == item2 || std::find(done.begin(), done.end(), counter2) != done.end()) {
                            counter2++;
                            continue;
                        }
                        if (iter2->_pathvector.front().closed() && linked_paths._vector.size() > 1) {
                            counter2++;
                            continue;
                        }
                        Geom::Point start = iter2->_pathvector.initialPoint();
                        Geom::Point end = iter2->_pathvector.front().finalPoint();
                        if (!legacytest && iter2->reversed) {
                            std::swap(start,end);
                        }
                        if (!legacytest) {
                            current = res_pathv.finalPoint();
                        }
                        Geom::Coord distance_iter =
                            std::min(Geom::distance(current, end), Geom::distance(current, start));
                        if (distance > distance_iter) {
                            distance = distance_iter;
                            nearest = iter2;
                            added = counter2;
                        }
                        counter2++;
                    }
                }
                if (nearest != nullptr) {
                    done.push_back(added);
                    Geom::Point start = nearest->_pathvector.initialPoint();
                    Geom::Point end = nearest->_pathvector.front().finalPoint();
                    if (!legacytest && nearest->reversed) {
                        linked_path = iter->_pathvector.front().reversed();
                        std::swap(start,end);
                    } else {
                        linked_path = iter->_pathvector.front();
                    }
                    if (Geom::distance(current, end) > Geom::distance(current, start)) {
                        linked_path = nearest->_pathvector.front();
                    } else {
                        linked_path = nearest->_pathvector.front().reversed();
                    }
                    
                    if (legacytest) {
                        current = end;
                    }
                    SPItem *itemnear;
                    if (nearest->ref.isAttached() && (( itemnear = cast<SPItem>(nearest->ref.getObject()) ))) {
                        linked_path *= itemnear->getRelativeTransform(sp_lpe_item);
                    }
                    if (!res_pathv.empty() && join) {
                        if (!getSPDoc()->getRoot()->inkscape_version.isInsideRangeExclusive({0, 1}, {1, 1}) &&
                            Geom::distance(res_pathv.front().finalPoint(), linked_path.initialPoint()) >
                                Geom::distance(res_pathv.front().finalPoint(), linked_path.finalPoint())) {
                            linked_path = linked_path.reversed();
                        }
                        if (!are_near(res_pathv.front().finalPoint(), linked_path.initialPoint(), 0.1)) {
                            res_pathv.front().appendNew<Geom::LineSegment>(linked_path.initialPoint());
                        } else {
                            linked_path.setInitial(res_pathv.front().finalPoint());
                        }
                        res_pathv.front().append(linked_path);
                    } else {
                        if (close && !join) {
                            linked_path.close();
                        }
                        res_pathv.push_back(linked_path);
                    }
                }
                counter++;
            }
        }
    }
    if (!res_pathv.empty() && close) {
        res_pathv.front().close();
        res_pathv.front().snapEnds(0.1);
    }
    if (!res_pathv.empty()) {
        curve = std::move(res_pathv);
    }
}

} // namespace LivePathEffect
} /* namespace Inkscape */

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
