// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Container for SPKnot visual handles.
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   bulia byak <buliabyak@users.sf.net>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2001-2008 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "knot-holder.h"

#include <glib/gi18n.h>

#include "desktop.h"
#include "display/control/snap-indicator.h"
#include "document-undo.h"
#include "knot-holder-entity.h"
#include "knot.h"
#include "object/box3d.h"
#include "object/sp-ellipse.h"
#include "object/sp-filter.h"
#include "object/sp-hatch.h"
#include "object/sp-marker.h"
#include "object/sp-offset.h"
#include "object/sp-pattern.h"
#include "object/sp-rect.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "style.h"
#include "ui/icon-names.h"
#include "ui/shape-editor.h"
#include "ui/tools/node-tool.h"

Glib::ustring get_icon_name(SPObject *object) {

    Glib::ustring icon_name{};

    if (is<SPRect>(object)) {
        icon_name = INKSCAPE_ICON("draw-rectangle");
    } else if (is<SPBox3D>(object)) {
        icon_name = INKSCAPE_ICON("draw-cuboid");
    } else if (is<SPGenericEllipse>(object)) {
        icon_name = INKSCAPE_ICON("draw-ellipse");
    } else if (is<SPStar>(object)) {
        icon_name = INKSCAPE_ICON("draw-polygon-star");
    } else if (is<SPSpiral>(object)) {
        icon_name = INKSCAPE_ICON("draw-spiral");
    } else if (is<SPMarker>(object)) {
        icon_name = INKSCAPE_ICON("tool-pointer");
    } else if (auto offset = cast<SPOffset>(object)) {
        icon_name = offset->sourceHref ? INKSCAPE_ICON("path-offset-linked") : INKSCAPE_ICON("path-offset-dynamic");
    }

    return icon_name;
}

using Inkscape::DocumentUndo;

KnotHolder::KnotHolder(SPDesktop *desktop, SPItem *item)
    : desktop(desktop)
    , item(item)
    // XML Tree being used directly for item->getRepr() while it shouldn't be...
    , repr(item ? item->getRepr() : nullptr)
{
    if (!desktop || !item) {
        g_warning ("Error! Throw an exception, please!");
    }

    sp_object_ref(item);
}

KnotHolder::~KnotHolder() {
    sp_object_unref(item);
    clear();
}

// this allow clear knothoolder before a destruction
void
KnotHolder::clear()
{
    for (auto & i : entity) {
        delete i;
    }
    entity.clear(); // is this necessary?
}

void
KnotHolder::setEditTransform(Geom::Affine edit_transform)
{
    _edit_transform = edit_transform;
}

void KnotHolder::update_knots()
{
    for (auto e = entity.begin(); e != entity.end(); ) {
        // check if pattern was removed without deleting the knot
        if ((*e)->knot_missing()) {
            delete (*e);
            e = entity.erase(e);
        } else {
            (*e)->update_knot();
            ++e;
        }
    }
}

/**
 * Returns true if at least one of the KnotHolderEntities has the mouse hovering above it.
 */
bool KnotHolder::knot_mouseover() const {
    for (auto i : entity) {
        const SPKnot *knot = i->knot;

        if (knot && knot->is_mouseover()) {
            return true;
        }
    }

    return false;
}

/**
 * Returns true if at least one of the KnotHolderEntities is selected
 */
bool KnotHolder::knot_selected() const {
    for (auto i : entity) {
        const SPKnot *knot = i->knot;

        if (knot && knot->is_selected()) {
            return true;
        }
    }
    return false;
}

