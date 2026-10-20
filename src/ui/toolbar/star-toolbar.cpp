// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "star-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/sp-star.h"
#include "preferences.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::DocumentUndo;

namespace Inkscape::UI::Toolbar {

StarToolbar::StarToolbar()
    : StarToolbar{create_builder("toolbar-star.ui")}
{}

StarToolbar::StarToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "star-toolbar")}
    , _mode_item{get_widget<Gtk::Label>(builder, "_mode_item")}
    , _magnitude_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_magnitude_item")}
    , _spoke_box{get_widget<Gtk::Box>(builder, "_spoke_box")}
    , _spoke_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_spoke_item")}
    , _roundedness_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_roundedness_item")}
    , _randomization_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_randomization_item")}
    , _tracker{std::make_unique<UI::Widget::UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _length_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_length_item")}
{
    bool is_flat_sided = Preferences::get()->getBool("/tools/shapes/star/isflatsided", false);

    _magnitude_item.set_custom_numeric_menu_data({
        {2, ""},
        {3, _("triangle/tri-star")},
        {4, _("square/quad-star")},
        {5, _("pentagon/five-pointed star")},
        {6, _("hexagon/six-pointed star")},
        {7, ""},
        {8, ""},
        {10, ""},
        {12, ""},
        {20, ""}
    });

    _spoke_item.set_custom_numeric_menu_data({
        {0.010, _("thin-ray star")},
        {0.200, ""},
        {0.382, _("pentagram")},
        {0.577, _("hexagram")},
        {0.692, _("heptagram")},
        {0.765, _("octagram")},
        {1.000, _("regular polygon")}
    });

    _roundedness_item.set_custom_numeric_menu_data({
        {-1.0 , _("stretched")},
        {-0.2 , _("twisted")},
        {-0.03, _("slightly pinched")},
        { 0.0 , _("NOT rounded")},
        { 0.05, _("slightly rounded")},
        { 0.1 , _("visibly rounded")},
        { 0.2 , _("well rounded")},
        { 0.3 , _("amply rounded")},
        { 0.5 , ""},
        { 1.0 , _("stretched")},
        {10.0 , _("blown up")}
    });

    _randomization_item.set_custom_numeric_menu_data({
        { 0.00, _("NOT randomized")},
        { 0.01, _("slightly irregular")},
        { 0.10, _("visibly randomized")},
        { 0.50, _("strongly randomized")},
        {10.00, _("blown up")}
    });

    setup_derived_spin_button(_magnitude_item, "magnitude", is_flat_sided ? 3 : 2, &StarToolbar::magnitude_value_changed);
    setup_derived_spin_button(_spoke_item, "proportion", 0.5, &StarToolbar::proportion_value_changed);
    setup_derived_spin_button(_roundedness_item, "rounded", 0.0, &StarToolbar::rounded_value_changed);
    setup_derived_spin_button(_randomization_item, "randomized", 0.0, &StarToolbar::randomized_value_changed);
    setup_derived_spin_button(_length_item, "length", 0.0, &StarToolbar::length_value_changed);

    // Flatsided checkbox
    _flat_item_buttons.push_back(&get_widget<Gtk::ToggleButton>(builder, "flat_polygon_button"));
    _flat_item_buttons.push_back(&get_widget<Gtk::ToggleButton>(builder, "flat_star_button"));
    _flat_item_buttons[is_flat_sided ? 0 : 1]->set_active();

    int btn_index = 0;
    for (auto btn : _flat_item_buttons) {
        btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &StarToolbar::side_mode_changed), btn_index++));
    }

    get_widget<Gtk::Button>(builder, "turn_upright_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &StarToolbar::_turn_upright));

    get_widget<Gtk::Button>(builder, "reset_btn")
        .signal_clicked()
        .connect(sigc::mem_fun(*this, &StarToolbar::_setDefaults));

    _spoke_box.set_visible(!is_flat_sided);

    auto unit_menu = _tracker->create_unit_dropdown();
    get_widget<Gtk::Box>(builder, "unit_menu_box").append(*unit_menu);
    _tracker->addAdjustment(_length_item.get_adjustment()->gobj());

    _initMenuBtns();
}

