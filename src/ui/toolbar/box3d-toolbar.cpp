// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file 3d box toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "box3d-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/box3d.h"
#include "object/persp3d.h"
#include "preferences.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/spinbutton.h"

using Inkscape::DocumentUndo;

namespace Inkscape::UI::Toolbar {

Box3DToolbar::Box3DToolbar()
    : Box3DToolbar{create_builder("toolbar-box3d.ui")}
{}

Box3DToolbar::Box3DToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "box3d-toolbar")}
    , _angle_x_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_angle_x_item")}
    , _vp_x_state_btn{get_widget<Gtk::ToggleButton>(builder, "_vp_x_state_btn")}
    , _angle_y_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_angle_y_item")}
    , _vp_y_state_btn{get_widget<Gtk::ToggleButton>(builder, "_vp_y_state_btn")}
    , _angle_z_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_angle_z_item")}
    , _vp_z_state_btn{get_widget<Gtk::ToggleButton>(builder, "_vp_z_state_btn")}
{
    auto prefs = Preferences::get();

    _vp_x_state_btn.set_active(prefs->getBool("/tools/shapes/3dbox/vp_x_state", true));
    _vp_x_state_btn.signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::vp_state_changed), Proj::X));

    _vp_y_state_btn.set_active(prefs->getBool("/tools/shapes/3dbox/vp_y_state", true));
    _vp_y_state_btn.signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::vp_state_changed), Proj::Y));

    _vp_z_state_btn.set_active(prefs->getBool("/tools/shapes/3dbox/vp_z_state", true));
    _vp_z_state_btn.signal_toggled().connect(
        sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::vp_state_changed), Proj::Z));

    setup_derived_spin_button(_angle_x_item, "box3d_angle_x", Proj::X);
    setup_derived_spin_button(_angle_y_item, "box3d_angle_y", Proj::Y);
    setup_derived_spin_button(_angle_z_item, "box3d_angle_z", Proj::Z);

    _angle_x_item.set_custom_numeric_menu_data({
        {135, ""},
        {150, ""},
        {165, ""},
        {180, ""},
        {195, ""},
        {210, ""},
        {235, ""}
    });

    _angle_y_item.set_custom_numeric_menu_data({
        {270, ""},
    });

    _angle_z_item.set_custom_numeric_menu_data({
        {-45, ""},
        {-30, ""},
        {-15, ""},
        {0, ""},
        {15, ""},
        {30, ""},
        {45, ""}
    });

    _initMenuBtns();
}

void Box3DToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        _selection_changed_conn.disconnect();

        if (_repr) {
            _detachRepr();
        }
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        auto sel = _desktop->getSelection();
        _selection_changed_conn = sel->connectChanged(sigc::mem_fun(*this, &Box3DToolbar::_selectionChanged));
        _selectionChanged(sel); // Synthesize an emission to trigger the update
    }
}

void Box3DToolbar::_attachRepr(XML::Node *repr, Persp3D *persp)
{
    assert(!_repr);
    _repr = repr;
    _persp = persp;
    GC::anchor(_repr);
    _repr->addObserver(*this);
}

void Box3DToolbar::_detachRepr()
{
    assert(_repr);
    _repr->removeObserver(*this);
    GC::release(_repr);
    _repr = nullptr;
    _persp = nullptr;
    _cancelUpdate();
}

void Box3DToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, Proj::Axis axis)
{
    auto const path = "/tools/shapes/3dbox/" + name;
    auto const val = Preferences::get()->getDouble(path, 30);

    auto adj = btn.get_adjustment();
    adj->set_value(val);

    adj->signal_value_changed().connect(
        sigc::bind(sigc::mem_fun(*this, &Box3DToolbar::angle_value_changed), adj, axis));

    btn.setDefocusTarget(this);
}

void Box3DToolbar::angle_value_changed(Glib::RefPtr<Gtk::Adjustment> const &adj, Proj::Axis axis)
{
    auto document = _desktop->getDocument();

    // quit if run by the attr_changed or selection changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    auto const sel_persps = _desktop->getSelection()->perspList();
    if (sel_persps.empty()) {
        // this can happen when the document is created; we silently ignore it
        return;
    }
    auto const persp = sel_persps.front();

    persp->perspective_impl->tmat.set_infinite_direction(axis, adj->get_value());
    persp->updateRepr();

    // TODO: use the correct axis here, too
    DocumentUndo::maybeDone(document, "perspangle", RC_("Undo", "3D Box: Change perspective (angle of infinite axis)"), INKSCAPE_ICON("draw-cuboid"));
}