void
KnotHolder::knot_mousedown_handler(SPKnot *knot, guint state)
{
    if (!(state & GDK_SHIFT_MASK)) {
        // TODO: handle this event in the Node Tool
        if (auto *nt = dynamic_cast<Inkscape::UI::Tools::NodeTool *>(desktop->getTool())) {
            for (auto &_shape_editor : nt->_shape_editors) {
                Inkscape::UI::ShapeEditor *shape_editor = _shape_editor.second.get();
                if (shape_editor && shape_editor->knotholder) {
                    shape_editor->knotholder->unselect_knots();
                }
            }
        }
    }
    for(auto e : this->entity) {
        if (!(state & GDK_SHIFT_MASK)) {
            e->knot->selectKnot(false);
        }
        if (e->knot == knot) {
            if (!(e->knot->is_selected()) || !(state & GDK_SHIFT_MASK)){
                e->knot->selectKnot(true);
            } else {
                e->knot->selectKnot(false);
            }
        }
    }
}

void
KnotHolder::knot_clicked_handler(SPKnot *knot, guint state)
{
    SPItem *saved_item = this->item;

    for(auto e : this->entity) {
        if (e->knot == knot)
            // no need to test whether knot_click exists since it's virtual now
            e->knot_click(state);
    }

    // Is this necessary, and if it is, why isn't item->updateRepr() called?
    auto shape = cast<SPShape>(saved_item);
    if (shape) {
        shape->set_shape();
    }

    this->update_knots();

    auto icon_name = get_icon_name(saved_item);

    // Is this necessary? We're not changing the XML tree when clicking on a shape handle.
    // Maybe it is left over from editting paths (which is not done here).
    // for drag, this is done by ungrabbed_handler, but for click we must do it here
    if (saved_item && saved_item->document) { // increasingly aggressive sanity checks
       DocumentUndo::done(saved_item->document, RC_("Undo", "Change handle"), icon_name);
    } else {
        std::terminate();
    }
}

void
KnotHolder::transform_selected(Geom::Affine transform){
    for (auto & i : entity) {
        SPKnot *knot = i->knot;
        if (knot->is_selected()) {
            knot_moved_handler(knot, knot->pos * transform , 0);
            knot->selectKnot(true);
        }
    }
}

void KnotHolder::unselect_knots()
{
    for (auto e : entity) {
        if (e->knot->is_selected()) {
            e->knot->selectKnot(false);
        }
    }
}

/** Notifies an entity that its knot has just been grabbed. */
void KnotHolder::knot_grabbed_handler(SPKnot *knot, unsigned state)
{
    auto grab_entity = std::find_if(entity.begin(), entity.end(),
                                    [=](KnotHolderEntity *khe) -> bool { return khe->knot == knot; });
    if (grab_entity == entity.end()) {
        return;
    }
    auto const item_origin = (*grab_entity)->knot->drag_origin * item->dt2i_affine()
                             * _edit_transform.inverse();
    (*grab_entity)->knot_grabbed(item_origin, state);
}

void
KnotHolder::knot_moved_handler(SPKnot *knot, Geom::Point const &p, guint state)
{
    if (!dragging) {
        // The knot has just been grabbed
        knot_grabbed_handler(knot, state);
        dragging = true;
    }

    // this was a local change and the knotholder does not need to be recreated:
    this->local_change = TRUE;

    for(auto e : this->entity) {
        if (e->knot == knot) {
            Geom::Point const q = p * item->i2dt_affine().inverse() * _edit_transform.inverse();
            e->knot_set(q, e->knot->drag_origin * item->i2dt_affine().inverse() * _edit_transform.inverse(), state);
            break;
        }
    }

    auto shape = cast<SPShape>(item);
    if (shape) {
        shape->set_shape();
    }

    this->update_knots();

    item->updateRepr();

    // Keyboard move, may be many small moves.
    auto icon_name = get_icon_name(item);
    DocumentUndo::maybeDone(item->document, "move-handle", RC_("Undo", "Move handle"), icon_name);
}

