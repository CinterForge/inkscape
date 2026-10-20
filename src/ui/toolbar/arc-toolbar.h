// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_ARC_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_ARC_TOOLBAR_H

/**
 * @file Arc toolbar
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
class Button;
class Builder;
class Label;
class ToggleButton;
} // namespace Gtk

class SPGenericEllipse;

namespace Inkscape {
class Selection;
namespace UI {
namespace Tools { class ToolBase; }
namespace Widget {
class Label;
class SpinButton;
class UnitTracker;
} // namespace Widget
} // namespace UI
namespace XML { class Node; }
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class ArcToolbar
    : public Toolbar
    , private XML::NodeObserver
{
public:
    ArcToolbar();
    ~ArcToolbar() override;

    void setDesktop(SPDesktop *desktop) override;
    void setActiveUnit(Util::Unit const *unit) override;

private:
    ArcToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    std::unique_ptr<UI::Widget::UnitTracker> _tracker;

    UI::Widget::SpinButton &_cx_item;
    UI::Widget::SpinButton &_cy_item;
    UI::Widget::SpinButton &_rx_item;
    UI::Widget::SpinButton &_ry_item;
    UI::Widget::SpinButton &_start_item;
    UI::Widget::SpinButton &_end_item;

    Gtk::Label &_mode_item;

    std::vector<Gtk::ToggleButton *> _type_buttons;
    Gtk::Button &_make_whole;

    XML::Node *_repr = nullptr;
    SPGenericEllipse *_ellipse = nullptr;
    void _attachRepr(XML::Node *repr, SPGenericEllipse *ellipse);
    void _detachRepr();

    OperationBlocker _blocker;
    bool _single = true;

    void _setupDerivedSpinButton(UI::Widget::SpinButton &btn, Glib::ustring const &name);
    void _setupStartendButton(UI::Widget::SpinButton &btn, Glib::ustring const &name, UI::Widget::SpinButton &other_btn);
    void _valueChanged(Glib::RefPtr<Gtk::Adjustment> const &adj, Glib::ustring const &value_name);
    void _startendValueChanged(Glib::RefPtr<Gtk::Adjustment> const &adj, Glib::ustring const &value_name, Glib::RefPtr<Gtk::Adjustment> const &other_adj);
    void _typeChanged(int type);
    void _setDefaults();
    void _sensitivize();

    sigc::connection _selection_changed_conn;
    void _selectionChanged(Selection *selection);

    void notifyAttributeChanged(XML::Node &node, GQuark name, Util::ptr_shared old_value, Util::ptr_shared new_value) override;
    void _queueUpdate();
    void _cancelUpdate();
    void _update();
    unsigned _tick_callback = 0;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_ARC_TOOLBAR_H
