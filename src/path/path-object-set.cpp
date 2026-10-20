// SPDX-License-Identifier: GPL-2.0-or-later

/** @file
 *
 * Path related functions for ObjectSet.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * TODO: Move related code from path-chemistry.cpp
 *
 */

#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "message-stack.h"
#include "path-boolop.h"
#include "path-chemistry.h"     // copy_object_properties()
#include "path-util.h"
#include "preferences.h"
#include "text-editing.h"

#include "object/object-set.h"
#include "object/sp-flowtext.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "path/path-outline.h"
#include "path/path-simplify.h"
#include "ui/icon-names.h"
#include "xml/repr-sorting.h"
#include "style.h"

static bool item_has_curve(SPItem* item)
{
    return (is<SPShape>(item) || is<SPText>(item) || is<SPFlowtext>(item));
}

using Inkscape::ObjectSet;

void Inkscape::ObjectSet::pathUnion(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_union, INKSCAPE_ICON("path-union"), RC_("Undo", "Union"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathIntersect(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_inters, INKSCAPE_ICON("path-intersection"), RC_("Undo", "Intersection"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathDiff(bool skip_undo, bool silent)
{
    _pathBoolOpIterative(bool_op_diff, INKSCAPE_ICON("path-difference"), RC_("Undo", "Difference"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathSymDiff(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_symdiff, INKSCAPE_ICON("path-exclusion"), RC_("Undo", "Exclusion"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathCut(bool skip_undo, bool silent)
{
    _pathBoolOpIterative(bool_op_cut, INKSCAPE_ICON("path-division"), RC_("Undo", "Division"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathSlice(bool skip_undo, bool silent)
{
    _pathBoolOpIterative(bool_op_slice, INKSCAPE_ICON("path-cut"), RC_("Undo", "Cut path"), skip_undo, silent);
}

// This allows Difference, Divsion, and Cut to work on more than two objects by repeatedly copying the
// top object and applying the operation one-by-one on the objects beneath.
void Inkscape::ObjectSet::_pathBoolOpIterative(BooleanOp bop, char const *icon_name, Inkscape::Util::Internal::ContextString description, bool skip_undo, bool silent)
{
    auto items = items_vector();

    if (items.size() <= 2) {
        // Short circuit all this nonsense
        _pathBoolOp(bop, icon_name, description, skip_undo, silent);
        return;
    }

    std::sort(items.begin(), items.end(), sp_object_compare_position_bool);

    auto xml_doc = document()->getReprDoc();

    auto top = items.back();

    const char* msg = _("Top object must be a shape or text to perform boolean operations.");
    if (!item_has_curve(top)) {
        if (!silent) {
            if (desktop()) {
                desktop()->messageStack()->flash(ERROR_MESSAGE, msg);
            } else {
                g_printerr("%s\n", msg);
            }
        }
        return;
    }

    auto top_repr = top->getRepr();
    items.pop_back();

    bool done = false;
    while (!items.empty()) {
        if (item_has_curve(items.back())) {

            // Duplicate top element and insert it next to original
            auto top_copy = top_repr->duplicate(xml_doc);
            top_repr->parent()->addChild(top_copy, top_repr);
            auto top_copy_item = document()->getObjectByRepr(top_copy);

            // We need to set the curve before passing the new object to pathBoolOp().
            if (auto shape = cast<SPShape>(top_copy_item)) {
                shape->set_shape();
            } else if (is<SPText>(top_copy_item) || is<SPFlowtext>(top_copy_item)) {
                te_update_layout_now_recursive(cast<SPItem>(top_copy_item));
            } else {
                // Should never happen, we tested for curve above.
                std::cerr << "Inkscape::ObjectSet::pathDiff: logic error!" << std::endl;
                continue;
            }

            // Clear selection and add our two objects.
            clear();
            add(items.back()); // Add item
            add(top_copy); // Add repr

            _pathBoolOp(bop, icon_name, description, true, silent);

            done = true;
        }

        // Move on to next item.
        items.pop_back();
    }

    if (done) {
        // _pathBoolOp always removed a copy, we need to remove original (if at least one bool up done).
        top_repr->parent()->removeChild(top_repr);
        if (!skip_undo) {
            DocumentUndo::done(document(), description, icon_name);
        }
    }
}

void Inkscape::ObjectSet::_pathBoolOp(BooleanOp bop, char const *icon_name, Inkscape::Util::Internal::ContextString description, bool skip_undo, bool silent)
{
    try {
        ObjectSet::_pathBoolOp(bop);
        if (!skip_undo) {
            DocumentUndo::done(document(), description, icon_name);
        }
    } catch (char const *msg) {
        if (!silent) {
            if (desktop()) {
                desktop()->messageStack()->flash(ERROR_MESSAGE, msg);
            } else {
                g_printerr("%s\n", msg);
            }
        }
    }
}

void Inkscape::ObjectSet::_pathBoolOp(BooleanOp bop)
{
    auto const doc = document();

    // Grab the items list.
    auto const il = items_vector();

    // Validate number of items.
    switch (bop) {
        case bool_op_union:
            if (il.size() < 1) { // Allow union of single item --> flatten.
                throw _("Select <b>at least 1 path</b> to perform a boolean union.");
            }
            break;
        case bool_op_inters:
        case bool_op_symdiff:
        case bool_op_diff:
        case bool_op_cut:
        case bool_op_slice:
            if (il.size() < 2) {
                throw _("Select <b>at least 2 paths</b> to perform the boolean operation.");
            }
            break;
    }
    assert(!il.empty());

    // reverseOrderForOp marks whether the order of the list is the top->down order
    // it's only used when there are 2 objects, and for operations which need to know the
    // topmost object (differences, cuts)
    bool reverseOrderForOp = false;

    if (bop == bool_op_diff || bop == bool_op_cut || bop == bool_op_slice) {
        // check in the tree to find which element of the selection list is topmost (for 2-operand commands only)
        Inkscape::XML::Node *a = il.front()->getRepr();
        Inkscape::XML::Node *b = il.back()->getRepr();

        if (!a || !b) {
            return;
        }

        if (is_descendant_of(a, b)) {
            // a is a child of b, already in the proper order
        } else if (is_descendant_of(b, a)) {
            // reverse order
            reverseOrderForOp = true;
        } else {

            // objects are not in parent/child relationship;
            // find their lowest common ancestor
            Inkscape::XML::Node *parent = lowest_common_ancestor(a, b);
            if (!parent) {
                return;
            }

            // find the children of the LCA that lead from it to the a and b
            Inkscape::XML::Node *as = find_containing_child(a, parent);
            Inkscape::XML::Node *bs = find_containing_child(b, parent);

            // find out which comes first
            for (Inkscape::XML::Node *child = parent->firstChild(); child; child = child->next()) {
                if (child == as) {
                    /* a first, so reverse. */
                    reverseOrderForOp = true;
                    break;
                }
                if (child == bs)
                    break;
            }
        }
    }

    // First check if all the input objects have curves, otherwise bail out.
    for (auto item : il) {
        if (!item_has_curve(item)) {
            return;
        }
    }

    struct Operand
    {
        // From source objects
        FillRule fill_rule{};
        Geom::PathVector pathv;

        // Computed
        std::vector<Geom::PathVectorTime> cuts;
        std::unique_ptr<Path> path;
    };

    std::vector<Operand> operands;
    operands.resize(il.size());

    // Extract the fill rules and pathvectors from the source objects.
    for (int i = 0; i < il.size(); i++) {
        auto item = il[i];
        auto &operand = operands[i];

        // apply live path effects prior to performing boolean operation
        char const *id = item->getAttribute("id");
        if (auto lpeitem = cast<SPLPEItem>(item)) {
            SPDocument * document = item->document;
            lpeitem->removeAllPathEffects(true);
            SPObject *elemref = document->getObjectById(id);
            if (elemref && elemref != item) {
                // If the LPE item is a shape, it is converted to a path
                // so we need to reupdate the item
                item = cast<SPItem>(elemref);
            }
        }

        // Get the fill rule.
        if (item->style->fill_rule.computed == SP_WIND_RULE_EVENODD) {
            operand.fill_rule = fill_oddEven;
        } else {
            operand.fill_rule = fill_nonZero;
        }

        // Get the pathvector.
        auto curve = curve_for_item(item);
        if (!curve) {
            std::cerr << "Inkscape::ObjectSet::_pathBoolOp(BooleanOp bop): no curve: "
                      << (item->getId() ? item->getId() : "no id") << std::endl;
            return;
        }
        operand.pathv = *curve * item->i2doc_affine();
    }

    // Compute the intersections and self-intersections, and use this information when converting to livarot paths.
    for (int i = 0; i < operands.size(); i++) {
        for (int j = 0; j < i; j++) {
            distribute_intersection_times(operands[i].cuts, operands[j].cuts, operands[i].pathv.intersect(operands[j].pathv));
        }
    }

    for (auto &operand : operands) {
        distribute_intersection_times(operand.cuts, operand.cuts, operand.pathv.intersectSelf());
        sort_and_clean_intersection_times(operand.cuts);

        operand.path = std::make_unique<Path>();
        operand.path->LoadPathVector(operand.pathv, operand.cuts);
        operand.path->ConvertWithBackData(RELATIVE_THRESHOLD, true);

        if (operand.path->descr_cmd.size() <= 1) {
            return;
        }
    }

    // reverse if needed
    // note that the selection list keeps its order
    if (reverseOrderForOp) {
        std::swap(operands[0], operands[1]);
    }

    // and work
    // some temporary instances, first
    Shape *theShapeA = new Shape;
    Shape *theShapeB = new Shape;
    Shape *theShape = new Shape;
    Path *res = new Path;
    Path::cut_position  *toCut=nullptr;
    int                  nbToCut=0;

    if (bop == bool_op_inters || bop == bool_op_union || bop == bool_op_diff || bop == bool_op_symdiff) {
        // true boolean op
        // get the polygons of each path, with the winding rule specified, and apply the operation iteratively

        operands[0].path->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, operands[0].fill_rule);

        for (int i = 1; i < operands.size(); i++) {
            operands[i].path->Fill(theShape, i);

            theShapeB->ConvertToShape(theShape, operands[i].fill_rule);

            /* Due to quantization of the input shape coordinates, we may end up with A or B being empty.
             * If this is a union or symdiff operation, we just use the non-empty shape as the result:
             *   A=0  =>  (0 or B) == B
             *   B=0  =>  (A or 0) == A
             *   A=0  =>  (0 xor B) == B
             *   B=0  =>  (A xor 0) == A
             * If this is an intersection operation, we just use the empty shape as the result:
             *   A=0  =>  (0 and B) == 0 == A
             *   B=0  =>  (A and 0) == 0 == B
             * If this a difference operation, and the upper shape (A) is empty, we keep B.
             * If the lower shape (B) is empty, we still keep B, as it's empty:
             *   A=0  =>  (B - 0) == B
             *   B=0  =>  (0 - A) == 0 == B
             *
             * In any case, the output from this operation is stored in shape A, so we may apply
             * the above rules simply by judicious use of swapping A and B where necessary.
             */
            bool zeroA = theShapeA->numberOfEdges() == 0;
            bool zeroB = theShapeB->numberOfEdges() == 0;
            if (zeroA || zeroB) {
                // We might need to do a swap. Apply the above rules depending on operation type.
                bool resultIsB =   ((bop == bool_op_union || bop == bool_op_symdiff) && zeroA)
                                   || ((bop == bool_op_inters) && zeroB)
                                   ||  (bop == bool_op_diff);
                if (resultIsB) {
                    // Swap A and B to use B as the result
                    std::swap(theShapeA, theShapeB);
                }
            } else {
                // Just do the Boolean operation as usual
                // les elements arrivent en ordre inverse dans la liste
                theShape->Booleen(theShapeB, theShapeA, bop);
                std::swap(theShape, theShapeA);
            }
        }

        std::swap(theShape, theShapeA);

    } else if (bop == bool_op_cut) {
        // cuts= sort of a bastard boolean operation, thus not the axact same modus operandi
        // technically, the cut path is not necessarily a polygon (thus has no winding rule)
        // it is just uncrossed, and cleaned from duplicate edges and points
        // then it's fed to Booleen() which will uncross it against the other path
        // then comes the trick: each edge of the cut path is duplicated (one in each direction),
        // thus making a polygon. the weight of the edges of the cut are all 0, but
        // the Booleen need to invert the ones inside the source polygon (for the subsequent
        // ConvertToForme)

        // the cut path needs to have the highest pathID in the back data
        // that's how the Booleen() function knows it's an edge of the cut
        std::swap(operands[0], operands[1]);

        operands[0].path->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, operands[0].fill_rule);

        operands[1].path->Fill(theShape, 1, false, is_line(*operands[1].path), false);

        theShapeB->ConvertToShape(theShape, fill_justDont);

        // les elements arrivent en ordre inverse dans la liste
        theShape->Booleen(theShapeB, theShapeA, bool_op_cut, 1);

    } else if (bop == bool_op_slice) {
        // slice is not really a boolean operation
        // you just put the 2 shapes in a single polygon, uncross it
        // the points where the degree is > 2 are intersections
        // just check it's an intersection on the path you want to cut, and keep it
        // the intersections you have found are then fed to ConvertPositionsToMoveTo() which will
        // make new subpath at each one of these positions
        // inversion pour l'opération
        std::swap(operands[0], operands[1]);

        operands[0].path->Fill(theShapeA, 0, false, false, false); // don't closeIfNeeded

        operands[1].path->Fill(theShapeA, 1, true, false, false); // don't closeIfNeeded and just dump in the shape, don't reset it

        theShape->ConvertToShape(theShapeA, fill_justDont);

        if (theShape->hasBackData()) {
            // should always be the case, but ya never know
            {
                for (int i = 0; i < theShape->numberOfPoints(); i++) {
                    if ( theShape->getPoint(i).totalDegree() > 2 ) {
                        // possibly an intersection
                        // we need to check that at least one edge from the source path is incident to it
                        // before we declare it's an intersection
                        int cb = theShape->getPoint(i).incidentEdge[FIRST];
                        int   nbOrig=0;
                        int   nbOther=0;
                        int   piece=-1;
                        float t=0.0;
                        while ( cb >= 0 && cb < theShape->numberOfEdges() ) {
                            if ( theShape->ebData[cb].pathID == 0 ) {
                                // the source has an edge incident to the point, get its position on the path
                                piece=theShape->ebData[cb].pieceID;
                                if ( theShape->getEdge(cb).st == i ) {
                                    t=theShape->ebData[cb].tSt;
                                } else {
                                    t=theShape->ebData[cb].tEn;
                                }
                                nbOrig++;
                            }
                            if ( theShape->ebData[cb].pathID == 1 ) nbOther++; // the cut is incident to this point
                            cb=theShape->NextAt(i, cb);
                        }
                        if ( nbOrig > 0 && nbOther > 0 ) {
                            // point incident to both path and cut: an intersection
                            // note that you only keep one position on the source; you could have degenerate
                            // cases where the source crosses itself at this point, and you wouyld miss an intersection
                            toCut=(Path::cut_position*)realloc(toCut, (nbToCut+1)*sizeof(Path::cut_position));
                            toCut[nbToCut].piece=piece;
                            toCut[nbToCut].t=t;
                            nbToCut++;
                        }
                    }
                }
            }
            {
                // i think it's useless now
                int i = theShape->numberOfEdges() - 1;
                for (;i>=0;i--) {
                    if ( theShape->ebData[i].pathID == 1 ) {
                        theShape->SubEdge(i);
                    }
                }
            }

        }
    }

    auto get_path_arr = [&] {
        std::vector<Path *> result;
        result.reserve(operands.size());
        for (auto &operand : operands) {
            result.emplace_back(operand.path.get());
        }
        return result;
    };

    int*    nesting=nullptr;
    int*    conts=nullptr;
    int     nbNest=0;
    // pour compenser le swap juste avant
    if (bop == bool_op_slice) {
        res->Copy(operands[0].path.get());
        res->ConvertPositionsToMoveTo(nbToCut, toCut); // cut where you found intersections
        free(toCut);
    } else if (bop == bool_op_cut) {
        // il faut appeler pour desallouer PointData (pas vital, mais bon)
        // the Booleen() function did not deallocate the point_data array in theShape, because this
        // function needs it.
        // this function uses the point_data to get the winding number of each path (ie: is a hole or not)
        // for later reconstruction in objects, you also need to extract which path is parent of holes (nesting info)
        theShape->ConvertToFormeNested(res, operands.size(), get_path_arr().data(), nbNest, nesting, conts, true);
    } else {
        theShape->ConvertToForme(res, operands.size(), get_path_arr().data());
    }

    delete theShape;
    delete theShapeA;
    delete theShapeB;

    if (res->descr_cmd.size() <= 1) {
        // only one command, presumably a moveto: it isn't a path
        for (auto l : il){
            l->deleteObject();
        }
        clear();

        delete res;
        return;
    }

    // get the source path object
    SPObject *source;
    if ( bop == bool_op_diff || bop == bool_op_cut || bop == bool_op_slice ) {
        if (reverseOrderForOp) {
            source = il[0];
        } else {
            source = il.back();
        }
    } else {
        // find out the bottom object
        auto sorted = xmlNodes_vector();
        std::sort(sorted.begin(), sorted.end(), sp_repr_compare_position_bool);
        source = doc->getObjectByRepr(sorted.front());
    }

    // adjust style properties that depend on a possible transform in the source object in order
    // to get a correct style attribute for the new path
    auto item_source = cast<SPItem>(source);
    Geom::Affine i2doc(item_source->i2doc_affine());

    Inkscape::XML::Node *repr_source = source->getRepr();

    // remember important aspects of the source path, to be restored
    gint pos = repr_source->position();
    Inkscape::XML::Node *parent = repr_source->parent();
    // remove source paths
    clear();
    for (auto l : il){
        if (l != item_source) {
            // delete the object for real, so that its clones can take appropriate action
            l->deleteObject();
        }
    }

    auto const source2doc_inverse = i2doc.inverse();
    char const *const old_transform_attibute = repr_source->attribute("transform");

    // now that we have the result, add it on the canvas
    if ( bop == bool_op_cut || bop == bool_op_slice ) {
        int    nbRP=0;
        Path** resPath;
        if ( bop == bool_op_slice ) {
            // there are moveto's at each intersection, but it's still one unique path
            // so break it down and add each subpath independently
            // we could call break_apart to do this, but while we have the description...
            resPath=res->SubPaths(nbRP, false);
        } else {
            // cut operation is a bit wicked: you need to keep holes
            // that's why you needed the nesting
            // ConvertToFormeNested() dumped all the subpath in a single Path "res", so we need
            // to get the path for each part of the polygon. that's why you need the nesting info:
            // to know in which subpath to add a subpath
            resPath=res->SubPathsWithNesting(nbRP, true, nbNest, nesting, conts);

            // cleaning
            if ( conts ) free(conts);
            if ( nesting ) free(nesting);
        }

        // add all the pieces resulting from cut or slice
        std::vector <Inkscape::XML::Node*> selection;
        for (int i=0;i<nbRP;i++) {
            resPath[i]->Transform(source2doc_inverse);

            Inkscape::XML::Document *xml_doc = doc->getReprDoc();
            Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
            Inkscape::copy_object_properties(repr, repr_source);

            // Delete source on last iteration (after we don't need repr_source anymore). As a consequence, the last
            // item will inherit the original's id.
            if (i + 1 == nbRP) {
                item_source->deleteObject(false);
            }

            repr->setAttribute("d", resPath[i]->svg_dump_path().c_str());

            // for slice, remove fill
            if (bop == bool_op_slice) {
                SPCSSAttr *css;

                css = sp_repr_css_attr_new();
                sp_repr_css_set_property(css, "fill", "none");

                sp_repr_css_change(repr, css, "style");

                sp_repr_css_attr_unref(css);
            }

            repr->setAttributeOrRemoveIfEmpty("transform", old_transform_attibute);

            // add the new repr to the parent
            // move to the saved position
            parent->addChildAtPos(repr, pos);

            selection.push_back(repr);
            Inkscape::GC::release(repr);

            delete resPath[i];
        }
        setReprList(selection);
        if ( resPath ) free(resPath);

    } else {
        res->Transform(source2doc_inverse);

        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

        Inkscape::copy_object_properties(repr, repr_source);

        // delete it so that its clones don't get alerted; this object will be restored shortly, with the same id
        item_source->deleteObject(false);

        repr->setAttribute("d", res->svg_dump_path().c_str());

        repr->setAttributeOrRemoveIfEmpty("transform", old_transform_attibute);

        parent->addChildAtPos(repr, pos);

        set(repr);
        Inkscape::GC::release(repr);
    }

    delete res;
}

bool ObjectSet::strokesToPaths(bool legacy, bool skip_undo)
{
    if (desktop() && isEmpty()) {
        desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>stroked path(s)</b> to convert stroke to path."));
        return false;
    }

    bool did = false;

    auto prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/options/pathoperationsunlink/value", true)) {
        did = unlinkRecursive(true);
    }

    // Need to turn on stroke scaling to ensure stroke is scaled when transformed!
    bool scale_stroke = prefs->getBool("/options/transform/stroke", true);
    prefs->setBool("/options/transform/stroke", true);

    for (auto item : items_vector()) {
        // Do not remove the object from the selection here
        // as we want to keep it selected if the whole operation fails
        Inkscape::XML::Node *new_node = item_to_paths(item, legacy);
        if (new_node) {
            SPObject* new_item = document()->getObjectByRepr(new_node);

            add(new_item); // Add to selection.
            did = true;
        }
    }

    // Reset
    prefs->setBool("/options/transform/stroke", scale_stroke);

    if (desktop() && !did) {
        desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No stroked paths</b> in the selection."));
    }

    if (did && !skip_undo) {
        Inkscape::DocumentUndo::done(document(), RC_("Undo", "Convert stroke to path"), "");
    } else if (!did && !skip_undo) {
        Inkscape::DocumentUndo::cancel(document());
    }

    return did;
}

bool
ObjectSet::simplifyPaths(bool skip_undo)
{
    if (desktop() && isEmpty()) {
        desktop()->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>path(s)</b> to simplify."));
        return false;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double threshold = prefs->getDouble("/options/simplifythreshold/value", 0.003);
    bool justCoalesce = prefs->getBool(  "/options/simplifyjustcoalesce/value", false);

    // Keep track of accelerated simplify
    static gint64 previous_time = 0;
    static gdouble multiply = 1.0;

    // Get the current time
    gint64 current_time = g_get_monotonic_time();

    // Was the previous call to this function recent? (<0.5 sec)
    if (previous_time > 0 && current_time - previous_time < 500000) {

        // add to the threshold 1/2 of its original value
        multiply  += 0.5;
        threshold *= multiply;

    } else {
        // reset to the default
        multiply = 1;
    }

    // Remember time for next call
    previous_time = current_time;

    // set "busy" cursor
    if (desktop()) {
        desktop()->setWaitingCursor();
    }

    Geom::OptRect selectionBbox = visualBounds();
    if (!selectionBbox) {
        std::cerr << "ObjectSet::: selection has no visual bounding box!" << std::endl;
        return false;
    }
    double size = L2(selectionBbox->dimensions());

    int pathsSimplified = 0;
    for (auto item : items_vector()) {
        pathsSimplified += path_simplify(item, threshold, justCoalesce, size);
    }

    if (pathsSimplified > 0 && !skip_undo) {
        DocumentUndo::done(document(), RC_("Undo", "Simplify"), INKSCAPE_ICON("path-simplify"));
    }

    if (desktop()) {
        desktop()->clearWaitingCursor();
        if (pathsSimplified > 0) {
            desktop()->messageStack()->flashF(Inkscape::NORMAL_MESSAGE, _("<b>%d</b> paths simplified."), pathsSimplified);
        } else {
            desktop()->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No paths</b> to simplify in the selection."));
        }
    }

    return (pathsSimplified > 0);
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
