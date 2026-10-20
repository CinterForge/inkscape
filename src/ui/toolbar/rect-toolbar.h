// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_RECT_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_RECT_TOOLBAR_H

/**
 * @file Rectangle toolbar
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

#include <array>

#include "toolbar.h"
#include "ui/operation-blocker.h"
#include "xml/node-observer.h"

namespace Gtk {
class Builder;
class Button;
class Label;
class Adjustment;
} // namespace Gtk

class SPRect;

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

class RectToolbar
    : public Toolbar
    , private XML::NodeObserver
{
public:
    RectToolbar();
    ~RectToolbar() override;

    void setDesktop(SPDesktop *desktop) override;
    void setActiveUnit(Util::Unit const *unit) override;

private:
    RectToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    std::unique_ptr<UI::Widget::UnitTracker> _tracker;

    Gtk::Label &_mode_item;
    Gtk::Button &_not_rounded;

    struct DerivedSpinButton;
    DerivedSpinButton &_width_item;
    DerivedSpinButton &_height_item;
    DerivedSpinButton &_rx_item;
    DerivedSpinButton &_ry_item;
    auto _getDerivedSpinButtons() const { return std::to_array({&_rx_item, &_ry_item, &_width_item, &_height_item}); }
    void _valueChanged(DerivedSpinButton &btn, char const *name);

    XML::Node *_repr = nullptr;
    SPRect *_rect = nullptr;
    void _attachRepr(XML::Node *repr, SPRect *rect);
    void _detachRepr();

    OperationBlocker _blocker;
    bool _single = true;

    sigc::connection _selection_changed_conn;
    void _selectionChanged(Selection *selection);

    void _sensitivize();
    void _setDefaults();

    void notifyAttributeChanged(XML::Node &node, GQuark name, Util::ptr_shared old_value, Util::ptr_shared new_value) override;
    void _queueUpdate();
    void _cancelUpdate();
    void _update();
    unsigned _tick_callback = 0;
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_RECT_TOOLBAR_H