void Box3DToolbar::vp_state_changed(Proj::Axis axis)
{
    // TODO: Take all selected perspectives into account
    auto const sel_persps = _desktop->getSelection()->perspList();
    if (sel_persps.empty()) {
        // this can happen when the document is created; we silently ignore it
        return;
    }
    auto const persp = sel_persps.front();

    bool set_infinite = false;

    switch(axis) {
        case Proj::X:
            set_infinite = _vp_x_state_btn.get_active();
            break;
        case Proj::Y:
            set_infinite = _vp_y_state_btn.get_active();
            break;
        case Proj::Z:
            set_infinite = _vp_z_state_btn.get_active();
            break;
        default:
            return;
    }

    persp->set_VP_state(axis, set_infinite ? Proj::VP_INFINITE : Proj::VP_FINITE);
}

// FIXME: This should rather be put into persp3d-reference.cpp or something similar so that it reacts upon each
//        change of the perspective, and not of the current selection (but how to refer to the toolbar then?)
void Box3DToolbar::_selectionChanged(Selection *selection)
{
    // Here the following should be done: If all selected boxes have finite VPs in a certain direction,
    // disable the angle entry fields for this direction (otherwise entering a value in them should only
    // update the perspectives with infinite VPs and leave the other ones untouched).

    if (_repr) {
        _detachRepr();
    }

    auto box = cast<SPBox3D>(selection->singleItem());
    if (!box) {
        return;
    }

    // FIXME: Also deal with multiple selected boxes
    auto persp = box->get_perspective();
    if (!persp) {
        g_warning("Box has no perspective set!");
        return;
    }

    _attachRepr(persp->getRepr(), persp);
    _queueUpdate();

    selection->document()->setCurrentPersp3D(_persp);
    Preferences::get()->setString("/tools/shapes/3dbox/persp", _repr->attribute("id"));
}

void Box3DToolbar::set_button_and_adjustment(Proj::Axis axis, UI::Widget::SpinButton &spin_btn, Gtk::ToggleButton &toggle_btn)
{
    // TODO: Take all selected perspectives into account but don't touch the state button if not all of them
    //       have the same state (otherwise a call to box3d_vp_z_state_changed() is triggered and the states
    //       are reset).
    bool is_infinite = !Persp3D::VP_is_finite(_persp->perspective_impl.get(), axis);

    toggle_btn.set_active(is_infinite);
    spin_btn.set_sensitive(is_infinite);

    if (is_infinite) {
        double angle = _persp->get_infinite_angle(axis);
        if (angle != Geom::infinity()) { // FIXME: We should catch this error earlier (don't show the spinbutton at all)
            spin_btn.get_adjustment()->set_value(Geom::deg_from_rad(Geom::Angle::from_degrees(angle).radians0()));
        }
    }
}

void Box3DToolbar::notifyAttributeChanged(XML::Node &, GQuark, Util::ptr_shared, Util::ptr_shared)
{
    assert(_repr);
    assert(_persp);

    // quit if run by the UI callbacks
    if (_blocker.pending()) {
        return;
    }

    _persp->update_box_reprs();
    _queueUpdate();
}

void Box3DToolbar::_queueUpdate()
{
    if (_tick_callback) {
        return;
    }

    _tick_callback = add_tick_callback([this] (Glib::RefPtr<Gdk::FrameClock> const &) {
        _update();
        _tick_callback = 0;
        return false;
    });
}

void Box3DToolbar::_cancelUpdate()
{
    if (!_tick_callback) {
        return;
    }

    remove_tick_callback(_tick_callback);
    _tick_callback = 0;
}

void Box3DToolbar::_update()
{
    assert(_repr);
    assert(_persp);

    // prevent UI callbacks from responding
    auto guard = _blocker.block();

    set_button_and_adjustment(Proj::X, _angle_x_item, _vp_x_state_btn);
    set_button_and_adjustment(Proj::Y, _angle_y_item, _vp_y_state_btn);
    set_button_and_adjustment(Proj::Z, _angle_z_item, _vp_z_state_btn);
}

} // namespace Inkscape::UI::Toolbar

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
