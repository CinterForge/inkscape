// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "spiral-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/label.h>

#include "desktop.h"
#include "document-undo.h"
#include "preferences.h"

#include "object/sp-spiral.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/spinbutton.h"
#include "xml/node.h"

using Inkscape::DocumentUndo;

namespace Inkscape::UI::Toolbar {

SpiralToolbar::SpiralToolbar()
    : SpiralToolbar{create_builder("toolbar-spiral.ui")}
{}

SpiralToolbar::SpiralToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "spiral-toolbar")}
    , _mode_item{get_widget<Gtk::Label>(builder, "_mode_item")}
    , _revolution_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_revolution_item")}
    , _expansion_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_expansion_item")}
    , _t0_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_t0_item")}
{
    _setupDerivedSpinButton(_revolution_item, "revolution", 3.0);
    _setupDerivedSpinButton(_expansion_item, "expansion", 1.0);
    _setupDerivedSpinButton(_t0_item, "t0", 0.0);

    _revolution_item.set_custom_numeric_menu_data({
        {0.01, _("just a curve")},
        {0.5, ""},
        {1, _("one full revolution")},
        {2, ""},
        {3, ""},
        {5, ""},
        {10, ""},
        {50, ""},
        {100, ""}
    });

    _expansion_item.set_custom_numeric_menu_data({
        {0, _("circle")},
        {0.1, _("edge is much denser")},
        {0.5, _("edge is denser")},
        {1, _("even")},
        {1.5, _("center is denser")},
        {5, _("center is much denser")},
        {20, ""}
    });

    _t0_item.set_custom_numeric_menu_data({
        {0, _("starts from center")},
        {0.5, _("starts mid-way")},
        {0.9, _("starts near edge")},
    });

    get_widget<Gtk::Button>(builder, "reset_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &SpiralToolbar::_setDefaults));

    _initMenuBtns();
}

void SpiralToolbar::_setupDerivedSpinButton(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value)
{
    auto const adj = btn.get_adjustment();
    auto const path = "/tools/shapes/spiral/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);
    adj->set_value(val);

    adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &SpiralToolbar::_valueChanged), adj, name));

    btn.setDefocusTarget(this);
}

void SpiralToolbar::setDesktop(SPDesktop *desktop)
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
        _selection_changed_conn = sel->connectChanged(sigc::mem_fun(*this, &SpiralToolbar::_selectionChanged));
        _selectionChanged(sel); // Synthesize an emission to trigger the update
    }
}

void SpiralToolbar::_attachRepr(XML::Node *repr)
{
    assert(!_repr);
    _repr = repr;
    GC::anchor(_repr);
    _repr->addObserver(*this);
}

void SpiralToolbar::_detachRepr()
{
    assert(_repr);
    _repr->removeObserver(*this);
    GC::release(_repr);
    _repr = nullptr;
    _cancelUpdate();
}

void SpiralToolbar::_valueChanged(Glib::RefPtr<Gtk::Adjustment> &adj, Glib::ustring const &value_name)
{
    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Preferences::get()->setDouble("/tools/shapes/spiral/" + value_name, adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    auto const namespaced_name = "sodipodi:" + value_name;

    bool modified = false;
    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPSpiral>(item)) {
            auto repr = item->getRepr();
            repr->setAttributeSvgDouble(namespaced_name, adj->get_value());
            item->updateRepr();
            modified = true;
        }
    }

    if (modified) {
        DocumentUndo::maybeDone(_desktop->getDocument(), value_name.c_str(), RC_("Undo", "Change spiral"),
                                INKSCAPE_ICON("draw-spiral"));
    }
}

void SpiralToolbar::_setDefaults()
{
    // fixme: make settable
    _revolution_item.get_adjustment()->set_value(3);
    _expansion_item.get_adjustment()->set_value(1.0);
    _t0_item.get_adjustment()->set_value(0.0);

    onDefocus();
}

void SpiralToolbar::_selectionChanged(Selection *selection)
{
    if (_repr) {
        _detachRepr();
    }

    int n_selected = 0;
    XML::Node *repr = nullptr;

    for (auto item : selection->items()) {
        if (is<SPSpiral>(item)) {
            n_selected++;
            repr = item->getRepr();
        }
    }

    _mode_item.set_markup(n_selected == 0 ? _("<b>New:</b>") : _("<b>Change:</b>"));

    if (n_selected == 1) {
        _attachRepr(repr);
        _repr->synthesizeEvents(*this);
    }
}

void SpiralToolbar::notifyAttributeChanged(XML::Node &, GQuark, Util::ptr_shared, Util::ptr_shared)
{
    assert(_repr);

    // quit if run by the UI callbacks
    if (_blocker.pending()) {
        return;
    }

    _queueUpdate();
}

void SpiralToolbar::_queueUpdate()
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

void SpiralToolbar::_cancelUpdate()
{
    if (!_tick_callback) {
        return;
    }

    remove_tick_callback(_tick_callback);
    _tick_callback = 0;
}

void SpiralToolbar::_update()
{
    assert(_repr);

    // prevent UI callbacks from responding
    auto guard = _blocker.block();

    _revolution_item.get_adjustment()->set_value(_repr->getAttributeDouble("sodipodi:revolution", 3.0));
    _expansion_item.get_adjustment()->set_value(_repr->getAttributeDouble("sodipodi:expansion", 1.0));
    _t0_item.get_adjustment()->set_value(_repr->getAttributeDouble("sodipodi:t0", 0.0));
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