StarToolbar::~StarToolbar() = default;

void StarToolbar::_attachRepr(XML::Node *repr)
{
    assert(!_repr);
    _repr = repr;
    GC::anchor(_repr);
    _repr->addObserver(*this);
}

void StarToolbar::_detachRepr()
{
    assert(_repr);
    _repr->removeObserver(*this);
    GC::release(_repr);
    _repr = nullptr;
}

void StarToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                            double default_value, ValueChangedMemFun value_changed_mem_fun)
{
    auto const path = "/tools/shapes/star/" + name;
    auto const val = Preferences::get()->getDouble(path, default_value);

    auto adj = btn.get_adjustment();
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));

    btn.setDefocusTarget(this);
}

void StarToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        _selection_changed_conn.disconnect();
        _selection_modified_conn.disconnect();

        if (_repr) {
            _detachRepr();
        }
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        auto sel = _desktop->getSelection();
        _selection_changed_conn = sel->connectChanged(sigc::mem_fun(*this, &StarToolbar::_selectionChanged));
        _selection_modified_conn = sel->connectChanged(sigc::mem_fun(*this, &StarToolbar::_selectionModified));
        _selectionChanged(sel); // Synthesize an emission to trigger the update
    }
}

void StarToolbar::setActiveUnit(Util::Unit const *unit)
{
    _tracker->setActiveUnit(unit);
}

void StarToolbar::side_mode_changed(int mode)
{
    bool const flat = mode == 0;

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Preferences::get()->setBool("/tools/shapes/star/isflatsided", flat);
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    auto adj = _magnitude_item.get_adjustment();
    _spoke_box.set_visible(!flat);

    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPStar>(item)) {
            auto repr = item->getRepr();
            if (flat) {
                int sides = adj->get_value();
                if (sides < 3) {
                    repr->setAttributeInt("sodipodi:sides", 3);
                }
            }
            repr->setAttribute("inkscape:flatsided", flat ? "true" : "false");

            item->updateRepr();
        }
    }

    adj->set_lower(flat ? 3 : 2);
    if (flat && adj->get_value() < 3) {
        adj->set_value(3);
    }

    if (!_batchundo) {
        DocumentUndo::done(
            _desktop->getDocument(),
            flat ? RC_("Undo", "Make polygon") : RC_("Undo", "Make star"),
            INKSCAPE_ICON("draw-polygon-star"));
    }
}

void StarToolbar::magnitude_value_changed()
{
    auto adj = _magnitude_item.get_adjustment();

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        // do not remember prefs if this call is initiated by an undo change, because undoing object
        // creation sets bogus values to its attributes before it is deleted
        Preferences::get()->setInt("/tools/shapes/star/magnitude", adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPStar>(item)) {
            auto repr = item->getRepr();
            repr->setAttributeInt("sodipodi:sides", adj->get_value());
            double arg1 = repr->getAttributeDouble("sodipodi:arg1", 0.5);
            repr->setAttributeSvgDouble("sodipodi:arg2", arg1 + M_PI / adj->get_value());
            item->updateRepr();
        }
    }

    _updateLengthValue(_desktop->getSelection());

    if (!_batchundo) {
        DocumentUndo::maybeDone(_desktop->getDocument(), "star:numcorners", RC_("Undo", "Star: Change number of corners"), INKSCAPE_ICON("draw-polygon-star"));
    }
}

