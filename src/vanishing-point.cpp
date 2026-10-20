// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Vanishing point for 3D perspectives
 *
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Maximilian Albert <Anhalter42@gmx.de>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2005-2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "vanishing-point.h"

#include "desktop.h"
#include "document-undo.h"
#include "perspective-line.h"
#include "snap.h"

#include "display/control/canvas-item-curve.h"

#include "object/sp-namedview.h"
#include "object/box3d.h"

#include "ui/icon-names.h"
#include "ui/knot/knot.h"
#include "ui/shape-editor.h"
#include "ui/tools/tool-base.h"

using Inkscape::DocumentUndo;

namespace Box3D {

// screen pixels between knots when they snap:
#define SNAP_DIST 5

// absolute distance between gradient points for them to become a single dragger when the drag is created:
#define MERGE_DIST 0.1

static void vp_drag_sel_changed(Inkscape::Selection * /*selection*/, gpointer data)
{
    VPDrag *drag = (VPDrag *)data;
    drag->updateDraggers();
    drag->updateLines();
    drag->updateBoxReprs();
}

static void vp_drag_sel_modified(Inkscape::Selection * /*selection*/, guint /*flags*/, gpointer data)
{
    VPDrag *drag = (VPDrag *)data;
    drag->updateLines();
    // drag->updateBoxReprs();
    drag->updateBoxHandles(); // FIXME: Only update the handles of boxes on this dragger (not on all)
    drag->updateDraggers();
}

static bool have_VPs_of_same_perspective(VPDragger *dr1, VPDragger *dr2)
{
    for (auto & vp : dr1->vps) {
        if (dr2->hasPerspective(vp.get_perspective())) {
            return true;
        }
    }
    return false;
}

static void vp_knot_moved_handler(SPKnot *knot, Geom::Point const &ppointer, guint state, gpointer data)
{
    VPDragger *dragger = (VPDragger *)data;
    VPDrag *drag = dragger->parent;

    Geom::Point p = ppointer;

    // FIXME: take from prefs
    double snap_dist = SNAP_DIST / SP_ACTIVE_DESKTOP->current_zoom();

    /*
     * We use dragging_started to indicate if we have already checked for the need to split Draggers up.
     * This only has the purpose of avoiding costly checks in the routine below.
     */
    if (!dragger->dragging_started && (state & GDK_SHIFT_MASK)) {
        /* with Shift; if there is more than one box linked to this VP
           we need to split it and create a new perspective */
        if (dragger->numberOfBoxes() > 1) { // FIXME: Don't do anything if *all* boxes of a VP are selected
            std::set<VanishingPoint *> sel_vps = dragger->VPsOfSelectedBoxes();

            for (auto sel_vp : sel_vps) {
                // for each VP that has selected boxes:
                Persp3D *old_persp = sel_vp->get_perspective();
                auto sel_boxes = sel_vp->selectedBoxes(SP_ACTIVE_DESKTOP->getSelection());

                // create a copy of the existing perspective
                Persp3D *new_persp = Persp3D::create_copy(*old_persp);

                /* ... unlink unselected boxes of each VP so that
                          the selected boxes are kept with the VP being moved */
                for (auto *box : old_persp->list_of_boxes()) {
                    if (!sel_boxes.contains(box)) {
                        /* if a box in the VP is unselected, move it to the
                           newly created perspective so that it doesn't get dragged */
                        box->switch_perspectives(old_persp, new_persp);
                    }
                }
            }
            // FIXME: Do we need to create a new dragger as well?
            dragger->updateZOrders();
            DocumentUndo::done(SP_ACTIVE_DESKTOP->getDocument(), RC_("Undo", "Split vanishing points"), INKSCAPE_ICON("draw-cuboid"));
            return;
        }
    }

    if (!(state & GDK_SHIFT_MASK)) {
        // without Shift; see if we need to snap to another dragger
        for (std::vector<VPDragger *>::const_iterator di = dragger->parent->draggers.begin();
             di != dragger->parent->draggers.end(); ++di) {
            VPDragger *d_new = *di;
            if ((d_new != dragger) && (Geom::L2(d_new->point - p) < snap_dist)) {
                if (have_VPs_of_same_perspective(dragger, d_new)) {
                    // this would result in degenerate boxes, which we disallow for the time being
                    continue;
                }

                // update positions ... (this is needed so that the perspectives are detected as identical)
                // FIXME: This is called a bit too often, isn't it?
                for (auto & vp : dragger->vps) {
                    vp.set_pos(d_new->point);
                }

                // ... join lists of VPs ...
                d_new->vps.merge(dragger->vps);

                // ... delete old dragger ...
                drag->draggers.erase(std::remove(drag->draggers.begin(), drag->draggers.end(), dragger),
                                     drag->draggers.end());
                delete dragger;
                dragger = nullptr;

                // ... and merge any duplicate perspectives
                d_new->mergePerspectives();

                // TODO: Update the new merged dragger
                d_new->updateTip();

                d_new->parent->updateBoxDisplays(); // FIXME: Only update boxes in current dragger!
                d_new->updateZOrders();

                drag->updateLines();

                // TODO: Undo machinery; this doesn't work yet because perspectives must be created and
                //       deleted according to changes in the svg representation, not based on any user input
                //       as is currently the case.

                DocumentUndo::done(SP_ACTIVE_DESKTOP->getDocument(), RC_("Undo", "Merge vanishing points"), INKSCAPE_ICON("draw-cuboid"));

                return;
            }
        }
    }

    // We didn't hit the return statement above, so we didn't snap to another dragger. Therefore we'll now try a regular
    // snap
    // Regardless of the status of the SHIFT key, we will try to snap; Here SHIFT does not disable snapping, as the
    // shift key
    // has a different purpose in this context (see above)
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    auto &m = desktop->getNamedView()->snap_manager;
    m.setup(desktop);
    Inkscape::SnappedPoint s = m.freeSnap(Inkscape::SnapCandidatePoint(p, Inkscape::SNAPSOURCE_OTHER_HANDLE));
    m.unSetup();
    if (s.getSnapped()) {
        p = s.getPoint();
        knot->moveto(p);
    }

    dragger->point = p; // FIXME: Is dragger->point being used at all?

    dragger->updateVPs(p);
    dragger->updateBoxDisplays();
    dragger->parent->updateBoxHandles(); // FIXME: Only update the handles of boxes on this dragger (not on all)
    dragger->updateZOrders();

    drag->updateLines();

    dragger->dragging_started = true;
}

static void vp_knot_grabbed_handler(SPKnot * /*knot*/, unsigned int /*state*/, gpointer data)
{
    VPDragger *dragger = (VPDragger *)data;
    VPDrag *drag = dragger->parent;

    drag->dragging = true;
}

static void vp_knot_ungrabbed_handler(SPKnot *knot, guint /*state*/, gpointer data)
{
    VPDragger *dragger = (VPDragger *)data;

    dragger->point_original = dragger->point = knot->pos;

    dragger->dragging_started = false;

    for (auto & vp : dragger->vps) {
        vp.set_pos(knot->pos);
        vp.updateBoxReprs();
        vp.updatePerspRepr();
    }

    dragger->parent->updateDraggers();
    dragger->parent->updateLines();
    dragger->parent->updateBoxHandles();

    // TODO: Update box's paths and svg representation

    dragger->parent->dragging = false;

    // TODO: Undo machinery!!
    g_return_if_fail(dragger->parent);
    g_return_if_fail(dragger->parent->document);
    DocumentUndo::done(dragger->parent->document, RC_("Undo", "3D box: Move vanishing point"), INKSCAPE_ICON("draw-cuboid"));
}

unsigned int VanishingPoint::global_counter = 0;

// FIXME: Rename to something more meaningful!
void VanishingPoint::set_pos(Proj::Pt2 const &pt)
{
    g_return_if_fail(_persp);
    _persp->perspective_impl->tmat.set_image_pt(_axis, pt);
}

std::unordered_set<SPBox3D *> VanishingPoint::selectedBoxes(Inkscape::Selection *sel)
{
    std::unordered_set<SPBox3D *> sel_boxes;
    auto itemlist = sel->items();
    for (auto item : itemlist) {
        auto *box = cast<SPBox3D>(item);
        if (box && this->hasBox(box)) {
            sel_boxes.emplace(box);
        }
    }
    return sel_boxes;
}

VPDragger::VPDragger(VPDrag *parent, Geom::Point p, VanishingPoint &vp)
    : parent(parent)
    , knot(nullptr)
    , point(p)
    , point_original(p)
    , dragging_started(false)
    , vps()
{
    if (vp.is_finite()) {
        // create the knot
        this->knot = new SPKnot(SP_ACTIVE_DESKTOP, "", Inkscape::CANVAS_ITEM_CTRL_TYPE_ANCHOR, "CanvasItemCtrl:VPDragger");
        this->knot->updateCtrl();

        // move knot to the given point
        this->knot->setPosition(this->point, SP_KNOT_STATE_NORMAL);
        this->knot->show();

        // connect knot's signals
        this->_moved_connection =
            this->knot->moved_signal.connect(sigc::bind(sigc::ptr_fun(vp_knot_moved_handler), this));
        this->_grabbed_connection =
            this->knot->grabbed_signal.connect(sigc::bind(sigc::ptr_fun(vp_knot_grabbed_handler), this));
        this->_ungrabbed_connection =
            this->knot->ungrabbed_signal.connect(sigc::bind(sigc::ptr_fun(vp_knot_ungrabbed_handler), this));

        // add the initial VP (which may be NULL!)
        this->addVP(vp);
    }
}

VPDragger::~VPDragger()
{
    // disconnect signals
    this->_moved_connection.disconnect();
    this->_grabbed_connection.disconnect();
    this->_ungrabbed_connection.disconnect();

    // unref should call destroy
    SPKnot::unref(knot);
}

/**
Updates the statusbar tip of the dragger knot, based on its draggables
 */
void VPDragger::updateTip()
{
    char *tip = nullptr;

    guint num = this->numberOfBoxes();
    if (this->vps.size() == 1) {
        if (this->vps.front().is_finite()) {
            tip = g_strdup_printf(ngettext("<b>Finite</b> vanishing point shared by the box",
                                           "<b>Finite</b> vanishing point shared by <b>%d</b> boxes; drag "
                                           "with <b>Shift</b> to separate selected box(es)",
                                           num),
                                  num);
        }
        else {
            // This won't make sense any more when infinite VPs are not shown on the canvas,
            // but currently we update the status message anyway
            tip = g_strdup_printf(ngettext("<b>Infinite</b> vanishing point shared by the box",
                                           "<b>Infinite</b> vanishing point shared by <b>%d</b> boxes; "
                                           "drag with <b>Shift</b> to separate selected box(es)",
                                           num),
                                  num);
        }
    }
    else {
        int length = this->vps.size();
        char const *tmpl = ngettext("Collection of <b>%d</b> vanishing points shared by the box; "
                                    "drag with <b>Shift</b> to separate",
                                    "Collection of <b>%d</b> vanishing points shared by <b>%d</b> boxes; "
                                    "drag with <b>Shift</b> to separate",
                                    num);
        tip = g_strdup_printf(tmpl, length, num);
    }

    knot->setTip(tip);
    g_free(tip);
}

/**
 * Adds a vanishing point to the dragger (also updates the position if necessary);
 * the perspective is stored separately, too, for efficiency in updating boxes.
 */
void VPDragger::addVP(VanishingPoint &vp, bool update_pos)
{
    if (!vp.is_finite() || std::find(vps.begin(), vps.end(), vp) != vps.end()) {
        // don't add infinite VPs; don't add the same VP twice
        return;
    }

    if (update_pos) {
        vp.set_pos(this->point);
    }
    this->vps.push_front(vp);

    this->updateTip();
}

void VPDragger::removeVP(VanishingPoint const &vp)
{
    std::list<VanishingPoint>::iterator i = std::find(this->vps.begin(), this->vps.end(), vp);
    if (i != this->vps.end()) {
        this->vps.erase(i);
    }
    this->updateTip();
}

VanishingPoint *VPDragger::findVPWithBox(SPBox3D *box)
{
    for (auto & vp : vps) {
        if (vp.hasBox(box)) {
            return &vp;
        }
    }
    return nullptr;
}

std::set<VanishingPoint *> VPDragger::VPsOfSelectedBoxes()
{
    std::set<VanishingPoint *> sel_vps;
    VanishingPoint *vp;
    // FIXME: Should we take the selection from the parent VPDrag? I guess it shouldn't make a difference.
    Inkscape::Selection *sel = SP_ACTIVE_DESKTOP->getSelection();
    auto itemlist = sel->items();
    for (auto item : itemlist) {
        auto box = cast<SPBox3D>(item);
        if (box) {
            vp = this->findVPWithBox(box);
            if (vp) {
                sel_vps.insert(vp);
            }
        }
    }
    return sel_vps;
}

guint VPDragger::numberOfBoxes()
{
    guint num = 0;
    for (auto & vp : vps) {
        num += vp.numberOfBoxes();
    }
    return num;
}

bool VPDragger::hasPerspective(const Persp3D *persp)
{
    for (auto & vp : vps) {
        if (persp->perspectives_coincide(vp.get_perspective())) {
            return true;
        }
    }
    return false;
}

void VPDragger::mergePerspectives()
{
    Persp3D *persp1, *persp2;
    for (std::list<VanishingPoint>::iterator i = vps.begin(); i != vps.end(); ++i) {
        persp1 = (*i).get_perspective();
        for (std::list<VanishingPoint>::iterator j = i; j != vps.end(); ++j) {
            persp2 = (*j).get_perspective();
            if (persp1 == persp2) {
                /* don't merge a perspective with itself */
                continue;
            }
            if (persp1->perspectives_coincide(persp2)) {
                /* if perspectives coincide but are not the same, merge them */
                persp1->absorb(persp2);

                this->parent->swap_perspectives_of_VPs(persp2, persp1);

                persp2->deleteObject(false);
            }
        }
    }
}

void VPDragger::updateBoxDisplays()
{
    for (auto & vp : this->vps) {
        vp.updateBoxDisplays();
    }
}

void VPDragger::updateVPs(Geom::Point const &pt)
{
    for (auto & vp : this->vps) {
        vp.set_pos(pt);
    }
}

void VPDragger::updateZOrders()
{
    for (auto & vp : this->vps) {
        vp.get_perspective()->update_z_orders();
    }
}

void VPDragger::printVPs()
{
    g_print("VPDragger at position (%f, %f):\n", point[Geom::X], point[Geom::Y]);
    for (auto & vp : this->vps) {
        g_print("    VP %s\n", vp.axisString());
    }
}

VPDrag::VPDrag(SPDocument *document)
{
    this->document = document;
    this->selection = SP_ACTIVE_DESKTOP->getSelection();

    this->show_lines = true;
    this->front_or_rear_lines = 0x1;

    this->dragging = false;

    this->sel_changed_connection =
        this->selection->connectChanged(sigc::bind(sigc::ptr_fun(&vp_drag_sel_changed), (gpointer) this)

                                            );
    this->sel_modified_connection =
        this->selection->connectModified(sigc::bind(sigc::ptr_fun(&vp_drag_sel_modified), (gpointer) this));

    this->updateDraggers();
    this->updateLines();
}

VPDrag::~VPDrag()
{
    this->sel_changed_connection.disconnect();
    this->sel_modified_connection.disconnect();

    for (auto dragger : this->draggers) {
        delete dragger;
    }
    this->draggers.clear();

    item_curves.clear();
}

/**
 * Select the dragger that has the given VP.
 */
VPDragger *VPDrag::getDraggerFor(VanishingPoint const &vp)
{
    for (auto dragger : this->draggers) {
        for (std::list<VanishingPoint>::iterator j = dragger->vps.begin(); j != dragger->vps.end(); ++j) {
            // TODO: Should we compare the pointers or the VPs themselves!?!?!?!
            if (*j == vp) {
                return (dragger);
            }
        }
    }
    return nullptr;
}

void VPDrag::printDraggers()
{
    g_print("=== VPDrag info: =================================\n");
    for (auto dragger : this->draggers) {
        dragger->printVPs();
        g_print("========\n");
    }
    g_print("=================================================\n");
}

/**
 * Regenerates the draggers list from the current selection; is called when selection is changed or modified
 */
void VPDrag::updateDraggers()
{
    if (this->dragging)
        return;
    // delete old draggers
    for (auto dragger : this->draggers) {
        delete dragger;
    }
    this->draggers.clear();

    g_return_if_fail(this->selection != nullptr);

    auto itemlist = this->selection->items();
    for (auto item : itemlist) {
        auto box = cast<SPBox3D>(item);
        if (box) {
            VanishingPoint vp;
            for (int i = 0; i < 3; ++i) {
                vp.set(box->get_perspective(), Proj::axes[i]);
                addDragger(vp);
            }
        }
    }
}

/**
Regenerates the lines list from the current selection; is called on each move
of a dragger, so that lines are always in sync with the actual perspective
*/
void VPDrag::updateLines()
{
    // Delete old lines
    item_curves.clear();

    // do nothing if perspective lines are currently disabled
    if (this->show_lines == 0)
        return;

    g_return_if_fail(this->selection != nullptr);

    auto itemlist = this->selection->items();
    for (auto item : itemlist) {
        auto box = cast<SPBox3D>(item);
        if (box) {
            this->drawLinesForFace(box, Proj::X);
            this->drawLinesForFace(box, Proj::Y);
            this->drawLinesForFace(box, Proj::Z);
        }
    }
}

void VPDrag::updateBoxHandles()
{
    // FIXME: Is there a way to update the knots without accessing the
    //        (previously) statically linked function KnotHolder::update_knots?

    auto sel = selection->items();
    if (sel.empty())
        return; // no selection

    if (std::ranges::distance(sel) > 1) {
        // Currently we only show handles if a single box is selected
        return;
    }

    auto const tool = SP_ACTIVE_DESKTOP->getTool();
    g_assert(tool);
    if (tool->shape_editor) {
        tool->shape_editor->update_knotholder();
    }
}

void VPDrag::updateBoxReprs()
{
    for (auto dragger : this->draggers) {
        for (auto & vp : dragger->vps) {
            vp.updateBoxReprs();
        }
    }
}

void VPDrag::updateBoxDisplays()
{
    for (auto dragger : this->draggers) {
        for (auto & vp : dragger->vps) {
            vp.updateBoxDisplays();
        }
    }
}


/**
 * Depending on the value of all_lines, draw the front and/or rear perspective lines starting from the given corners.
 */
void VPDrag::drawLinesForFace(const SPBox3D *box,
                              Proj::Axis axis) //, guint corner1, guint corner2, guint corner3, guint corner4)
{
    Inkscape::CanvasItemColor type = Inkscape::CANVAS_ITEM_PRIMARY;
    switch (axis) {
        // TODO: Make color selectable by user
        case Proj::X:
            type = Inkscape::CANVAS_ITEM_SECONDARY;
            break;
        case Proj::Y:
            type = Inkscape::CANVAS_ITEM_PRIMARY;
            break;
        case Proj::Z:
            type = Inkscape::CANVAS_ITEM_TERTIARY;;
            break;
        default:
            g_assert_not_reached();
    }

    const size_t NUM_CORNERS = 4;
    Geom::Point corners[NUM_CORNERS];
    box->corners_for_PLs(axis, corners[0], corners[1], corners[2], corners[3]);

    g_return_if_fail(box->get_perspective());
    Proj::Pt2 vp = box->get_perspective()->get_VP(axis);
    if (vp.is_finite()) {
        // draw perspective lines for finite VPs
        Geom::Point pt = vp.affine();
        if (this->front_or_rear_lines & 0x1) {
            // draw 'front' perspective lines
            this->addCurve(corners[0], pt, type);
            this->addCurve(corners[1], pt, type);
        }
        if (this->front_or_rear_lines & 0x2) {
            // draw 'rear' perspective lines
            this->addCurve(corners[2], pt, type);
            this->addCurve(corners[3], pt, type);
        }
    }
    else {
        // draw perspective lines for infinite VPs
        std::optional<Geom::Point> pts[NUM_CORNERS];
        Persp3D *persp = box->get_perspective();
        SPDesktop *desktop = SP_ACTIVE_DESKTOP; // FIXME: Store the desktop in VPDrag

        for (size_t i = 0; i < NUM_CORNERS; i++) {
            Box3D::PerspectiveLine pl(corners[i], axis, persp);
            if (!(pts[i] = pl.intersection_with_viewbox(desktop))) {
                // some perspective lines are outside the canvas; currently we don't draw any of them
                return;
            }
        }
        if (this->front_or_rear_lines & 0x1) {
            // draw 'front' perspective lines
            this->addCurve(corners[0], *pts[0], type);
            this->addCurve(corners[1], *pts[1], type);
        }
        if (this->front_or_rear_lines & 0x2) {
            // draw 'rear' perspective lines
            this->addCurve(corners[2], *pts[2], type);
            this->addCurve(corners[3], *pts[3], type);
        }
    }
}

/**
 * If there already exists a dragger within MERGE_DIST of p, add the VP to it;
 * otherwise create new dragger and add it to draggers list
 * We also store the corresponding perspective in case it is not already present.
 */
void VPDrag::addDragger(VanishingPoint &vp)
{
    if (!vp.is_finite()) {
        // don't create draggers for infinite vanishing points
        return;
    }
    Geom::Point p = vp.get_pos();

    for (auto dragger : this->draggers) {
        if (Geom::L2(dragger->point - p) < MERGE_DIST) {
            // distance is small, merge this draggable into dragger, no need to create new dragger
            dragger->addVP(vp);
            return;
        }
    }

    VPDragger *new_dragger = new VPDragger(this, p, vp);
    // fixme: draggers should be added AFTER the last one: this way tabbing through them will be from begin to end.
    this->draggers.push_back(new_dragger);
}

void VPDrag::swap_perspectives_of_VPs(Persp3D *persp2, Persp3D *persp1)
{
    // iterate over all VP in all draggers and replace persp2 with persp1
    for (auto dragger : this->draggers) {
        for (auto & vp : dragger->vps) {
            if (vp.get_perspective() == persp2) {
                vp.set_perspective(persp1);
            }
        }
    }
}

void VPDrag::addCurve(Geom::Point const &p1, Geom::Point const &p2, Inkscape::CanvasItemColor color)
{
    auto item_curve = new Inkscape::CanvasItemCurve(SP_ACTIVE_DESKTOP->getCanvasControls(), p1, p2);
    item_curve->set_name("3DBoxCurve");
    item_curve->set_stroke(color);
    item_curves.emplace_back(item_curve);
}

} // namespace Box3D

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
