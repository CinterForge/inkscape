// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 *
 * Two related object to path operations:
 *
 * 1. Find a path that includes fill, stroke, and markers. Useful for finding a visual bounding box.
 * 2. Take a set of objects and find an identical visual representation using only paths.
 *
 * Copyright (C) 2020 Tavmjong Bah
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Code moved from splivarot.cpp
 *
 */

#include <vector>

#include "document.h"
#include "selection.h"
#include "style.h"

#include "helper/geom.h"    // pathv_to_linear_and_cubic()

#include "livarot/LivarotDefs.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"

#include "object/object-set.h"
#include "object/box3d.h"
#include "object/sp-item.h"
#include "object/sp-marker.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "object/sp-flowtext.h"

#include "path-chemistry.h"
#include "path-curve.h"
#include "path-outline.h"

#include "svg/svg.h"

/**
 * Given an item, find a path representing the fill and a path representing the stroke.
 * Returns true if fill path found. Item may not have a stroke in which case stroke path is empty.
 * bbox_only==true skips cleaning up the stroke path.
 * Encapsulates use of livarot.
 */
bool
item_find_paths(const SPItem *item, Geom::PathVector& fill, Geom::PathVector& stroke, bool bbox_only)
{
    auto shape = cast<SPShape>(item);
    auto text = cast<SPText>(item);

    if (!shape && !text) {
        return false;
    }

    std::optional<Geom::PathVector> curve;
    if (shape) {
        curve = ptr_to_opt(shape->curve());
    } else if (text) {
        curve = text->getNormalizedBpath();
    } else {
        std::cerr << "item_find_paths: item not shape or text!" << std::endl;
        return false;
    }

    if (!curve) {
        std::cerr << "item_find_paths: no curve!" << std::endl;
        return false;
    }

    if (curve->empty()) {
        std::cerr << "item_find_paths: curve empty!" << std::endl;
        return false;
    }

    fill = std::move(*curve);

    if (!item->style) {
        // Should never happen
        std::cerr << "item_find_paths: item with no style!" << std::endl;
        return false;
    }

    if (item->style->stroke.isNone() || item->style->stroke_width.computed <= Geom::EPSILON) {
        // No stroke, no chocolate!
        return true;
    }

    // Now that we have a valid curve with stroke, do offset. We use Livarot for this as
    // lib2geom does not yet handle offsets correctly.

    // Livarot's outline of arcs is broken. So convert the path to linear and cubics only, for
    // which the outline is created correctly.
    Geom::PathVector pathv = pathv_to_linear_and_cubic_beziers( fill );

    SPStyle *style = item->style;

    double stroke_width = style->stroke_width.computed;
    double miter = style->stroke_miterlimit.value * stroke_width;

    JoinType join;
    switch (style->stroke_linejoin.computed) {
        case SP_STROKE_LINEJOIN_MITER:
            join = join_pointy;
            break;
        case SP_STROKE_LINEJOIN_ROUND:
            join = join_round;
            break;
        default:
            join = join_straight;
            break;
    }

    ButtType butt;
    switch (style->stroke_linecap.computed) {
        case SP_STROKE_LINECAP_SQUARE:
            butt = butt_square;
            break;
        case SP_STROKE_LINECAP_ROUND:
            butt = butt_round;
            break;
        default:
            butt = butt_straight;
            break;
    }

    Path *origin = new Path; // Fill
    Path *offset = new Path;

    Geom::Affine const transform(item->transform);
    double const scale = transform.descrim();

    origin->LoadPathVector(pathv);
    offset->SetBackData(false);

    if (!style->stroke_dasharray.values.empty() && style->stroke_dasharray.is_valid()) {
        // We have dashes!
        origin->ConvertWithBackData(0.005); // Approximate by polyline
        origin->DashPolyline(style->stroke_dasharray.get_computed(), style->stroke_dashoffset.computed, scale, 0);
        auto bounds = Geom::bounds_fast(pathv);
        if (bounds) {
            double size = Geom::L2(bounds->dimensions());
            origin->Simplify(size * 0.000005); // Polylines to Beziers
        }
    }

    // Finally do offset!
    origin->Outline(offset, 0.5 * stroke_width, join, butt, 0.5 * miter);

    if (bbox_only) {
        stroke = offset->MakePathVector();
    } else {
        // Clean-up shape

        offset->ConvertWithBackData(1.0); // Approximate by polyline

        Shape *theShape  = new Shape;
        offset->Fill(theShape, 0); // Convert polyline to shape, step 1.

        Shape *theOffset = new Shape;
        theOffset->ConvertToShape(theShape, fill_positive); // Create an intersection free polygon (theOffset), step2.
        theOffset->ConvertToForme(origin, 1, &offset); // Turn shape into contour (stored in origin).

        stroke = origin->MakePathVector(); // Note origin was replaced above by stroke!
    }

    delete origin;
    delete offset;

    // std::cout << "    fill:   " << sp_svg_write_path(fill)   << "  count: " << fill.curveCount() << std::endl;
    // std::cout << "    stroke: " << sp_svg_write_path(stroke) << "  count: " << stroke.curveCount() << std::endl;
    return true;
}