void StarToolbar::proportion_value_changed()
{
    auto adj = _spoke_item.get_adjustment();

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        if (!std::isnan(adj->get_value())) {
            Preferences::get()->setDouble("/tools/shapes/star/proportion", adj->get_value());
        }
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPStar>(item)) {
            auto repr = item->getRepr();

            double r1 = repr->getAttributeDouble("sodipodi:r1", 1.0);
            double r2 = repr->getAttributeDouble("sodipodi:r2", 1.0);

            if (r2 < r1) {
                repr->setAttributeSvgDouble("sodipodi:r2", r1 * adj->get_value());
            } else {
                repr->setAttributeSvgDouble("sodipodi:r1", r2 * adj->get_value());
            }

            item->updateRepr();
        }
    }

    _updateLengthValue(_desktop->getSelection());

    if (!_batchundo) {
        DocumentUndo::maybeDone(_desktop->getDocument(), "star:spokeratio", RC_("Undo", "Star: Change spoke ratio"), INKSCAPE_ICON("draw-polygon-star"));
    }
}

void StarToolbar::rounded_value_changed()
{
    auto adj = _roundedness_item.get_adjustment();

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Preferences::get()->setDouble("/tools/shapes/star/rounded", adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPStar>(item)) {
            auto repr = item->getRepr();
            repr->setAttributeSvgDouble("inkscape:rounded", adj->get_value());
            item->updateRepr();
        }
    }

    if (!_batchundo) {
        DocumentUndo::maybeDone(_desktop->getDocument(), "star:rounding", RC_("Undo", "Star: Change rounding"), INKSCAPE_ICON("draw-polygon-star"));
    }
}

void StarToolbar::randomized_value_changed()
{
    auto adj = _randomization_item.get_adjustment();

    if (DocumentUndo::getUndoSensitive(_desktop->getDocument())) {
        Preferences::get()->setDouble("/tools/shapes/star/randomized", adj->get_value());
    }

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    // in turn, prevent listener from responding
    auto guard = _blocker.block();

    for (auto item : _desktop->getSelection()->items()) {
        if (is<SPStar>(item)) {
            auto repr = item->getRepr();
            repr->setAttributeSvgDouble("inkscape:randomized", adj->get_value());
            item->updateRepr();
        }
    }

    if (!_batchundo) {
        DocumentUndo::maybeDone(_desktop->getDocument(), "star:randomisation", RC_("Undo", "Star: Change randomization"), INKSCAPE_ICON("draw-polygon-star"));
    }
}

void StarToolbar::length_value_changed()
{
    if (!_blocker.pending() || _tracker->isUpdating()) {
        // in turn, prevent listener from responding
        auto guard = _blocker.block();

        auto adj = _length_item.get_adjustment();
        Preferences::get()->setDouble("/tools/shapes/star/length", adj->get_value());

        auto value = Util::Quantity::convert(adj->get_value(), _tracker->getActiveUnit(), "px");

        for (auto item : _desktop->getSelection()->items()) {
            if (auto star = cast<SPStar>(item)) {
                star -> setSideLength(value);
            }
        }

        DocumentUndo::maybeDone(_desktop->getDocument(), "star:length", RC_("Undo", "Star: Change edge length"),
                                INKSCAPE_ICON("draw-polygon-star"));
    }
}

void StarToolbar::_turn_upright()
{
    activate_action("app.object-star-turn-upright");
}

void StarToolbar::_setDefaults()
{
    _batchundo = true;

    // fixme: make settable in prefs!
    int mag = 5;
    double prop = 0.5;
    bool flat = false;
    double randomized = 0;
    double rounded = 0;

    _flat_item_buttons[flat ? 0 : 1]->set_active();

    _spoke_box.set_visible(!flat);

    if (_magnitude_item.get_adjustment()->get_value() == mag) {
        // Ensure handler runs even if value not changed, to reset inner handle.
        magnitude_value_changed();
    } else {
        _magnitude_item.get_adjustment()->set_value(mag);
    }
    _spoke_item.get_adjustment()->set_value(prop);
    _roundedness_item.get_adjustment()->set_value(rounded);
    _randomization_item.get_adjustment()->set_value(randomized);

    DocumentUndo::done(_desktop->getDocument(), RC_("Undo", "Star: Reset to defaults"), INKSCAPE_ICON("draw-polygon-star"));
    _batchundo = false;
}

