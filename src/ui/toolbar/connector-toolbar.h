// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_CONNECTOR_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_CONNECTOR_TOOLBAR_H

/**
 * @file Connector toolbar
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
#include "ui/widget/spinbutton.h"
#include "xml/node-observer.h"

namespace Gtk {
class Builder;
class ToggleButton;
} // namespace Gtk

class SPNamedView;

namespace Inkscape {
class Selection;
namespace XML { class Node; }
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class ConnectorToolbar
    : public Toolbar
    , private XML::NodeObserver
{
public:
    ConnectorToolbar();

    void setDesktop(SPDesktop *desktop) override;

private:
    ConnectorToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    using ValueChangedMemFun = void (ConnectorToolbar::*)();

    Gtk::ToggleButton &_orthogonal_btn;
    Gtk::ToggleButton &_directed_btn;
    Gtk::ToggleButton &_overlap_btn;

    UI::Widget::SpinButton &_curvature_item;
    UI::Widget::SpinButton &_spacing_item;
    UI::Widget::SpinButton &_length_item;

    OperationBlocker _blocker;

    XML::Node *_repr = nullptr;

    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value,
                                   ValueChangedMemFun value_changed_mem_fun);
    void path_set_avoid();
    void path_set_ignore();
    void orthogonal_toggled();
    void graph_layout();
    void directed_graph_layout_toggled();
    void nooverlaps_graph_layout_toggled();
    void curvature_changed();
    void spacing_changed();
    void length_changed();

    sigc::connection _selection_changed_conn;
    void _selectionChanged(Selection *selection);

    void notifyAttributeChanged(XML::Node &node, GQuark name, Util::ptr_shared old_value, Util::ptr_shared new_value) override;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_CONNECTOR_TOOLBAR_H
