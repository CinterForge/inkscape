// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_BOX3D_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_BOX3D_TOOLBAR_H

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

#include "axis-manip.h"
#include "toolbar.h"
#include "ui/operation-blocker.h"
#include "xml/node-observer.h"

namespace Gtk {
class Builder;
class ToggleButton;
} // namespace Gtk

class Persp3D;

namespace Inkscape {
class Selection;
namespace UI {
namespace Tools { class ToolBase; }
namespace Widget { class SpinButton; }
} // namespace UI
namespace XML { class Node; }
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class Box3DToolbar
    : public Toolbar
    , private XML::NodeObserver
{
public:
    Box3DToolbar();

    void setDesktop(SPDesktop *desktop) override;

private:
    Box3DToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    UI::Widget::SpinButton &_angle_x_item;
    UI::Widget::SpinButton &_angle_y_item;
    UI::Widget::SpinButton &_angle_z_item;

    Gtk::ToggleButton &_vp_x_state_btn;
    Gtk::ToggleButton &_vp_y_state_btn;
    Gtk::ToggleButton &_vp_z_state_btn;

    XML::Node *_repr = nullptr;
    Persp3D *_persp = nullptr;
    void _attachRepr(XML::Node *repr, Persp3D *persp);
    void _detachRepr();

    OperationBlocker _blocker;

    void angle_value_changed(Glib::RefPtr<Gtk::Adjustment> const &adj, Proj::Axis axis);
    void vp_state_changed(Proj::Axis axis);
    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, Proj::Axis axis);
    void set_button_and_adjustment(Proj::Axis axis, UI::Widget::SpinButton &spin_btn, Gtk::ToggleButton &toggle_btn);

    sigc::connection _selection_changed_conn;
    void _selectionChanged(Selection *selection);

    void notifyAttributeChanged(XML::Node &node, GQuark name,
                                Util::ptr_shared old_value,
                                Util::ptr_shared new_value) override;

    void _queueUpdate();
    void _cancelUpdate();
    void _update();
    unsigned _tick_callback = 0;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_BOX3D_TOOLBAR_H