void
KnotHolder::knot_ungrabbed_handler(SPKnot *knot, guint state)
{
    dragging = false;
    desktop->getSnapIndicator()->remove_snaptarget();

    // if a point is dragged while not selected, it should select itself,
    // even if it was just unselected in the mousedown event handler.
    if (!knot->is_selected()) {
        knot->selectKnot(true);
    } else {
        for (auto e : entity) {
            if (e->knot == knot) {
                e->knot_ungrabbed(e->knot->position(),
                                  e->knot->drag_origin * item->i2dt_affine().inverse() * _edit_transform.inverse(),
                                  state);
                if (e->knot->is_lpe) {
                    return;
                }
                break;
            }
        }
    }

    SPObject *object = item;

    // Caution: this call involves a screen update, which may process events, and as a
    // result the knotholder may be destructed. So, after the updateRepr, we cannot use any
    // fields of this knotholder (such as this->item), but only values we have saved beforehand
    // (such as object).
    object->updateRepr();

    SPFilter *filter = (object->style) ? object->style->getFilter() : nullptr;
    if (filter) {
        filter->updateRepr();
    }

    auto icon_name = get_icon_name(object);
    DocumentUndo::done(object->document, RC_("Undo", "Move handle"), icon_name);
}

void KnotHolder::add(KnotHolderEntity *e)
{
    // g_message("Adding a knot at %p", e);
    entity.push_back(e);
}

void KnotHolder::remove(KnotHolderEntity *e)
{
    std::size_t counter = -1;
    for (auto & i : entity) {
        ++ counter;
        if (e == i) {
            entity.remove_if([i=&i](auto& x){return &x==i;});
            delete i;
            break;
        }
    }
    entity.clear(); // is this necessary?
}

void KnotHolder::add_pattern_knotholder()
{
    if (is<SPPattern>(item->style->getFillPaintServer())) {
        auto entity_xy = new PatternKnotHolderEntityXY(true);
        auto entity_angle = new PatternKnotHolderEntityAngle(true);
        auto entity_scale = new PatternKnotHolderEntityScale(true);
        entity_xy->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Pattern:Fill:xy",
                          // TRANSLATORS: This refers to the pattern that's inside the object
                          _("<b>Move</b> the pattern fill inside the object"));

        entity_scale->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Pattern:Fill:scale",
                             _("<b>Scale</b> the pattern fill; uniformly if with <b>Ctrl</b>"));

        entity_angle->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Pattern:Fill:angle",
                             _("<b>Rotate</b> the pattern fill; with <b>Ctrl</b> to snap angle"));

        entity.push_back(entity_xy);
        entity.push_back(entity_angle);
        entity.push_back(entity_scale);
    }

    if (is<SPPattern>(item->style->getStrokePaintServer())) {
        auto entity_xy = new PatternKnotHolderEntityXY(false);
        auto entity_angle = new PatternKnotHolderEntityAngle(false);
        auto entity_scale = new PatternKnotHolderEntityScale(false);
        entity_xy->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Pattern:Stroke:xy",
                          // TRANSLATORS: This refers to the pattern that's inside the object
                          _("<b>Move</b> the stroke's pattern inside the object"));

        entity_scale->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Pattern:Stroke:scale",
                             _("<b>Scale</b> the stroke's pattern; uniformly if with <b>Ctrl</b>"));

        entity_angle->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Pattern:Stroke:angle",
                             _("<b>Rotate</b> the stroke's pattern; with <b>Ctrl</b> to snap angle"));

        entity.push_back(entity_xy);
        entity.push_back(entity_angle);
        entity.push_back(entity_scale);
    }

    // watch patterns and update knots when they change
    install_modification_watch();
}

