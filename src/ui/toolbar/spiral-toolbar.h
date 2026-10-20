// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_SPIRAL_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_SPIRAL_TOOLBAR_H

/**
 * @file Spiral toolbar
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

#include "toolbar.h"
#include "ui/operation-blocker.h"
#include "xml/node-observer.h"

namespace Gtk {
class Builder;
class Label;
class Adjustment;
} // namespace Gtk

namespace Inkscape {
class Selection;
namespace UI::Widget { class SpinButton; }
namespace XML { class Node; }
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class SpiralToolbar
    : public Toolbar
    , private XML::NodeObserver
{
public:
    SpiralToolbar();

    void setDesktop(SPDesktop *desktop) override;

private:
    SpiralToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    Gtk::Label &_mode_item;

    UI::Widget::SpinButton &_revolution_item;
    UI::Widget::SpinButton &_expansion_item;
    UI::Widget::SpinButton &_t0_item;

    OperationBlocker _blocker;

    XML::Node *_repr = nullptr;
    void _attachRepr(XML::Node *repr);
    void _detachRepr();

    void _setupDerivedSpinButton(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value);
    void _valueChanged(Glib::RefPtr<Gtk::Adjustment> &adj, Glib::ustring const &value_name);
    void _setDefaults();

    sigc::connection _selection_changed_conn;
    void _selectionChanged(Selection *selection);

    void notifyAttributeChanged(XML::Node &node, GQuark key, Util::ptr_shared oldval, Util::ptr_shared newval) override;
    void _queueUpdate();
    void _cancelUpdate();
    void _update();
    unsigned _tick_callback = 0;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_SPIRAL_TOOLBAR_H
