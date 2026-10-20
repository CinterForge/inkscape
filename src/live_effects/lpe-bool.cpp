// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Boolean operation live path effect
 *
 * Copyright (C) 2016-2017 Michael Soegtrop
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpe-bool.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <glibmm/i18n.h>

#include <2geom/affine.h>
#include <2geom/bezier-curve.h>
#include <2geom/path-sink.h>
#include <2geom/path.h>
#include <2geom/svg-path-parser.h>

#include "inkscape.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "snap.h"
#include "style.h"

#include "helper/geom.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"
#include "livarot/path-description.h"
#include "live_effects/lpeobject.h"
#include "object/sp-clippath.h"
#include "object/sp-defs.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "path/path-boolop.h"
#include "path/path-util.h"
#include "svg/svg.h"
#include "ui/tools/tool-base.h"

namespace Inkscape {
namespace LivePathEffect {

// Define an extended boolean operation type

static const Util::EnumData<LPEBool::bool_op_ex> BoolOpData[LPEBool::bool_op_ex_count] = {
    {LPEBool::bool_op_ex_union, N_("union"), "union"},
    {LPEBool::bool_op_ex_inters, N_("intersection"), "inters"},
    {LPEBool::bool_op_ex_diff, N_("difference"), "diff"},
    {LPEBool::bool_op_ex_symdiff, N_("symmetric difference"), "symdiff"},
    {LPEBool::bool_op_ex_cut, N_("division"), "cut"},
    {LPEBool::bool_op_ex_cut_both, N_("division both"), "cut-both"},
    // Note on naming of operations:
    // bool_op_cut is called "Division" in the manu, see sp_selected_path_cut
    // bool_op_slice is called "Cut path" in the menu, see sp_selected_path_slice
    // TODO: this 3 options are commented because dont work properly
    // maybe in 1.2 can be fixed but need libarot base to do
    // {LPEBool::bool_op_ex_slice, N_("cut"), "slice"},
    // {LPEBool::bool_op_ex_slice_inside, N_("cut inside"), "slice-inside"},
    // {LPEBool::bool_op_ex_slice_outside, N_("cut outside"), "slice-outside"},
};

static const Util::EnumDataConverter<LPEBool::bool_op_ex> BoolOpConverter(BoolOpData, sizeof(BoolOpData) / sizeof(*BoolOpData));

static const Util::EnumData<FillRule> FillTypeData[] = {
    { fill_oddEven, N_("even-odd"), "oddeven" },
    { fill_nonZero, N_("non-zero"), "nonzero" },
    { fill_positive, N_("positive"), "positive" },
    { fill_justDont, N_("take from object"), "from-curve" }
};

static const Util::EnumDataConverter<FillRule> FillTypeConverter(FillTypeData, sizeof(FillTypeData) / sizeof(*FillTypeData));

LPEBool::LPEBool(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , operand_item(_("Operand path:"), _("Operand for the boolean operation"), "operand-path", &wr, this)
    , bool_operation(_("Operation:"), _("Boolean Operation"), "operation", BoolOpConverter, &wr, this, bool_op_ex_union)
    , swap_operands(_("Swap operands"), _("Swap operands (useful e.g. for difference)"), "swap-operands", &wr, this)
    , rmv_inner(
          _("Remove inner"),
          _("For cut operations: remove inner (non-contour) lines of cutting path to avoid invisible extra points"),
          "rmv-inner", &wr, this)
    , fill_type_this(_("Fill type this:"), _("Fill type (winding mode) for this path"), "filltype-this",
                     FillTypeConverter, &wr, this, fill_justDont)
    , fill_type_operand(_("Fill type operand:"), _("Fill type (winding mode) for operand path"), "filltype-operand",
                        FillTypeConverter, &wr, this, fill_justDont)
    , filter("Filter", "Previous filter", "filter", &wr, this, "", true)
{
    registerParameter(&operand_item);
    registerParameter(&bool_operation);
    registerParameter(&swap_operands);
    //registerParameter(&rmv_inner);
    registerParameter(&fill_type_this);
    registerParameter(&filter);
    registerParameter(&fill_type_operand);
    show_orig_path = true;
    satellitestoclipboard = true;
    prev_affine = Geom::identity();
    operand = cast<SPItem>(operand_item.getObject());
    if (operand) {
        operand_id = operand->getId();
    }
}

LPEBool::~LPEBool() = default;

bool LPEBool::doOnOpen(SPLPEItem const *lpeitem)
{
    if (!is_load || is_applied) {
        return false;
    }
    Glib::ustring version = lpeversion.param_getSVGValue();
    if (version < "1.2") {
        lpeversion.param_setValue("1.2", true);
    }
    operand_item.start_listening(operand_item.getObject());
    operand_item.connect_selection_changed();
    return false;
}

bool cmp_cut_position(const Path::cut_position &a, const Path::cut_position &b)
{
    return a.piece == b.piece ? a.t < b.t : a.piece < b.piece;
}

Geom::PathVector
sp_pathvector_boolop_slice_intersect(Geom::PathVector const &pathva, Geom::PathVector const &pathvb, bool inside, FillRule fra, FillRule frb)
{
    // This is similar to sp_pathvector_boolop/bool_op_slice, but keeps only edges inside the cutter area.
    // The code is also based on sp_pathvector_boolop_slice.
    //
    // We have two paths on input
    // - a closed area which is used to cut out pieces from a contour (called area below)
    // - a contour which is cut into pieces by the border of thr area (called contour below)
    //
    // The code below works in the following steps
    // (a) Convert the area to a shape, so that we can ask the winding number for any point
    // (b) Add both, the contour and the area to a single shape and intersect them
    // (c) Find the intersection points between area border and contour (vector toCut)
    // (d) Split the original contour at the intersection points
    // (e) check for each contour edge in combined shape if its center is inside the area - if not discard it
    // (f) create a vector of all inside edges
    // (g) convert the piece numbers to the piece numbers after applying the cuts
    // (h) fill a bool vector with information which pieces are in
    // (i) filter the descr_cmd of the result path with this bool vector
    //
    // The main inefficiency here is step (e) because I use a winding function of the area-shape which goes
    // through the complete edge list for each point I ask for, so effort is n-edges-contour * n-edges-area.
    // It is tricky to improve this without building into the livarot code.
    // One way might be to decide at the intersection points which edges touching the intersection points are
    // in by making a loop through all edges on the intersection vertex. Since this is a directed non intersecting
    // graph, this should provide sufficient information.
    // But since I anyway will change this to the new mechanism some time speed is fairly ok, I didn't look into this.


    // extract the livarot Paths from the source objects
    // also get the winding rule specified in the style
    // Livarot's outline of arcs is broken. So convert the path to linear and cubics only, for which the outline is created correctly.
    auto contour_path = Path_for_pathvector(pathv_to_linear_and_cubic_beziers(pathva));
    auto area_path = Path_for_pathvector(pathv_to_linear_and_cubic_beziers(pathvb));

    // Shapes from above paths
    Shape *area_shape = new Shape;
    Shape *combined_shape = new Shape;
    Shape *combined_inters = new Shape;

    // Add the area (process to intersection free shape)
    area_path->ConvertWithBackData(1.0);
    area_path->Fill(combined_shape, 1);

    // Convert this to a shape with full winding information
    area_shape->ConvertToShape(combined_shape, frb);

    // Add the contour to the combined path (just add, no winding processing)
    contour_path->ConvertWithBackData(1.0);
    contour_path->Fill(combined_shape, 0, true, false, false);

    // Intersect the area and the contour - no fill processing
    combined_inters->ConvertToShape(combined_shape, fill_justDont);

    // Result path
    Path *result_path = new Path;
    result_path->SetBackData(false);

    // Cutting positions for contour
    std::vector<Path::cut_position> toCut;

    if (combined_inters->hasBackData()) {
        // should always be the case, but ya never know
        {
            for (int i = 0; i < combined_inters->numberOfPoints(); i++) {
                if (combined_inters->getPoint(i).totalDegree() > 2) {
                    // possibly an intersection
                    // we need to check that at least one edge from the source path is incident to it
                    // before we declare it's an intersection
                    int cb = combined_inters->getPoint(i).incidentEdge[FIRST];
                    int   nbOrig = 0;
                    int   nbOther = 0;
                    int   piece = -1;
                    float t = 0.0;
                    while (cb >= 0 && cb < combined_inters->numberOfEdges()) {
                        if (combined_inters->ebData[cb].pathID == 0) {
                            // the source has an edge incident to the point, get its position on the path
                            piece = combined_inters->ebData[cb].pieceID;
                            if (combined_inters->getEdge(cb).st == i) {
                                t = combined_inters->ebData[cb].tSt;
                            } else {
                                t = combined_inters->ebData[cb].tEn;
                            }
                            nbOrig++;
                        }
                        if (combined_inters->ebData[cb].pathID == 1) {
                            nbOther++;    // the cut is incident to this point
                        }
                        cb = combined_inters->NextAt(i, cb);
                    }
                    if (nbOrig > 0 && nbOther > 0) {
                        // point incident to both path and cut: an intersection
                        // note that you only keep one position on the source; you could have degenerate
                        // cases where the source crosses itself at this point, and you wouyld miss an intersection
                        Path::cut_position cutpos;
                        cutpos.piece = piece;
                        cutpos.t = t;
                        toCut.push_back(cutpos);
                    }
                }
            }
        }
        {
            // remove the edges from the intersection polygon
            int i = combined_inters->numberOfEdges() - 1;
            for (; i >= 0; i--) {
                if (combined_inters->ebData[i].pathID == 1) {
                    combined_inters->SubEdge(i);
                } else {
                    const Shape::dg_arete &edge = combined_inters->getEdge(i);
                    const Shape::dg_point &start = combined_inters->getPoint(edge.st);
                    const Shape::dg_point &end = combined_inters->getPoint(edge.en);
                    Geom::Point mid = 0.5 * (start.x + end.x);
                    int wind = area_shape->PtWinding(mid);
                    if (wind == 0) {
                        combined_inters->SubEdge(i);
                    }
                }
            }
        }
    }

    // create a vector of pieces, which are in the intersection
    std::vector<Path::cut_position> inside_pieces(combined_inters->numberOfEdges());
    for (int i = 0; i < combined_inters->numberOfEdges(); i++) {
        inside_pieces[i].piece = combined_inters->ebData[i].pieceID;
        // Use the t middle point, this is safe to compare with values from toCut in the presence of roundoff errors
        inside_pieces[i].t = 0.5 * (combined_inters->ebData[i].tSt + combined_inters->ebData[i].tEn);
    }
    std::sort(inside_pieces.begin(), inside_pieces.end(), cmp_cut_position);

    // sort cut positions
    std::sort(toCut.begin(), toCut.end(), cmp_cut_position);

    // Compute piece ids after ConvertPositionsToMoveTo
    {
        int idIncr = 0;
        std::vector<Path::cut_position>::iterator itPiece = inside_pieces.begin();
        std::vector<Path::cut_position>::iterator itCut = toCut.begin();
        while (itPiece != inside_pieces.end()) {
            while (itCut != toCut.end() && cmp_cut_position(*itCut, *itPiece)) {
                ++itCut;
                idIncr += 2;
            }
            itPiece->piece += idIncr;
            ++itPiece;
        }
    }

    // Copy the original path to result and cut at the intersection points
    result_path->Copy(contour_path.get());
    result_path->ConvertPositionsToMoveTo(toCut.size(), toCut.data());   // cut where you found intersections

    // Create an array of bools which states which pieces are in
    std::vector<bool> inside_flags(result_path->descr_cmd.size(), false);
    for (auto & inside_piece : inside_pieces) {
        inside_flags[ inside_piece.piece ] = true;
        // also enable the element -1 to get the MoveTo
        if (inside_piece.piece >= 1) {
            inside_flags[ inside_piece.piece - 1 ] = true;
        }
    }

#if 0 // CONCEPT TESTING
    //Check if the inside/outside verdict is consistent - just for testing the concept
    // Retrieve the pieces
    int nParts = 0;
    Path **parts = result_path->SubPaths(nParts, false);

    // Each piece should be either fully in or fully out
    int iPiece = 0;
    for (int iPart = 0; iPart < nParts; iPart++) {
        bool andsum = true;
        bool orsum = false;
        for (int iCmd = 0; iCmd < parts[iPart]->descr_cmd.size(); iCmd++, iPiece++) {
            andsum = andsum && inside_flags[ iPiece ];
            orsum = andsum || inside_flags[ iPiece ];
        }

        if (andsum != orsum) {
            g_warning("Inconsistent inside/outside verdict for part=%d", iPart);
        }
    }
    g_free(parts);
#endif

    // iterate over the commands of a path and keep those which are inside
    int iDest = 0;
    for (int iSrc = 0; iSrc < result_path->descr_cmd.size(); iSrc++) {
        if (inside_flags[iSrc] == inside) {
            result_path->descr_cmd[iDest++] = result_path->descr_cmd[iSrc];
        } else {
            delete result_path->descr_cmd[iSrc];
        }
    }
    result_path->descr_cmd.resize(iDest);

    delete combined_inters;
    delete combined_shape;
    delete area_shape;

    auto outres = result_path->MakePathVector();
    delete result_path;

    return outres;
}

// remove inner contours
Geom::PathVector
sp_pathvector_boolop_remove_inner(Geom::PathVector const &pathva, FillRule fra)
{
    Geom::PathVector patht;
    auto patha = Path_for_pathvector(pathv_to_linear_and_cubic_beziers(pathva));

    Shape *shape = new Shape;
    Shape *shapeshape = new Shape;
    Path *resultp = new Path;
    resultp->SetBackData(false);

    patha->ConvertWithBackData(0.1);
    patha->Fill(shape, 0);
    shapeshape->ConvertToShape(shape, fra);
    Path *paths[] = { patha.get() };
    shapeshape->ConvertToForme(resultp, 1, paths);

    delete shape;
    delete shapeshape;

    auto resultpv = resultp->MakePathVector();

    delete resultp;
    return resultpv;
}

static FillRule GetFillTyp(SPItem *item)
{
    SPCSSAttr *css = sp_repr_css_attr(item->getRepr(), "style");
    gchar const *val = sp_repr_css_property(css, "fill-rule", nullptr);
    if (val && strcmp(val, "nonzero") == 0) {
        return fill_nonZero;
    } else if (val && strcmp(val, "evenodd") == 0) {
        return fill_oddEven;
    } else {
        return fill_nonZero;
    }
}

void LPEBool::add_filter()
{
    auto operand = cast<SPItem>(operand_item.getObject());
    if (operand) {
        Inkscape::XML::Node *repr = operand->getRepr();
        if (!repr) {
            return;
        }
        SPFilter *filt = operand->style->getFilter();
        if (filt && filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") != 0) {
            filter.param_setValue(filt->getId(), true);
        }
        if (!filt || (filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") != 0)) {
            SPCSSAttr *css = sp_repr_css_attr_new();
            sp_repr_css_set_property(css, "filter", "url(#selectable_hidder_filter)");
            sp_repr_css_change(repr, css, "style");
            sp_repr_css_attr_unref(css);
        }
    }
}

void LPEBool::remove_filter(SPObject *operand)
{
    if (operand) {
        Inkscape::XML::Node *repr = operand->getRepr();
        if (!repr) {
            return;
        }
        SPFilter *filt = operand->style->getFilter();
        if (filt && (filt->getId() && strcmp(filt->getId(), "selectable_hidder_filter") == 0)) {
            SPCSSAttr *css = sp_repr_css_attr_new();
            Glib::ustring filtstr = filter.param_getSVGValue();
            if (filtstr != "") {
                Glib::ustring url = "url(#";
                url += filtstr;
                url += ")";
                sp_repr_css_set_property(css, "filter", url.c_str());
                // blur is removed when no item using it
                /*SPDocument *document = getSPDoc();
                SPObject * filterobj = nullptr;
                if((filterobj = document->getObjectById(filtstr))) {
                    for (auto obj:filterobj->childList(false)) {
                        if (obj) {
                            obj->deleteObject(false);
                            break;
                        }
                    }
                } */
                filter.param_setValue("");
            } else {
                sp_repr_css_unset_property(css, "filter");
            }
            sp_repr_css_change(repr, css, "style");
            sp_repr_css_attr_unref(css);
        }
    }
}

void LPEBool::doAfterEffect(SPLPEItem const *lpeitem, Geom::PathVector *)
{
    if (onremove) {
        onremove = false;
    }
}

void LPEBool::doBeforeEffect(SPLPEItem const *lpeitem)
{
    SPDocument *document = getSPDoc();
    if (!document) {
        return;
    }

    _hp.clear();
    Inkscape::XML::Document *xml_doc = document->getReprDoc();
    SPObject *elemref = nullptr;
    Inkscape::XML::Node *boolfilter = nullptr;
    if (!(elemref = document->getObjectById("selectable_hidder_filter"))) {
        boolfilter = xml_doc->createElement("svg:filter");
        boolfilter->setAttribute("id", "selectable_hidder_filter");
        boolfilter->setAttribute("width", "1");
        boolfilter->setAttribute("height", "1");
        boolfilter->setAttribute("x", "0");
        boolfilter->setAttribute("y", "0");
        boolfilter->setAttribute("style", "color-interpolation-filters:sRGB;");
        boolfilter->setAttribute("inkscape:label", "LPE boolean visibility");
        /* Create <path> */
        Inkscape::XML::Node *primitive = xml_doc->createElement("svg:feComposite");
        primitive->setAttribute("id", "boolops_hidder_primitive");
        primitive->setAttribute("result", "composite1");
        primitive->setAttribute("operator", "arithmetic");
        primitive->setAttribute("in2", "SourceGraphic");
        primitive->setAttribute("in", "BackgroundImage");
        Inkscape::XML::Node *defs = document->getDefs()->getRepr();
        defs->addChild(boolfilter, nullptr);
        Inkscape::GC::release(boolfilter);
        boolfilter->addChild(primitive, nullptr);
        Inkscape::GC::release(primitive);
    } else {
        for (auto obj : elemref->childList(false)) {
            if (obj && strcmp(obj->getId(), "boolops_hidder_primitive") != 0) {
                obj->deleteObject(true);
            }
        }
    }
    if (!is_load && !isOnClipboard() && operand_item.lperef && 
        operand_item.lperef->isAttached() && operand_item.lperef.get()->getObject() == nullptr) 
    {
        operand_item.unlink();
        return;
    }
    auto current_operand = cast<SPItem>(operand_item.getObject());
    if (onremove && current_operand) {
        operand_id = current_operand->getId();
        return;
    }
    operand =  cast<SPItem>(getSPDoc()->getObjectById(operand_id));
    if (!operand_item.linksToItem()) {
        operand_item.read_from_SVG();
        current_operand = cast<SPItem>(operand_item.getObject());
    }
    if (!current_operand && !operand) {
        return;
    }
    if (!current_operand) {
        operand_item.unlink();
    }
    if (current_operand && !operand ) {
        operand_id = current_operand->getId();
        sp_lpe_item_update_patheffect(sp_lpe_item, false, false, true);
        return;
    }
    if (current_operand && !operand_item.isConnected()) {
        operand_item.start_listening(current_operand);
        sp_lpe_item_update_patheffect(sp_lpe_item, false, false, true);
        return;
    }

    if (current_operand) {
        if (!(document->getObjectById(current_operand->getId()))) {
            operand_item.unlink();
            operand = nullptr;
            operand_id = "";
            current_operand = nullptr;
        } else {
            operand_id = current_operand->getId();
        }
    }
    auto operandlpe = cast<SPLPEItem>(operand_item.getObject());
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (desktop) {
        Inkscape::Selection *selection = desktop->getSelection();
        if (selection && operand && sp_lpe_item && selection->includes(operand) && selection->includes(sp_lpe_item)) {
            if (operandlpe && operandlpe->hasPathEffectOfType(Inkscape::LivePathEffect::EffectType::BOOL_OP)) {
                sp_lpe_item_update_patheffect(operandlpe, false, false);
            }
        }
    }
    if (!current_operand) {
        if (operand) {
            remove_filter(operand);
        }
        operand = nullptr;
        operand_id = "";
    }
    
    if (current_operand && operand != current_operand) {
        if (operand) {
            remove_filter(operand);
        }
        operand = current_operand;
        remove_filter(operand);
        if (is_load && sp_lpe_item) {
            sp_lpe_item_update_patheffect(sp_lpe_item, true, true);
        }
    }
    if (current_operand) {
        bool_op_ex op = bool_operation.get_value();
        if (is_visible && op != bool_op_ex_cut_both) {
            add_filter();
        } else {
            remove_filter(current_operand);
        }
    }
}

void LPEBool::transform_multiply(Geom::Affine const &postmul, bool /*set*/)
{
    operand =  cast<SPItem>(sp_lpe_item->document->getObjectById(operand_id));
    if (is_visible && sp_lpe_item->pathEffectsEnabled() && operand && !isOnClipboard()) {
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        if (desktop && !desktop->getSelection()->includes(operand)) {
            prev_affine = operand->transform * sp_item_transform_repr(sp_lpe_item).inverse() * postmul;
            operand->doWriteTransform(prev_affine);
        }
    }
}

Geom::PathVector LPEBool::get_union(SPObject *root, SPObject *object, bool _from_original_d)
{
    Geom::PathVector res;
    Geom::PathVector clippv;
    if (auto objitem = cast<SPItem>(object)) {
        SPObject *clip_path = objitem->getClipObject();
        if (clip_path) {
            std::vector<SPObject *> clip_path_list = clip_path->childList(true);
            if (clip_path_list.size()) {
                for (auto clip : clip_path_list) {
                    if (auto clipshape = cast<SPShape>(clip)) {
                        auto curve = _from_original_d
                                   ? clipshape->curveForEdit()
                                   : clipshape->curve();
                        if (curve) {
                            clippv = *curve;
                        }
                    }
                }
            }
        }
    }
    if (auto group = cast<SPGroup>(object)) {
        std::vector<SPItem *> item_list = group->item_list();
        for (auto iter : item_list) {
            Geom::PathVector tmp = get_union(root, iter, _from_original_d);
            if (res.empty()) {
                res = std::move(tmp);
            } else {
                res = sp_pathvector_boolop(res, tmp, to_bool_op(bool_op_ex_union), fill_oddEven, fill_oddEven);
            }
        }
    }
    if (auto shape = cast<SPShape>(object)) {
        FillRule originfill = fill_oddEven;
        auto curve = _from_original_d
                   ? shape->curveForEdit()
                   : shape->curve();
        if (curve) {
            auto tmp = *curve * i2anc_affine(shape, root->parent);
            if (res.empty()) {
                res = std::move(tmp);
            } else {
                res = sp_pathvector_boolop(res, tmp, to_bool_op(bool_op_ex_union), originfill, GetFillTyp(shape));
            }
        }
        originfill = GetFillTyp(shape);
    }
    if (auto text = cast<SPText>(object)) {
        auto tmp = text->getNormalizedBpath();
        tmp *= i2anc_affine(text, root->parent);
        if (res.empty()) {
            res = std::move(tmp);
        } else {
            res = sp_pathvector_boolop(res, tmp, to_bool_op(bool_op_ex_union), fill_oddEven, fill_oddEven);
        }
    }
    if (!clippv.empty()) {
        res = sp_pathvector_boolop(clippv, res, to_bool_op(bool_op_ex_diff), fill_oddEven, fill_oddEven);
    }
    return res;
}

void LPEBool::doEffect(Geom::PathVector &curve)
{
    Geom::PathVector path_in = curve;
    auto current_operand = cast<SPItem>(operand_item.getObject());
    if (current_operand == current_shape) {
        g_warning("operand and current shape are the same");
        operand_item.param_set_default();
        return;
    }
    if (onremove) {
        current_operand = cast<SPItem>(getSPDoc()->getObjectById(operand_id));
    }
    if (current_operand) {
        bool_op_ex op = bool_operation.get_value();
        bool swap =  swap_operands.get_value();
        if (op == bool_op_ex_cut_both) {
            swap = false;
        }

        Geom::Affine current_affine = sp_lpe_item->transform;
        Geom::PathVector operand_pv = get_union(current_operand, current_operand);
        if (operand_pv.empty()) {
            return;
        }
        path_in *= current_affine;

        Geom::PathVector path_a = swap ? path_in : operand_pv;
        Geom::PathVector path_b = swap ? operand_pv : path_in;
        _hp = path_a;
        _hp.insert(_hp.end(), path_b.begin(), path_b.end());
        _hp *= current_affine.inverse();
        auto item = cast<SPItem>(operand_item.getObject());
        FillRule fill_this    = fill_type_this.get_value() != fill_justDont ? fill_type_this.get_value() : GetFillTyp(current_shape);
        FillRule fill_operand =
            fill_type_operand.get_value() != fill_justDont ? fill_type_operand.get_value() : GetFillTyp(item);

        FillRule fill_a = swap ? fill_this : fill_operand;
        FillRule fill_b = swap ? fill_operand : fill_this;

        if (rmv_inner.get_value()) {
            path_b = sp_pathvector_boolop_remove_inner(path_b, fill_b);
        }
        Geom::PathVector path_out;
        helperLineSatellites = false;
        if (op == bool_op_ex_cut) {
            if (onremove) {
                path_out = sp_pathvector_boolop(path_a, path_b, to_bool_op(bool_op_ex_diff), fill_a, fill_b);
            } else {
                path_out = sp_pathvector_boolop(path_a, path_b, to_bool_op(op), fill_a, fill_b);
            }
        /* } else if (op == bool_op_ex_slice) {
            path_out = sp_pathvector_boolop_slice_intersect(path_a, path_b, true, fill_a, fill_b);
            Geom::PathVector path_tmp = sp_pathvector_boolop_slice_intersect(path_a, path_b, false, fill_a, fill_b);
            for (auto pathit : path_tmp) {
                path_out.push_back(pathit);
            }
        } else if (op == bool_op_ex_slice_inside) {
            path_out = sp_pathvector_boolop_slice_intersect(path_a, path_b, true, fill_a, fill_b);
        } else if (op == bool_op_ex_slice_outside) {
            path_out = sp_pathvector_boolop_slice_intersect(path_a, path_b, false, fill_a, fill_b);
         */
        } else if (op == bool_op_ex_cut_both){
            if (onremove) {
                path_out = sp_pathvector_boolop(path_a, path_b, to_bool_op(bool_op_ex_diff), fill_a, fill_b);
            } else {
                helperLineSatellites = true;
                path_out = sp_pathvector_boolop(path_a, path_b, bool_op_diff,   fill_a, fill_b);
                auto tmp = sp_pathvector_boolop(path_a, path_b, bool_op_inters, fill_a, fill_b);
                path_out.insert(path_out.end(),tmp.begin(),tmp.end());
                /* auto tmp2 = sp_pathvector_boolop(path_a, path_b, (bool_op) bool_op_diff, fill_a, fill_b);
                path_out.insert(path_out.end(),tmp2.begin(),tmp2.end()); */
            }
        } else {
            path_out = sp_pathvector_boolop(path_a, path_b, (BooleanOp) op, fill_a, fill_b);
        }
        curve = std::move(path_out) * current_affine.inverse();
    }
}

void LPEBool::addCanvasIndicators(SPLPEItem const * /*lpeitem*/, std::vector<Geom::PathVector> &hp_vec)
{
    hp_vec.push_back(_hp);
}

Inkscape::XML::Node *
LPEBool::dupleNode(SPObject * origin, Glib::ustring element_type)
{
    Inkscape::XML::Document *xml_doc = getSPDoc()->getReprDoc();
    Inkscape::XML::Node *dest = xml_doc->createElement(element_type.c_str());
    dest->setAttribute("transform", origin->getAttribute("transform"));
    dest->setAttribute("d", origin->getAttribute("d"));
    dest->setAttribute("style", origin->getAttribute("style"));
    dest->setAttribute("mask", origin->getAttribute("mask"));
    dest->setAttribute("clip-path", origin->getAttribute("clip-path"));
    dest->setAttribute("class", origin->getAttribute("class"));
    dest->setAttribute("style", origin->getAttribute("style"));
    for (auto iter : origin->style->properties()) {
        if (iter->style_src != SPStyleSrc::UNSET) {
            auto key = iter->id();
            if (key != SPAttr::FONT && key != SPAttr::D && key != SPAttr::MARKER) {
                const gchar *attr = origin->getAttribute(iter->name().c_str());
                if (attr) {
                    dest->setAttribute(iter->name(), attr);
                }
            }
        }
    }
    return dest;
}

void LPEBool::fractureit(SPObject * operandit, Geom::PathVector unionpv)
{
    // 1.2 feature no need to legacy bool
    auto operandit_item = cast<SPItem>(operandit);
    auto operandit_g = cast<SPGroup>(operandit);
    auto operandit_shape = cast<SPShape>(operandit);
    FillRule fill_a = fill_type_this.get_value() != fill_justDont ? fill_type_this.get_value() : GetFillTyp(operandit_item);
    FillRule fill_b = fill_type_operand.get_value() != fill_justDont ? fill_type_operand.get_value() : GetFillTyp(operandit_item);
    //unionpv *= sp_lpe_item->transform;
    auto divisionit = cast<SPItem>(getSPDoc()->getObjectById(division_id)); 
    if (operandit_g) {
        Inkscape::XML::Node *dest = dupleNode(operandit, "svg:g");
        dest->setAttribute("transform", nullptr);
        if (!division_other) {
            division_other = cast<SPGroup>(sp_lpe_item->parent->appendChildRepr(dest));
            Inkscape::GC::release(dest);
            division_other_id = division_other->getId();
            division_other->parent->reorder(division_other, divisionit);
        } else {
            division_other = cast<SPGroup>(division_other->appendChildRepr(dest));
        } 
        Inkscape::XML::Node *dest2 = dupleNode(operandit, "svg:g");
        dest2->setAttribute("transform", nullptr);
        if (!division_both) {
            division_both = cast<SPGroup>(sp_lpe_item->parent->appendChildRepr(dest2));
            Inkscape::GC::release(dest2);
            division_both->parent->reorder(division_both, division_other);
        } else {
            division_both = cast<SPGroup>(division_both->appendChildRepr(dest2));
        }
        
        for (auto& child: operandit_g->children) {
            auto item = cast<SPItem>(&child);
            if (item) {
                fractureit(item, unionpv);
            }
        }
    }
    if (operandit_shape) {
        if (auto c = operandit_shape->curve()) {
            auto curve = *c;
            curve *= i2anc_affine(operandit_shape, sp_lpe_item->parent);
            auto intesect = sp_pathvector_boolop(unionpv, curve, bool_op_inters, fill_a, fill_b);
            Inkscape::XML::Node *dest = dupleNode(operandit_shape, "svg:path");
            dest->setAttribute("d", sp_svg_write_path(intesect));
            dest->setAttribute("transform", nullptr);
            if (!division_other) {
                division_other = cast<SPGroup>(sp_lpe_item->parent);
            }
            auto divisionitem = cast<SPItem>(division_other->appendChildRepr(dest));
            Inkscape::GC::release(dest);
            if (division_other_id.empty()) {
                division_other->reorder(divisionitem, divisionit);
                division_other_id = Glib::ustring(dest->attribute("id"));
            }
            auto operandit_pathv = sp_pathvector_boolop(unionpv, curve, bool_op_diff, fill_a, fill_b);
            Inkscape::XML::Node *dest2 = dupleNode(operandit_shape, "svg:path");
            dest2->setAttribute("transform", nullptr);
            dest2->setAttribute("d", sp_svg_write_path(operandit_pathv));
            if (!division_both) {
                division_both = cast<SPGroup>(sp_lpe_item->parent);
                auto divisionitem2 = cast<SPItem>(division_both->appendChildRepr(dest2));
                division_both->reorder(divisionitem2, divisionitem);
            } else {
                division_both->appendChildRepr(dest2);
            }
            Inkscape::GC::release(dest2);
        }
    }
}

void LPEBool::divisionit(SPObject * operand_a, SPObject * operand_b, Geom::PathVector unionpv)
{
    auto operand_a_item = cast<SPItem>(operand_a);
    auto operand_b_item = cast<SPItem>(operand_b);
    auto operand_b_g = cast<SPGroup>(operand_b);
    auto operand_b_shape = cast<SPShape>(operand_b);
    FillRule fill_a = fill_type_this.get_value() != fill_justDont ? fill_type_this.get_value() : GetFillTyp(operand_a_item);
    FillRule fill_b = fill_type_operand.get_value() != fill_justDont ? fill_type_operand.get_value() : GetFillTyp(operand_b_item);
    if (operand_b_g) {
        Inkscape::XML::Node *dest = dupleNode(operand_b, "svg:g");
        dest->setAttribute("transform", nullptr);
        if (!division) {
            division = cast<SPGroup>(sp_lpe_item->parent->appendChildRepr(dest));
            Inkscape::GC::release(dest);
            division_id = division->getId();
            division->parent->reorder(division, sp_lpe_item);
        } else {
            division = cast<SPGroup>(division->appendChildRepr(dest));
        }
        for (auto& child: operand_b_g->children) {
            auto item = cast<SPItem>(&child);
            if (item) {
                divisionit(operand_a, item, unionpv);
            }
        }
    }
    if (operand_b_shape) {
        if (!division) {
            division = cast<SPGroup>(sp_lpe_item->parent);
        }
        if (auto c = operand_b_shape->curveForEdit()) {
            auto curve = *c;
            curve *= i2anc_affine(operand_b_shape, sp_lpe_item->parent);
            auto intesect = sp_pathvector_boolop(unionpv, curve, bool_op_inters, fill_a, fill_b);
            Inkscape::XML::Node *dest = dupleNode(operand_b_shape, "svg:path");
            dest->setAttribute("d", sp_svg_write_path(intesect));
            dest->setAttribute("transform", nullptr);
            auto item = cast<SPItem>(division->appendChildRepr(dest));
            Inkscape::GC::release(dest);
            if (item && division_id.empty()) {
                division_id = item->getId();
            }
        }
    }
}

void LPEBool::doOnRemove(SPLPEItem const * lpeitem)
{
    // set "keep paths" hook on sp-lpe-item.cpp
    remove_filter(operand_item.getObject());
    auto operand = cast<SPItem>(getSPDoc()->getObjectById(operand_id));
    if (operand) {
        if (keep_paths) {
            bool_op_ex op = bool_operation.get_value();
            if (op == bool_op_ex_cut || op == bool_op_ex_cut_both) {
                reverse = lpeitem->pos_in_parent() > operand->pos_in_parent();
                division = nullptr;
                Geom::PathVector unionpv = get_union(operand, operand);
                divisionit(operand, sp_lpe_item, unionpv);
                onremove = true;
                sp_lpe_item_update_patheffect(sp_lpe_item, false, true);
                if (op == bool_op_ex_cut_both) {
                    auto * a = cast<SPItem>(getSPDoc()->getObjectById(division_id)); 
                    if (a) {
                        unionpv = get_union(sp_lpe_item, sp_lpe_item, true);
                        fractureit(operand, unionpv); 
                        auto * b = cast<SPItem>(getSPDoc()->getObjectById(division_other_id));   
                        if (a && b) {
                            if (reverse) {
                                b->lowerOne();
                            }
                        }
                    }
                }
                // we reset variables because LPE is not removed on undo so I need to get clean on redo
                division = nullptr;
                division_both = nullptr;
                division_other = nullptr;
                operand_id = "";
                division_id = "";
                division_other_id = "";
                onremove = false;
            }
            if (is_visible) {
                processObjects(LPE_ERASE);
            }
        }
    }
}

// TODO: Migrate the tree next function to effect.cpp/h to avoid duplication
void LPEBool::doOnVisibilityToggled(SPLPEItem const * /*lpeitem*/)
{
    auto operand = cast<SPItem>(operand_item.getObject());
    if (operand) {
        if (!is_visible) {
            remove_filter(operand);
        }
    }
}

} // namespace LivePathEffect
} /* namespace Inkscape */