void KnotHolder::add_hatch_knotholder()
{
    if ((item->style->fill.isPaintserver()) && cast<SPHatch>(item->style->getFillPaintServer())) {
        HatchKnotHolderEntityXY *entity_xy = new HatchKnotHolderEntityXY(true);
        HatchKnotHolderEntityAngle *entity_angle = new HatchKnotHolderEntityAngle(true);
        HatchKnotHolderEntityScale *entity_scale = new HatchKnotHolderEntityScale(true);
        entity_xy->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Hatch:Fill:xy",
                          // TRANSLATORS: This refers to the hatch that's inside the object
                          _("<b>Move</b> the hatch fill inside the object"));

        entity_scale->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Hatch:Fill:scale",
                             _("<b>Scale</b> the hatch fill; uniformly if with <b>Ctrl</b>"));

        entity_angle->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Hatch:Fill:angle",
                             _("<b>Rotate</b> the hatch fill; with <b>Ctrl</b> to snap angle"));

        entity.push_back(entity_xy);
        entity.push_back(entity_angle);
        entity.push_back(entity_scale);
    }

    if ((item->style->stroke.isPaintserver()) && cast<SPHatch>(item->style->getStrokePaintServer())) {
        HatchKnotHolderEntityXY *entity_xy = new HatchKnotHolderEntityXY(false);
        HatchKnotHolderEntityAngle *entity_angle = new HatchKnotHolderEntityAngle(false);
        HatchKnotHolderEntityScale *entity_scale = new HatchKnotHolderEntityScale(false);
        entity_xy->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Hatch:Stroke:xy",
                          // TRANSLATORS: This refers to the pattern that's inside the object
                          _("<b>Move</b> the hatch stroke inside the object"));

        entity_scale->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Hatch:Stroke:scale",
                             _("<b>Scale</b> the hatch stroke; uniformly if with <b>Ctrl</b>"));

        entity_angle->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Hatch:Stroke:angle",
                             _("<b>Rotate</b> the hatch stroke; with <b>Ctrl</b> to snap angle"));

        entity.push_back(entity_xy);
        entity.push_back(entity_angle);
        entity.push_back(entity_scale);
    }
}

void KnotHolder::add_filter_knotholder() {
    if (auto filter = item->style->getFilter()) {
        if (!filter->auto_region) {
            auto entity_tl = new FilterKnotHolderEntity(true);
            auto entity_br = new FilterKnotHolderEntity(false);
            entity_tl->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Filter:TopLeft",
                              _("<b>Resize</b> the filter effect region"));
            entity_br->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Filter:BottomRight",
                              _("<b>Resize</b> the filter effect region"));
            entity.push_back(entity_tl);
            entity.push_back(entity_br);
        }
    }

    // always install blur nodes, they default to disabled.
    auto entity_x = new BlurKnotHolderEntity(Geom::X);
    auto entity_y = new BlurKnotHolderEntity(Geom::Y);
    entity_x->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Filter:BlurX",
                      _("<b>Drag</b> to <b>adjust</b> blur in x direction; <b>Ctrl</b>+<b>Drag</b> makes x equal to y; <b>Shift</b>+<b>Ctrl</b>+<b>Drag</b> scales blur proportionately "));
    entity_y->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Filter:BlurY",
                      _("<b>Drag</b> to <b>adjust</b> blur in y direction; <b>Ctrl</b>+<b>Drag</b> makes y equal to x; <b>Shift</b>+<b>Ctrl</b>+<b>Drag</b> scales blur proportionately "));
    entity.push_back(entity_x);
    entity.push_back(entity_y);
}

/**
 * When editing an object, this extra information tells out knots
 * where the user has clicked on the item.
 */
bool KnotHolder::set_item_clickpos(Geom::Point loc)
{
    bool ret = false;
    for (auto i : entity) {
        ret = i->set_item_clickpos(loc) || ret;
    }
    return ret;
}

/**
 * When object being edited has some attributes changed (fill, stroke)
 * update what objects we watch
 */
void KnotHolder::install_modification_watch() {
    g_assert(item); 

    if (auto pattern = cast<SPPattern>(item->style->getFillPaintServer())) {
        _watch_fill = pattern->connectModified([this] (SPObject *, unsigned) {
            update_knots();
        });
    }
    else {
        _watch_fill.disconnect();
    }

    if (auto pattern = cast<SPPattern>(item->style->getStrokePaintServer())) {
        _watch_stroke = pattern->connectModified([this] (SPObject *, unsigned) {
            update_knots();
        });
    }
    else {
        _watch_stroke.disconnect();
    }
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
