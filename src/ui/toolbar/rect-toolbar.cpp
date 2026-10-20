// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "rect-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/sp-rect.h"
#include "preferences.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::DocumentUndo;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;

namespace Inkscape::UI::Toolbar {

struct RectToolbar::DerivedSpinButton : UI::Widget::SpinButton
{
    using Getter = double (SPRect::*)() const;
    using Setter = void (SPRect::*)(double);

    DerivedSpinButton(BaseObjectType *cobject, Glib::RefPtr<Gtk::Builder> const &, char const *name, Getter getter, Setter setter)
        : UI::Widget::SpinButton(cobject)
        , name{name}
        , getter{getter}
        , setter{setter}
    {}

    char const *name;
    Getter getter;
    Setter setter;
};

RectToolbar::RectToolbar()
    : RectToolbar{create_builder("toolbar-rect.ui")}
{}

RectToolbar::RectToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "rect-toolbar")}
    , _tracker{std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _mode_item{UI::get_widget<Gtk::Label>(builder, "_mode_item")}
    , _not_rounded{UI::get_widget<Gtk::Button>(builder, "_not_rounded")}
    , _width_item{UI::get_derived_widget<DerivedSpinButton>(builder, "_width_item", "width", &SPRect::getVisibleWidth, &SPRect::setVisibleWidth)}
    , _height_item{UI::get_derived_widget<DerivedSpinButton>(builder, "_height_item", "height", &SPRect::getVisibleHeight, &SPRect::setVisibleHeight)}
    , _rx_item{UI::get_derived_widget<DerivedSpinButton>(builder, "_rx_item", "rx", &SPRect::getVisibleRx, &SPRect::setVisibleRx)}
    , _ry_item{UI::get_derived_widget<DerivedSpinButton>(builder, "_ry_item", "ry", &SPRect::getVisibleRy, &SPRect::setVisibleRy)}
{
    auto unit_menu = _tracker->create_unit_dropdown();
    get_widget<Gtk::Box>(builder, "unit_menu_box").append(*unit_menu);

    _not_rounded.signal_clicked().connect([this] { _setDefaults(); });

    for (auto sb : _getDerivedSpinButtons()) {
        auto const adj = sb->get_adjustment();
        auto const path = Glib::ustring{"/tools/shapes/rect/"} + sb->name;
        auto const val = Preferences::get()->getDouble(path, 0);
        adj->set_value(Quantity::convert(val, "px", _tracker->getActiveUnit()));
        adj->signal_value_changed().connect([this, sb] { _valueChanged(*sb, sb->name); });
        _tracker->addAdjustment(adj->gobj());
        sb->addUnitTracker(_tracker.get());
        sb->setDefocusTarget(this);
    }

    _width_item.set_custom_numeric_menu_data({
        {1, ""},
        {2, ""},
        {3, ""},
        {5, ""},
        {10, ""},
        {20, ""},
        {50, ""},
        {100, ""},
        {200, ""},
        {500, ""}
    });

    _height_item.set_custom_numeric_menu_data({
        {1, ""},
        {2, ""},
        {3, ""},
        {5, ""},
        {10, ""},
        {20, ""},
        {50, ""},
        {100, ""},
        {200, ""},
        {500, ""}
    });

    _rx_item.set_custom_numeric_menu_data({
        {0.5, _("not rounded")},
        {1, ""},
        {2, ""},
        {3, ""},
        {5, ""},
        {10, ""},
        {20, ""},
        {50, ""},
        {100, ""}
    });

    _ry_item.set_custom_numeric_menu_data({
        {0.5, _("not rounded")},
        {1, ""},
        {2, ""},
        {3, ""},
        {5, ""},
        {10, ""},
        {20, ""},
        {50, ""},
        {100, ""}
    });

    _initMenuBtns();
}

RectToolbar::~RectToolbar() = default;

void RectToolbar::setDesktop(SPDesktop *desktop)
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
        _selection_changed_conn = sel->connectChanged(sigc::mem_fun(*this, &RectToolbar::_selectionChanged));
        _selectionChanged(sel); // Synthesize an emission to trigger the update

        _sensitivize();
    }
}

