// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Arc aux toolbar
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

#include "arc-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/sp-ellipse.h"
#include "preferences.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::DocumentUndo;
using Inkscape::Util::Quantity;

namespace Inkscape::UI::Toolbar {

ArcToolbar::ArcToolbar()
    : ArcToolbar{create_builder("toolbar-arc.ui")}
{}

ArcToolbar::ArcToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "arc-toolbar")}
    , _tracker{std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _mode_item{get_widget<Gtk::Label>(builder, "_mode_item")}
    , _cx_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_cx_item")}
    , _cy_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_cy_item")}
    , _rx_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_rx_item")}
    , _ry_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_ry_item")}
    , _start_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_start_item")}
    , _end_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_end_item")}
    , _make_whole{get_widget<Gtk::Button>(builder, "_make_whole")}
    , _type_buttons{
          &get_widget<Gtk::ToggleButton>(builder, "slice_btn"),
          &get_widget<Gtk::ToggleButton>(builder, "arc_btn"),
          &get_widget<Gtk::ToggleButton>(builder, "chord_btn")
      }
{
    auto unit_menu = _tracker->create_unit_dropdown();
    get_widget<Gtk::Box>(builder, "unit_menu_box").append(*unit_menu);

    _setupDerivedSpinButton(_cx_item, "cx");
    _setupDerivedSpinButton(_cy_item, "cy");
    _setupDerivedSpinButton(_rx_item, "rx");
    _setupDerivedSpinButton(_ry_item, "ry");
    _setupStartendButton(_start_item, "start", _end_item);
    _setupStartendButton(_end_item, "end", _start_item);

    _rx_item.set_custom_numeric_menu_data({
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

    _ry_item.set_custom_numeric_menu_data({
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

    // Values auto-calculated.
    _start_item.set_custom_numeric_menu_data({});
    _end_item.set_custom_numeric_menu_data({});

    int type = Preferences::get()->getInt("/tools/shapes/arc/arc_type", 0);
    type = std::clamp<int>(type, 0, _type_buttons.size() - 1);
    _type_buttons[type]->set_active();

    for (int i = 0; i < _type_buttons.size(); i++) {
        _type_buttons[i]->signal_toggled().connect([this, i] {
            if (_type_buttons[i]->get_active()) {
                _typeChanged(i);
            }
        });
    }

    _make_whole.signal_clicked().connect(sigc::mem_fun(*this, &ArcToolbar::_setDefaults));

    _initMenuBtns();
}

ArcToolbar::~ArcToolbar() = default;

void ArcToolbar::_setupDerivedSpinButton(UI::Widget::SpinButton &btn, Glib::ustring const &name)
{
    auto const adj = btn.get_adjustment();
    auto const val = Preferences::get()->getDouble("/tools/shapes/arc/" + name, 0);
    adj->set_value(Quantity::convert(val, "px", _tracker->getActiveUnit()));
    adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &ArcToolbar::_valueChanged), adj, name));

    _tracker->addAdjustment(adj->gobj());
    btn.addUnitTracker(_tracker.get());
    btn.set_sensitive(false);
    btn.setDefocusTarget(this);
}

void ArcToolbar::_setupStartendButton(UI::Widget::SpinButton &btn, Glib::ustring const &name, UI::Widget::SpinButton &other_btn)
{
    auto const adj = btn.get_adjustment();
    auto const val = Preferences::get()->getDouble("/tools/shapes/arc/" + name, 0);
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &ArcToolbar::_startendValueChanged), adj, name, other_btn.get_adjustment()));
}

void ArcToolbar::_attachRepr(XML::Node *repr, SPGenericEllipse *ellipse)
{
    assert(!_repr);
    _repr = repr;
    _ellipse = ellipse;
    GC::anchor(_repr);
    _repr->addObserver(*this);
}

void ArcToolbar::_detachRepr()
{
    assert(_repr);
    _repr->removeObserver(*this);
    GC::release(_repr);
    _repr = nullptr;
    _ellipse = nullptr;
    _cancelUpdate();
}

void ArcToolbar::setDesktop(SPDesktop *desktop)
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
        _selection_changed_conn = sel->connectChanged(sigc::mem_fun(*this, &ArcToolbar::_selectionChanged));
        _selectionChanged(sel); // Synthesize an emission to trigger the update

        _sensitivize();
    }
}

void ArcToolbar::setActiveUnit(Util::Unit const *unit)
{
    _tracker->setActiveUnit(unit);
}

void ArcToolbar::_valueChanged(Glib::RefPtr<Gtk::Adjustment> const &adj, Glib::ustring const &value_name)
{
    // quit if run by the XML listener or a unit change
    if (_blocker.pending() || _tracker->isUpdating()) {
        return;
    }

    // in turn, prevent XML listener from responding
    auto guard = _blocker.block();

    // Per SVG spec "a [radius] value of zero disables rendering of the element".
    // However our implementation does not allow a setting of zero in the UI (not even in the XML editor)
    // and ugly things happen if it's forced here, so better leave the properties untouched.
    if (!adj->get_value() && value_name[0] == 'r') {
        return;
    }

    auto const unit = _tracker->getActiveUnit();

    Preferences::get()->setDouble("/tools/shapes/arc/" + value_name, Quantity::convert(adj->get_value(), unit, "px"));

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (auto ge = cast<SPGenericEllipse>(item)) {

            if (value_name == "rx") {
                ge->setVisibleRx(Quantity::convert(adj->get_value(), unit, "px"));
            } else if (value_name == "ry") {
                ge->setVisibleRy(Quantity::convert(adj->get_value(), unit, "px"));
            } else if (value_name == "cx") {
                ge->setVisibleCx(Quantity::convert(adj->get_value(), unit, "px"));
            } else if (value_name == "cy") {
                ge->setVisibleCy(Quantity::convert(adj->get_value(), unit, "px"));
            }

            ge->normalize();
            ge->updateRepr();

            modified = true;
        }
    }

    if (modified) {
        DocumentUndo::maybeDone(_desktop->getDocument(), value_name.c_str(), RC_("Undo", "Ellipse: Change radius"),
                                INKSCAPE_ICON("draw-ellipse"));
    }
}