// ======================== Item to Outline ===================== //

static
void item_to_outline_add_marker_child( SPItem const *item, Geom::Affine marker_transform, Geom::PathVector* pathv_in )
{
    Geom::Affine tr(marker_transform);
    tr = item->transform * tr;

    // note: a marker child item can be an item group!
    if (is<SPGroup>(item)) {
        // recurse through all childs:
        for (auto& o: item->children) {
            if (auto childitem = cast<SPItem>(&o)) {
                item_to_outline_add_marker_child(childitem, tr, pathv_in);
            }
        }
    } else {
        Geom::PathVector* marker_pathv = item_to_outline(item);

        if (marker_pathv) {
            for (const auto & j : *marker_pathv) {
                pathv_in->push_back(j * tr);
            }
            delete marker_pathv;
        }
    }
}

/**
 *  Returns a pathvector that is the outline of the stroked item, with markers.
 *  item must be an SPShape or an SPText.
 *  The only current use of this function has exclude_markers true! (SPShape::either_bbox).
 *  TODO: See if SPShape::either_bbox's union with markers is the same as one would get
 *  with bbox_only false.
 */
Geom::PathVector* item_to_outline(SPItem const *item, bool exclude_markers)
{
    Geom::PathVector fill;   // Used for locating markers.
    Geom::PathVector stroke; // Used for creating outline (and finding bbox).
    item_find_paths(item, fill, stroke, true); // Skip cleaning up stroke shape.

    Geom::PathVector *ret_pathv = nullptr;

    if (fill.curveCount() == 0) {
        std::cerr << "item_to_outline: fill path has no segments!" << std::endl;
        return ret_pathv;
    }

    if (stroke.size() > 0) {
        ret_pathv = new Geom::PathVector(stroke);
    } else {
        // No stroke, use fill path.
        ret_pathv = new Geom::PathVector(fill);
    }

    if (exclude_markers) {
        return ret_pathv;
    }

    auto shape = cast<SPShape>(item);
    if (shape && shape->hasMarkers()) {
        for (auto const &[_, marker, tr] : shape->get_markers()) {
            if (auto const marker_item = sp_item_first_item_child(marker)) {
                item_to_outline_add_marker_child(marker_item, marker->c2p * tr, ret_pathv);
            }
        }
    }

    return ret_pathv;
}

// ========================= Stroke to Path ====================== //
static void item_to_paths_add_marker(SPItem *context, SPMarker const *marker, Geom::Affine const &marker_transform,
                                     Inkscape::XML::Node *g_repr, bool legacy)
{
    auto doc = context->document;
    for (auto &obj : marker->children) {
        if (auto item = cast<SPItem>(&obj)) {
            // NOTE: The SVG spec says that a <marker> cannot have a transform attribute, even if it's set, it should be ignored.
            // The SPMarker in Inkscape inherits from SPGroup so it does allow a transform, even though it shouldn't.
            auto const tr = item->transform * marker_transform;

            Inkscape::XML::Node *m_repr = obj.getRepr()->duplicate(doc->getReprDoc());
            g_repr->appendChild(m_repr);

            if (auto m_item = cast<SPItem>(doc->getObjectByRepr(m_repr))) {
                m_item->doWriteTransform(tr);
                if (!legacy) {
                    item_to_paths(m_item, legacy, context);
                }
            }
        }
    }
}

