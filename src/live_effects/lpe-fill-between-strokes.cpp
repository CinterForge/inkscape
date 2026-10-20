// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "lpe-fill-between-strokes.h"

#include <glibmm/i18n.h>

#include "inkscape.h"
#include "selection.h"

#include "path/path-curve.h"
#include "object/sp-root.h"


namespace Inkscape {
namespace LivePathEffect {

LPEFillBetweenStrokes::LPEFillBetweenStrokes(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    linked_path(_("Linked path:"), _("Path from which to take the original path data"), "linkedpath", &wr, this),
    second_path(_("Second path:"), _("Second path from which to take the original path data"), "secondpath", &wr, this),
    reverse_second(_("Reverse Second"), _("Reverses the second path order"), "reversesecond", &wr, this),
    join(_("Join subpaths"), _("Join subpaths"), "join", &wr, this, true),
    close(_("Close"), _("Close path"), "close", &wr, this, true)
{
    registerParameter(&linked_path);
    registerParameter(&second_path);
    registerParameter(&reverse_second);
    registerParameter(&join);
    registerParameter(&close);
    linked_path.setUpdating(true);
    second_path.setUpdating(true);
}

LPEFillBetweenStrokes::~LPEFillBetweenStrokes() = default;

void
LPEFillBetweenStrokes::doOnApply(SPLPEItem const* lpeitem)
{
    lpeversion.param_setValue("1.2", true);
}

bool 
LPEFillBetweenStrokes::doOnOpen(SPLPEItem const *lpeitem)
{
    if (!is_load || is_applied) {
        return false;
    }
    linked_path.setUpdating(false);
    second_path.setUpdating(false);
    linked_path.start_listening(linked_path.getObject());
    linked_path.connect_selection_changed();
    second_path.start_listening(second_path.getObject());
    second_path.connect_selection_changed();
    std::vector<SPLPEItem *> lpeitems = getCurrrentLPEItems();
    if (lpeitems.size() == 1) {
        sp_lpe_item = lpeitems[0];
        prevaffine = i2anc_affine(sp_lpe_item, sp_lpe_item->document->getRoot());
    }
    if (auto item = linked_path.getObject()) {
        item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
    if (auto item = second_path.getObject()) {
        item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }
    return false;
}

void 
LPEFillBetweenStrokes::doBeforeEffect (SPLPEItem const* lpeitem)
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
        linked_path.setUpdating(false);
        second_path.setUpdating(false);
        linked_path.start_listening(linked_path.getObject());
        linked_path.connect_selection_changed();
        second_path.start_listening(second_path.getObject());
        second_path.connect_selection_changed();
        if (auto item = linked_path.getObject()) {
            item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        if (auto item = second_path.getObject()) {
            item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
    }
    Glib::ustring version = lpeversion.param_getSVGValue();
    if (version < "1.2") {
        legacytest = true;
    }
}
void 
LPEFillBetweenStrokes::transform_multiply_nested(Geom::Affine const &postmul)
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
        if (auto item = linked_path.getObject()) {
            if (item->document->isSensitive() && selection && !selection->includes(item, true) && selection->includes(sp_lpe_item, true)) {
                item->transform *= i2anc_affine(item->parent, item->document->getRoot());
                item->transform *=  postmul.inverse();
                item->transform *= i2anc_affine(item->parent, item->document->getRoot()).inverse();
                item->doWriteTransform(item->transform, nullptr, false);
                item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }
        if (auto item2 = second_path.getObject()) {
            if (item2->document->isSensitive() && selection && !selection->includes(item2, true) && selection->includes(sp_lpe_item, true)) {
                item2->transform *= i2anc_affine(item2->parent, item2->document->getRoot());
                item2->transform *=  postmul.inverse();
                item2->transform *= i2anc_affine(item2->parent, item2->document->getRoot()).inverse();
                item2->doWriteTransform(item2->transform, nullptr, false);
                item2->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            }
        }

    }
}

void LPEFillBetweenStrokes::doEffect(Geom::PathVector &curve)
{
    if ( linked_path.linksToPath() && second_path.linksToPath() && linked_path.getObject() && second_path.getObject() ) {
        SPItem * linked1 = linked_path.getObject();
        if (is_load) {
            linked1->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        Geom::PathVector linked_pathv = linked_path.get_pathvector();
        SPItem * linked2 = second_path.getObject();
        if (is_load) {
            linked2->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        Geom::PathVector second_pathv = second_path.get_pathvector();
        Geom::PathVector result_linked_pathv;
        Geom::PathVector result_second_pathv;
        linked_pathv *= linked1->getRelativeTransform(sp_lpe_item);
        second_pathv *= linked2->getRelativeTransform(sp_lpe_item);
        for (auto & iter : linked_pathv)
        {
            result_linked_pathv.push_back(iter);
        }

        for (auto & iter : second_pathv)
        {
            result_second_pathv.push_back(iter);
        }

        if ( !result_linked_pathv.empty() && !result_second_pathv.empty() && !result_linked_pathv.front().closed() ) {
            if (reverse_second.get_value()) {
                result_second_pathv.front() = result_second_pathv.front().reversed();
            }
            if (join) {
                if (!are_near(result_linked_pathv.front().finalPoint(), result_second_pathv.initialPoint(),0.1)) {
                    result_linked_pathv.front().appendNew<Geom::LineSegment>(result_second_pathv.initialPoint());
                } else {
                    result_second_pathv.front().setInitial(result_linked_pathv.front().finalPoint());
                }
                result_linked_pathv.front().append(result_second_pathv.front());
                if (close) {
                    result_linked_pathv.front().close();
                }
            } else {
                if (close) {
                    result_linked_pathv.front().close();
                    result_second_pathv.front().close();
                }
                result_linked_pathv.push_back(result_second_pathv.front());
            }
            curve = std::move(result_linked_pathv);
        } else if (!result_linked_pathv.empty()) {
            curve = std::move(result_linked_pathv);
        } else if (!result_second_pathv.empty()) {
            curve = std::move(result_second_pathv);
        }
    }
    else if ( linked_path.linksToPath() && linked_path.getObject() ) {
        SPItem *linked1 = linked_path.getObject();
        if (is_load) {
            linked1->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        Geom::PathVector linked_pathv = linked_path.get_pathvector();
        linked_pathv *= linked1->getRelativeTransform(sp_lpe_item);
        Geom::PathVector result_pathv;
        for (auto & iter : linked_pathv)
        {
            result_pathv.push_back(iter);
        }
        if ( !result_pathv.empty() ) {
            if (close) {
                result_pathv.front().close();
            }
            curve = std::move(result_pathv);
        }
    }
    else if ( second_path.linksToPath() && second_path.getObject() ) {
        SPItem *linked2 = second_path.getObject();
        if (is_load) {
            linked2->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        }
        Geom::PathVector second_pathv = second_path.get_pathvector();
        second_pathv *= linked2->getRelativeTransform(sp_lpe_item);
        Geom::PathVector result_pathv;
        for (auto & iter : second_pathv)
        {
            result_pathv.push_back(iter);
        }
        if ( !result_pathv.empty() ) {
            if (close) {
                result_pathv.front().close();
                result_pathv.front().snapEnds(0.1);
            }
            curve = std::move(result_pathv);
        }
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
