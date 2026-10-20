// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "lpe-powermask.h"

#include <glibmm/i18n.h>

#include "inkscape.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "object/sp-defs.h"
#include "object/sp-mask.h"
#include "object/uri.h"
#include "selection.h"
#include "svg/svg.h"

namespace Inkscape {
namespace LivePathEffect {

LPEPowerMask::LPEPowerMask(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , invert(_("Invert mask"), _("Invert mask"), "invert", &wr, this, true)
    , hide_mask(_("Hide mask"), _("Hide mask"), "hide_mask", &wr, this, false)
    , background(_("Add background to mask"), _("Add background to mask"), "background", &wr, this, true)
    , background_color(_("Background color and opacity"), _("Set color and opacity of the background"),
                       "background_color", &wr, this, Colors::Color(0xffffffff))
{
    // Register parameters (order matters for UI)
    registerParameter(&invert);
    registerParameter(&hide_mask);
    registerParameter(&background);
    registerParameter(&background_color);
}

LPEPowerMask::~LPEPowerMask() = default;

Glib::ustring LPEPowerMask::getId() const
{
    return Glib::ustring("mask-powermask-") + getLPEObj()->getId();
}

void LPEPowerMask::doOnApply(SPLPEItem const *lpeitem)
{
    // Update the mask id
    auto const mask = sp_lpe_item->getMaskObject();
    if (!mask) {
        return;
    }

    auto const new_mask_id = getId();
    mask->setAttribute("id", new_mask_id);

    // Update the mask uri
    auto const new_uri = "url(#" + new_mask_id + ")";
    sp_lpe_item->setAttribute("mask", new_uri);
}

void LPEPowerMask::doBeforeEffect(SPLPEItem const *lpeitem)
{
    // Handle the hide_mask and visibility toggles
    if (!update_mask_visibility(lpeitem)) {
        return;
    }

    // Make changes to mask if necessary
    update_mask_box();

    // Prepare the filter
    auto const filter_uri = prepare_color_inversion_filter(lpeitem);

    // Apply filter based on current parameters
    handle_inverse_filter(filter_uri);
}

void LPEPowerMask::doAfterEffect(SPLPEItem const *lpeitem, Geom::PathVector * /*curve*/) {}

void LPEPowerMask::doOnVisibilityToggled(SPLPEItem const *lpeitem)
{
    update_mask_visibility(lpeitem);
}

void LPEPowerMask::doEffect(Geom::PathVector &curve) {}

void LPEPowerMask::doOnRemove(SPLPEItem const *lpeitem)
{
    auto const document = getSPDoc();
    if (!document) {
        return;
    }

    // 1. Remove the filter
    auto const mask_id = getId();
    auto const filter_id = mask_id + "_inverse";

    if (auto const element_ref = document->getObjectById(filter_id)) {
        element_ref->deleteObject(true);
    }

    // 2. Remove the Background box
    auto const box_id = mask_id + "_box";

    if (auto const element_ref = document->getObjectById(box_id)) {
        element_ref->deleteObject(true);
    }
}

void sp_inverse_powermask(Inkscape::Selection *sel)
{
    if (!sel->isEmpty()) {
        SPDocument *document = SP_ACTIVE_DOCUMENT;
        if (!document) {
            return;
        }
        for (auto lpeitem : sel->objects_of_type<SPLPEItem>() | std::views::reverse) {
            if (lpeitem->getMaskObject()) {
                Effect::createAndApply(POWERMASK, SP_ACTIVE_DOCUMENT, lpeitem);
            }
        }
    }
}

void sp_remove_powermask(Inkscape::Selection *sel)
{
    if (sel->isEmpty()) {
        return;
    }

    for (auto lpeitem : sel->objects_of_type<SPLPEItem>() | std::views::reverse) {
        if (!(lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled())) {
            continue;
        }

        for (auto &lperef : *lpeitem->path_effect_list) {
            LivePathEffectObject *lpeobj = lperef->lpeobject;
            if (!lpeobj) {
                /**
                 * TODO: Investigate the cause of this.
                 * For example, this happens when copy pasting an object with LPE applied. Probably because
                 * the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                 */
                g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                return;
            }

            if (LPETypeConverter.get_key(lpeobj->effecttype) == "powermask") {
                lpeitem->removeCurrentPathEffect(false);
                break;
            }
        }
    }
}

bool LPEPowerMask::update_mask_visibility(SPLPEItem const *lpeitem)
{
    auto attached_uri = sp_lpe_item->getMaskRef().getURI();
    auto attached_uri_str = attached_uri ? attached_uri->str() : std::string();

    sp_lpe_item->getMaskRef().detach();

    // Prepare the bounding box
    Geom::OptRect const bbox = lpeitem->visualBounds();
    if (!bbox) {
        return false;
    }

    Geom::Rect const bbox_rect = (*bbox);
    mask_box_path.clear();
    mask_box_path = Geom::Path(bbox_rect);

    if (!hide_mask && is_visible) {
        return sp_lpe_item->getMaskRef().try_attach(attached_uri_str.c_str());
    }

    return false;
}

void LPEPowerMask::handle_inverse_filter(Glib::ustring const &filter_uri) const
{
    // Mask updates
    auto const mask = sp_lpe_item->getMaskObject();
    if (!mask) {
        return;
    }

    for (auto const mask_child : mask->childList(true)) {
        auto const mask_data = cast<SPItem>(mask_child);

        // Parse existing style attribute into CSS object
        auto const new_css = sp_repr_css_attr_new();
        if (auto const style_attr = mask_child->getAttribute("style")) {
            sp_repr_css_attr_add_from_string(new_css, style_attr);
        }

        // Check for either empty filter or the inversion filter we defined earlier
        Glib::ustring current_filter = sp_repr_css_property(new_css, "filter", "");
        if (current_filter.empty() || current_filter == filter_uri) {
            // Apply the inversion filter only if the "invert" parameter is checked
            // and the lpe object is visible. Otherwise, remove the inversion filter
            if (invert && is_visible) {
                sp_repr_css_set_property(new_css, "filter", filter_uri.c_str());
            } else {
                sp_repr_css_set_property(new_css, "filter", nullptr);
            }

            // Writ the updated CSS back to the mask child
            Glib::ustring css_str;
            sp_repr_css_write_string(new_css, css_str);
            mask_data->setAttribute("style", css_str);
        }
    }
}

Glib::ustring LPEPowerMask::prepare_color_inversion_filter(SPLPEItem const *lpeitem)
{
    auto const document = getSPDoc();
    if (!document) {
        return "";
    }

    auto const filter_id = getId() + "_inverse";
    auto const filter_uri = "url(#" + filter_id + ")";

    if (document->getObjectById(filter_id)) {
        return filter_uri;
    }

    // Create only if the filter is not present
    auto const xml_doc = document->getReprDoc();
    auto const filter = xml_doc->createElement("svg:filter");
    filter->setAttribute("id", filter_id);
    auto const filter_label = "filter" + getId();
    filter->setAttribute("inkscape:label", filter_label);

    // Create CSS for the filter
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property(css, "color-interpolation-filters", "sRGB");
    sp_repr_css_change(filter, css, "style");
    sp_repr_css_attr_unref(css);

    // Transformation matrix to normalize the color space
    auto const primitive1 = xml_doc->createElement("svg:feColorMatrix");
    auto const primitive1_id = getId() + "_primitive1";
    primitive1->setAttribute("id", primitive1_id);
    primitive1->setAttribute("values", "1");
    primitive1->setAttribute("type", "saturate");
    primitive1->setAttribute("result", "fbSourceGraphic");

    // Transformation matrix to invert RGB
    auto const primitive2 = xml_doc->createElement("svg:feColorMatrix");
    auto const primitive2_id = getId() + "_primitive2";
    primitive2->setAttribute("id", primitive2_id);
    primitive2->setAttribute("in", "fbSourceGraphic");
    auto const rgb_inversion_transformation_matrix = "-1 0 0 0 1 "
                                                     "0 -1 0 0 1 "
                                                     "0 0 -1 0 1 "
                                                     "0 0 0 1 0";
    primitive2->setAttribute("values", rgb_inversion_transformation_matrix);

    // Add the filter to the defs
    auto const defs = document->getDefs();
    defs->appendChildRepr(filter);

    filter->appendChild(primitive1);
    filter->appendChild(primitive2);

    Inkscape::GC::release(primitive1);
    Inkscape::GC::release(primitive2);
    Inkscape::GC::release(filter);

    return filter_uri;
}

void LPEPowerMask::update_mask_box()
{
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }

    SPMask *mask = sp_lpe_item->getMaskObject();
    if (!mask) {
        return;
    }

    auto const box_id = getId() + "_box";
    auto const box_ref = document->getObjectById(box_id);

    if (!background && box_ref) {
        // Delete the background box
        auto const box_item = cast<SPItem>(box_ref);
        box_item->setHidden(true);
        return;
    }

    // Prepare the background box
    Inkscape::XML::Node *box = nullptr;

    if (box_ref) {
        auto const box_item = cast<SPItem>(box_ref);
        box_item->setHidden(false);
        box = box_ref->getRepr();
    } else {
        // Initialize a box node if not present
        auto const xml_doc = document->getReprDoc();
        box = xml_doc->createElement("svg:path");
        box->setAttribute("id", box_id);
        mask->appendChildRepr(box);
        Inkscape::GC::release(box);
    }

    // Add CSS
    auto const css = sp_repr_css_attr_new();
    sp_repr_css_set_property_string(css, "fill", background_color.get_value()->toString(false));
    sp_repr_css_set_property_double(css, "fill-opacity", background_color.get_value()->getOpacity());
    sp_repr_css_set_property_string(css, "stroke", "none");
    sp_repr_css_change(box, css, "style");
    sp_repr_css_attr_unref(css);

    box->setAttribute("d", sp_svg_write_path(mask_box_path));
    box->setPosition(0);
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