void ArcToolbar::_startendValueChanged(Glib::RefPtr<Gtk::Adjustment> const &adj, Glib::ustring const &value_name, Glib::RefPtr<Gtk::Adjustment> const &other_adj)
{
    Preferences::get()->setDouble("/tools/shapes/arc/" + value_name, adj->get_value());

    // quit if run by the XML listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent XML listener from responding
    auto guard = _blocker.block();

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (auto ge = cast<SPGenericEllipse>(item)) {

            auto const val = Geom::rad_from_deg(adj->get_value());
            if (value_name == "start") {
                ge->start = val;
            } else {
                ge->end = val;
            }

            ge->normalize();
            ge->updateRepr();

            modified = true;
        }
    }

    _sensitivize();

    if (modified) {
        DocumentUndo::maybeDone(_desktop->getDocument(), value_name.c_str(), RC_("Undo", "Arc: Change start/end"), INKSCAPE_ICON("draw-ellipse"));
    }
}

void ArcToolbar::_typeChanged(int type)
{
    Preferences::get()->setInt("/tools/shapes/arc/arc_type", type);

    // quit if run by the XML listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent XML listener from responding
    auto guard = _blocker.block();

    char const *arc_type = "slice";
    bool open = false;
    switch (type) {
        case 0:
            arc_type = "slice";
            open = false;
            break;
        case 1:
            arc_type = "arc";
            open = true;
            break;
        case 2:
            arc_type = "chord";
            open = true; // For backward compat, not truly open but chord most like arc.
            break;
        default:
            std::cerr << __FUNCTION__ << ": bad arc type: " << type << std::endl;
            break;
    }

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPGenericEllipse>(item)) {
            auto repr = item->getRepr();
            repr->setAttribute("sodipodi:open", open ? "true" : nullptr);
            repr->setAttribute("sodipodi:arc-type", arc_type);
            item->updateRepr();
            modified = true;
        }
    }

    if (modified) {
        DocumentUndo::done(_desktop->getDocument(), RC_("Undo", "Arc: Change arc type"), INKSCAPE_ICON("draw-ellipse"));
    }
}

void ArcToolbar::_setDefaults()
{
    _start_item.get_adjustment()->set_value(0.0);
    _end_item.get_adjustment()->set_value(0.0);
    onDefocus();
}

void ArcToolbar::_sensitivize()
{
    bool disabled = _start_item.get_adjustment()->get_value() == 0 &&
                    _end_item  .get_adjustment()->get_value() == 0 &&
                    _single; // only for a single selected ellipse (for now)
    for (auto btn : _type_buttons) {
        btn->set_sensitive(!disabled);
    }
    _make_whole.set_sensitive(!disabled);
}

void ArcToolbar::_selectionChanged(Selection *selection)
{
    if (_repr) {
        _detachRepr();
    }

    int n_selected = 0;
    XML::Node *repr = nullptr;
    SPGenericEllipse *ellipse = nullptr;

    for (auto item : selection->items()){
        if (auto ge = cast<SPGenericEllipse>(item)) {
            n_selected++;
            repr = ge->getRepr();
            ellipse = ge;
        }
    }

    _single = n_selected == 1;

    if (_single) {
        _attachRepr(repr, ellipse);
        _queueUpdate();
    }

    _mode_item.set_markup(n_selected == 0 ? _("<b>New:</b>") : _("<b>Change:</b>"));
    _cx_item.set_sensitive(n_selected > 0);
    _cy_item.set_sensitive(n_selected > 0);
    _rx_item.set_sensitive(n_selected > 0);
    _ry_item.set_sensitive(n_selected > 0);

    if (!_single) { // otherwise handled by _queueUpdate
        _sensitivize();
    }
}

void ArcToolbar::notifyAttributeChanged(XML::Node &, GQuark name, Util::ptr_shared, Util::ptr_shared)
{
    assert(_repr);
    assert(_ellipse);

    // quit if run by the UI callbacks
    if (_blocker.pending()) {
        return;
    }

    _queueUpdate();
}

void ArcToolbar::_queueUpdate()
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

void ArcToolbar::_cancelUpdate()
{
    if (!_tick_callback) {
        return;
    }

    remove_tick_callback(_tick_callback);
    _tick_callback = 0;
}

void ArcToolbar::_update()
{
    assert(_repr);
    assert(_ellipse);

    // prevent UI callbacks from responding
    auto guard = _blocker.block();

    _cx_item.get_adjustment()->set_value(Quantity::convert(_ellipse->getVisibleCx(), "px", _tracker->getActiveUnit()));
    _cy_item.get_adjustment()->set_value(Quantity::convert(_ellipse->getVisibleCy(), "px", _tracker->getActiveUnit()));
    _rx_item.get_adjustment()->set_value(Quantity::convert(_ellipse->getVisibleRx(), "px", _tracker->getActiveUnit()));
    _ry_item.get_adjustment()->set_value(Quantity::convert(_ellipse->getVisibleRy(), "px", _tracker->getActiveUnit()));
    _start_item.get_adjustment()->set_value(Geom::deg_from_rad(Geom::Angle{_ellipse->start}.radians0()));
    _end_item.get_adjustment()->set_value(Geom::deg_from_rad(Geom::Angle{_ellipse->end}.radians0()));
    _type_buttons[_ellipse->arc_type]->set_active();

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
