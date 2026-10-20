// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_STAR_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_STAR_TOOLBAR_H

/**
 * @file Star toolbar
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
class ToggleButton;
} // namespace Gtk

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

class StarToolbar
    : public Toolbar
    , private XML::NodeObserver
{
public:
    StarToolbar();
    ~StarToolbar() override;

    void setDesktop(SPDesktop *desktop) override;
    void setActiveUnit(Util::Unit const *unit) override;

private:
    StarToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    using ValueChangedMemFun = void (StarToolbar::*)();

    Gtk::Label &_mode_item;
    std::vector<Gtk::ToggleButton *> _flat_item_buttons;
    UI::Widget::SpinButton &_magnitude_item;
    Gtk::Box &_spoke_box;
    UI::Widget::SpinButton &_spoke_item;
    UI::Widget::SpinButton &_roundedness_item;
    UI::Widget::SpinButton &_randomization_item;
    UI::Widget::SpinButton &_length_item;

    std::unique_ptr<UI::Widget::UnitTracker> _tracker;

    XML::Node *_repr = nullptr;
    void _attachRepr(XML::Node *repr);
    void _detachRepr();

    bool _batchundo = false;
    OperationBlocker _blocker;
    sigc::connection _selection_changed_conn;
    sigc::connection _selection_modified_conn;

    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value,
                                   ValueChangedMemFun value_changed_mem_fun);
    void side_mode_changed(int mode);
    void magnitude_value_changed();
    void proportion_value_changed();
    void rounded_value_changed();
    void randomized_value_changed();
    void length_value_changed();
    void _turn_upright();
    void _setDefaults();
    void _selectionChanged(Selection *selection);
    void _selectionModified(Selection *selection);
    void _updateLengthValue(Selection *selection);

    void notifyAttributeChanged(XML::Node &node, GQuark name, Util::ptr_shared old_value, Util::ptr_shared new_value) override;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_STAR_TOOLBAR_H