void StarToolbar::_selectionChanged(Selection *selection)
{
    if (_repr) {
        _detachRepr();
    }

    int n_selected = 0;
    XML::Node *repr = nullptr;
    double lengths = 0;

    for (auto item : selection->items()) {
        if (auto star = cast<SPStar>(item)) {
            n_selected++;
            repr = item->getRepr();
            lengths += star->getSideLength();
        }
    }

    _mode_item.set_markup(n_selected == 0 ? _("<b>New:</b>") : _("<b>Change:</b>"));
    _length_item.set_sensitive(n_selected > 0);

    if (n_selected == 1) {
        _attachRepr(repr);
        _repr->synthesizeEvents(*this); // Fixme: BAD
    }
    _selectionModified(selection);
}

void StarToolbar::_selectionModified(Selection *selection)
{
    if (!_blocker.pending()|| _tracker->isUpdating()) {
        auto guard = _blocker.block();
        _updateLengthValue(selection);
    }
}

void StarToolbar::_updateLengthValue(Selection *selection)
{
    auto length_adj = _length_item.get_adjustment();

    int n_selected = 0;
    double lengths = 0;
    for (auto item : selection->items()) {
        if (auto star = cast<SPStar>(item)) {
            n_selected++;
            lengths += star->getSideLength();
        }
    }
    if (n_selected > 0) {
        auto value = Util::Quantity::convert(lengths / n_selected, "px", _tracker->getActiveUnit());
        length_adj->set_value(value);
    }
}

void StarToolbar::notifyAttributeChanged(XML::Node &, GQuark name_, Util::ptr_shared, Util::ptr_shared)
{
    assert(_repr);

    // quit if run by the _changed callbacks
    if (_blocker.pending()) {
        return;
    }

    auto const name = g_quark_to_string(name_);

    // in turn, prevent callbacks from responding
    auto guard = _blocker.block();

    bool isFlatSided = Preferences::get()->getBool("/tools/shapes/star/isflatsided", false);
    auto mag_adj = _magnitude_item.get_adjustment();
    auto spoke_adj = _spoke_item.get_adjustment();

    if (!strcmp(name, "inkscape:randomized")) {
        double randomized = _repr->getAttributeDouble("inkscape:randomized", 0.0);
        _randomization_item.get_adjustment()->set_value(randomized);
    } else if (!strcmp(name, "inkscape:rounded")) {
        double rounded = _repr->getAttributeDouble("inkscape:rounded", 0.0);
        _roundedness_item.get_adjustment()->set_value(rounded);
    } else if (!strcmp(name, "inkscape:flatsided")) {
        char const *flatsides = _repr->attribute("inkscape:flatsided");
        if (flatsides && !strcmp(flatsides,"false")) {
            _flat_item_buttons[1]->set_active();
            _spoke_box.set_visible(true);
            mag_adj->set_lower(2);
        } else {
            _flat_item_buttons[0]->set_active();
            _spoke_box.set_visible(false);
            mag_adj->set_lower(3);
        }
    } else if (!strcmp(name, "sodipodi:r1") || !strcmp(name, "sodipodi:r2") && !isFlatSided) {
        double r1 = _repr->getAttributeDouble("sodipodi:r1", 1.0);
        double r2 = _repr->getAttributeDouble("sodipodi:r2", 1.0);

        if (r2 < r1) {
            spoke_adj->set_value(r2 / r1);
        } else {
            spoke_adj->set_value(r1 / r2);
        }
    } else if (!strcmp(name, "sodipodi:sides")) {
        int sides = _repr->getAttributeInt("sodipodi:sides", 0);
        mag_adj->set_value(sides);
    }

    _updateLengthValue(_desktop->getSelection());
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