void RectToolbar::setActiveUnit(Util::Unit const *unit)
{
    _tracker->setActiveUnit(unit);
}

void RectToolbar::_attachRepr(XML::Node *repr, SPRect *rect)
{
    assert(!_repr);
    _repr = repr;
    _rect = rect;
    GC::anchor(_repr);
    _repr->addObserver(*this);
}

void RectToolbar::_detachRepr()
{
    assert(_repr);
    _repr->removeObserver(*this);
    GC::release(_repr);
    _repr = nullptr;
    _rect = nullptr;
    _cancelUpdate();
}

void RectToolbar::_valueChanged(DerivedSpinButton &btn, char const *name)
{
    // quit if run by the XML listener or a unit change
    if (_blocker.pending() || _tracker->isUpdating()) {
        return;
    }

    // in turn, prevent XML listener from responding
    auto guard = _blocker.block();

    auto const adj = btn.get_adjustment();

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        auto const path = Glib::ustring{"/tools/shapes/rect/"} + btn.name;
        Preferences::get()->setDouble(path, Quantity::convert(adj->get_value(), _tracker->getActiveUnit(), "px"));
    }

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (auto rect = cast<SPRect>(item)) {
            if (adj->get_value() != 0) {
                (rect->*btn.setter)(Quantity::convert(adj->get_value(), _tracker->getActiveUnit(), "px"));
            } else {
                rect->removeAttribute(btn.name);
            }
            modified = true;
        }
    }

    _sensitivize();

    if (modified) {
        DocumentUndo::maybeDone(_desktop->getDocument(), name, RC_("Undo", "Change rectangle"),
                                INKSCAPE_ICON("draw-rectangle"));
    }
}

void RectToolbar::_sensitivize()
{
    bool disabled = _rx_item.get_adjustment()->get_value() == 0 &&
                    _ry_item.get_adjustment()->get_value() == 0 &&
                    _single; // only for a single selected rect (for now)
    _not_rounded.set_sensitive(!disabled);
}

void RectToolbar::_setDefaults()
{
    _rx_item.get_adjustment()->set_value(0.0);
    _ry_item.get_adjustment()->set_value(0.0);
    _sensitivize();
}

void RectToolbar::_selectionChanged(Selection *selection)
{
    if (_repr) {
        _detachRepr();
    }

    int n_selected = 0;
    XML::Node *repr = nullptr;
    SPRect *rect = nullptr;

    for (auto item : selection->items()) {
        if (auto r = cast<SPRect>(item)) {
            n_selected++;
            repr = r->getRepr();
            rect = r;
        }
    }

    _single = n_selected == 1;

    if (_single) {
        _attachRepr(repr, rect);
        _queueUpdate();
    }

    _mode_item.set_markup(n_selected == 0 ? _("<b>New:</b>") : _("<b>Change:</b>"));
    _width_item.set_sensitive(n_selected > 0);
    _height_item.set_sensitive(n_selected > 0);

    if (!_single) { // otherwise handled by _queueUpdate
        _sensitivize();
    }
}

void RectToolbar::notifyAttributeChanged(XML::Node &, GQuark, Util::ptr_shared, Util::ptr_shared)
{
    assert(_repr);
    assert(_rect);

    // quit if run by the UI callbacks
    if (_blocker.pending()) {
        return;
    }

    _queueUpdate();
}

// Todo: Code duplication; move into Toolbar container
// Todo: Similarly for setDefocusWidget() handling.
void RectToolbar::_queueUpdate()
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

void RectToolbar::_cancelUpdate()
{
    if (!_tick_callback) {
        return;
    }

    remove_tick_callback(_tick_callback);
    _tick_callback = 0;
}

void RectToolbar::_update()
{
    assert(_repr);
    assert(_rect);

    // prevent UI callbacks from responding
    auto guard = _blocker.block();

    for (auto sb : _getDerivedSpinButtons()) {
        sb->get_adjustment()->set_value(Quantity::convert((_rect->*sb->getter)(), "px", _tracker->getActiveUnit()));
    }

    _sensitivize();
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