/*
 * Find an outline that represents an item.
 * If legacy, text will not be handled as it is not a shape.
 * If a new item is created it is returned.
 * If the input item is a group and that group contains a changed item, the group node is returned
 * (marking a change).
 *
 * The return value is used externally to update a selection. It is nullptr if no change is made.
 */
Inkscape::XML::Node*
item_to_paths(SPItem *item, bool legacy, SPItem *context)
{
    char const *id = item->getAttribute("id");
    SPDocument *doc = item->document;
    bool flatten = false;
    // flatten all paths effects
    auto lpeitem = cast<SPLPEItem>(item);
    if (lpeitem && lpeitem->hasPathEffect()) {
        lpeitem->removeAllPathEffects(true);
        SPObject *elemref = doc->getObjectById(id);
        if (elemref && elemref != item) {
            // If the LPE item is a shape, it is converted to a path 
            // so we need to reupdate the item
            item = cast<SPItem>(elemref);
        }
        auto flat_item = cast<SPLPEItem>(elemref);
        if (!flat_item || !flat_item->hasPathEffect()) {
            flatten = true;
        }
    }
    // convert text/3dbox to path
    if (is<SPText>(item) || is<SPFlowtext>(item) || is<SPBox3D>(item)) {
        if (legacy) {
            return nullptr;
        }

        Inkscape::ObjectSet original_objects {doc}; // doc or desktop shouldn't be necessary
        original_objects.add(item);
        original_objects.toCurves(true);
        SPItem * new_item = original_objects.singleItem();
        if (new_item && new_item != item) {
            flatten = true;
            item = new_item;
        } else {
            g_warning("item_to_paths: flattening text or 3D box failed.");
            return nullptr;
        }
    }
    // if group, recurse
    auto group = cast<SPGroup>(item);
    if (group) {
        if (legacy) {
            return nullptr;
        }
        std::vector<SPItem*> const item_list = group->item_list();
        bool did = false;
        for (auto subitem : item_list) {
            if (item_to_paths(subitem, legacy)) {
                did = true;
            }
        }
        if (did || flatten) {
            // This indicates that at least one thing was changed inside the group.
            return group->getRepr();
        } else {
            return nullptr;
        }
    }

    auto shape = cast<SPShape>(item);
    if (!shape) {
        return nullptr;
    }

    Geom::PathVector fill_path;
    Geom::PathVector stroke_path;
    bool status = item_find_paths(item, fill_path, stroke_path);

    if (!status) {
        // Was not a well structured shape (or text).
        return nullptr;
    }

    // The styles ------------------------

    // Copying stroke style to fill will fail for properties not defined by style attribute
    // (i.e., properties defined in style sheet or by attributes).
    SPStyle *style = item->style;
    SPCSSAttr *ncss = sp_css_attr_from_style(style, SP_STYLE_FLAG_ALWAYS);
    SPCSSAttr *ncsf = sp_css_attr_from_style(style, SP_STYLE_FLAG_ALWAYS);

    if (context) {
        SPCSSAttr *ctxt_style = sp_css_attr_from_style(context->style, SP_STYLE_FLAG_ALWAYS);

        // TODO: browsers have different behaviours with context on markers
        // we need to revisit in the future for best matching
        // also dont know if opacity is or should be included in context
        gchar const *s_val   = sp_repr_css_property(ctxt_style, "stroke", nullptr);
        gchar const *f_val   = sp_repr_css_property(ctxt_style, "fill", nullptr);
        if (style->fill.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE ||
            style->fill.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) 
        {
            gchar const *fill_value = (style->fill.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE) ? s_val : f_val;
            sp_repr_css_set_property(ncss, "fill", fill_value);
            sp_repr_css_set_property(ncsf, "fill", fill_value);
        }
        if (style->stroke.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE ||
            style->stroke.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) 
        {
            gchar const *stroke_value = (style->stroke.paintOrigin == SP_CSS_PAINT_ORIGIN_CONTEXT_FILL) ? f_val : s_val;
            sp_repr_css_set_property(ncss, "stroke", stroke_value);
            sp_repr_css_set_property(ncsf, "stroke", stroke_value);
        }
    }
    // Stroke
    
    gchar const *s_val   = sp_repr_css_property(ncss, "stroke", nullptr);
    gchar const *s_opac  = sp_repr_css_property(ncss, "stroke-opacity", nullptr);
    gchar const *f_val   = sp_repr_css_property(ncss, "fill", nullptr);
    gchar const *opacity = sp_repr_css_property(ncss, "opacity", nullptr);  // Also for markers
    gchar const *filter  = sp_repr_css_property(ncss, "filter", nullptr);   // Also for markers

    sp_repr_css_set_property(ncss, "stroke", "none");
    sp_repr_css_set_property(ncss, "stroke-width", nullptr);
    sp_repr_css_set_property(ncss, "stroke-opacity", "1.0");
    sp_repr_css_set_property(ncss, "filter", nullptr);
    sp_repr_css_set_property(ncss, "opacity", nullptr);
    sp_repr_css_unset_property(ncss, "marker-start");
    sp_repr_css_unset_property(ncss, "marker-mid");
    sp_repr_css_unset_property(ncss, "marker-end");

    // we change the stroke to fill on ncss to create the filled stroke
    sp_repr_css_set_property(ncss, "fill", s_val);
    if ( s_opac ) {
        sp_repr_css_set_property(ncss, "fill-opacity", s_opac);
    } else {
        sp_repr_css_set_property(ncss, "fill-opacity", "1.0");
    }
    
    sp_repr_css_set_property(ncsf, "stroke", "none");
    sp_repr_css_set_property(ncsf, "stroke-width", nullptr);
    sp_repr_css_set_property(ncsf, "stroke-opacity", "1.0");
    sp_repr_css_set_property(ncsf, "filter", nullptr);
    sp_repr_css_set_property(ncsf, "opacity", nullptr);
    sp_repr_css_unset_property(ncsf, "marker-start");
    sp_repr_css_unset_property(ncsf, "marker-mid");
    sp_repr_css_unset_property(ncsf, "marker-end");

    // The object tree -------------------

    // Remember the position of the item
    gint pos = item->getRepr()->position();

    // Remember parent
    Inkscape::XML::Node *parent = item->getRepr()->parent();

    Inkscape::XML::Document *xml_doc = doc->getReprDoc();

    // Create a group to put everything in.
    Inkscape::XML::Node *g_repr = xml_doc->createElement("svg:g");

    Inkscape::copy_object_properties(g_repr, item->getRepr());
    // drop copied style, children will be re-styled (stroke becomes fill)
    g_repr->removeAttribute("style");

    // Add the group to the parent, move to the saved position
    parent->addChildAtPos(g_repr, pos);

    // The stroke ------------------------
    Inkscape::XML::Node *stroke = nullptr;
    if (s_val && g_strcmp0(s_val,"none") != 0 && stroke_path.size() > 0) {
        auto stroke_style = std::make_unique<SPStyle>(doc);
        stroke_style->mergeCSS(ncss);

        stroke = xml_doc->createElement("svg:path");
        stroke->setAttribute("style", stroke_style->writeIfDiff(item->parent->style));
        stroke->setAttribute("d", sp_svg_write_path(stroke_path));
    }
    sp_repr_css_attr_unref(ncss);

    // The fill --------------------------
    Inkscape::XML::Node *fill = nullptr;
    if (f_val && g_strcmp0(f_val,"none") != 0 && !legacy) {
        auto fill_style = std::make_unique<SPStyle>(doc);
        fill_style->mergeCSS(ncsf);

        fill = xml_doc->createElement("svg:path");
        fill->setAttribute("style", fill_style->writeIfDiff(item->parent->style));
        fill->setAttribute("d", sp_svg_write_path(fill_path));
    }
    sp_repr_css_attr_unref(ncsf);

    // The markers -----------------------
    Inkscape::XML::Node *markers = nullptr;

    if (shape->hasMarkers()) {
        if (!legacy) {
            markers = xml_doc->createElement("svg:g");
            g_repr->addChildAtPos(markers, pos);
        } else {
            markers = g_repr;
        }

        for (auto const &[_, marker, tr] : shape->get_markers()) {
            item_to_paths_add_marker(item, marker, marker->c2p * tr, markers, legacy);
        }
    }

    gchar const *paint_order = sp_repr_css_property(ncss, "paint-order", nullptr);
    SPIPaintOrder temp;
    temp.read( paint_order );
    bool unique = false;
    if ((!fill && !markers) || (!fill && !stroke) || (!markers && !stroke)) {
        unique = true;
    }
    if (temp.layer[0] != SP_CSS_PAINT_ORDER_NORMAL && !legacy && !unique) {

        if (temp.layer[0] == SP_CSS_PAINT_ORDER_FILL) {
            if (temp.layer[1] == SP_CSS_PAINT_ORDER_STROKE) {
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( markers ) {
                    markers->setPosition(2);
                }
            } else {
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( markers ) {
                    markers->setPosition(1);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
            }
        } else if (temp.layer[0] == SP_CSS_PAINT_ORDER_STROKE) {
            if (temp.layer[1] == SP_CSS_PAINT_ORDER_FILL) {
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( markers ) {
                    markers->setPosition(2);
                }
            } else {
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( markers ) {
                    markers->setPosition(1);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
            }
        } else {
            if (temp.layer[1] == SP_CSS_PAINT_ORDER_STROKE) {
                if ( markers ) {
                    markers->setPosition(0);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
            } else {
                if ( markers ) {
                    markers->setPosition(0);
                }
                if ( fill ) {
                    g_repr->appendChild(fill);
                }
                if ( stroke ) {
                    g_repr->appendChild(stroke);
                }
            }
        }

    } else if (!unique) {
        if ( fill ) {
            g_repr->appendChild(fill);
        }
        if ( stroke ) {
            g_repr->appendChild(stroke);
        }
        if ( markers ) {
            markers->setPosition(2);
        }
    }

    bool did = false;
    // only consider it a change if more than a fill is created.
    if (stroke || markers) {
        did = true;
    }

    Inkscape::XML::Node *out = nullptr;

    if (!fill && !markers && did) {
        out = stroke;
    } else if (!fill && !stroke  && did) {
        out = markers;
    } else if(did) {
        out = g_repr;
    } else {
        parent->removeChild(g_repr);
        Inkscape::GC::release(g_repr);
        if (fill) {
            // Copy the style, to preserve context-fill cascade
            if (context) {
                item->setAttribute("style", fill->attribute("style"));
            }
            Inkscape::GC::release(fill);
        }
        return (flatten ? item->getRepr() : nullptr);
    }

    SPCSSAttr *r_style = sp_repr_css_attr_new();
    sp_repr_css_set_property(r_style, "opacity", opacity);
    sp_repr_css_set_property(r_style, "filter", filter);
    sp_repr_css_change(out, r_style, "style");

    sp_repr_css_attr_unref(r_style);
    if (unique && out != markers) { // markers are already a child of g_repr
        g_assert(out != g_repr);
        parent->addChild(out, g_repr);
        parent->removeChild(g_repr);
        Inkscape::GC::release(g_repr);
    }
    out->setAttribute("transform", item->getRepr()->attribute("transform"));

    // We're replacing item, delete it.
    item->deleteObject(false);

    out->setAttribute("id",id);
    Inkscape::GC::release(out);

    return out;
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
