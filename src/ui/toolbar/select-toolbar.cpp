// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Select toolbar
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2003-2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "select-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/togglebutton.h>

#include "desktop.h"
#include "document-undo.h"
#include "object/sp-item-transform.h"
#include "page-manager.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/util.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/spinbutton.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::Util::Quantity;
using Inkscape::DocumentUndo;

namespace Inkscape::UI::Toolbar {

SelectToolbar::SelectToolbar()
    : SelectToolbar{create_builder("toolbar-select.ui")}
{}

SelectToolbar::SelectToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "select-toolbar")}
    , _tracker{std::make_unique<UnitTracker>(Util::UNIT_TYPE_LINEAR)}
    , _action_prefix{"selector:toolbar:"}
    , _select_touch_btn{get_widget<Gtk::ToggleButton>(builder, "_select_touch_btn")}
    , _transform_stroke_btn{get_widget<Gtk::ToggleButton>(builder, "_transform_stroke_btn")}
    , _transform_corners_btn{get_widget<Gtk::ToggleButton>(builder, "_transform_corners_btn")}
    , _transform_gradient_btn{get_widget<Gtk::ToggleButton>(builder, "_transform_gradient_btn")}
    , _transform_pattern_btn{get_widget<Gtk::ToggleButton>(builder, "_transform_pattern_btn")}
    , _x_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_x_item")}
    , _y_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_y_item")}
    , _w_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_w_item")}
    , _h_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_h_item")}
    , _lock_btn{get_widget<Gtk::ToggleButton>(builder, "_lock_btn")}
{
    auto prefs = Preferences::get();

    setup_derived_spin_button(_x_item, "X");
    setup_derived_spin_button(_y_item, "Y");
    setup_derived_spin_button(_w_item, "width");
    setup_derived_spin_button(_h_item, "height");

    auto unit_menu = _tracker->create_unit_dropdown();
    get_widget<Gtk::Box>(builder, "unit_menu_box").append(*unit_menu);

    _select_touch_btn.set_active(prefs->getBool("/tools/select/touch_box", false));
    _select_touch_btn.signal_toggled().connect(sigc::mem_fun(*this, &SelectToolbar::toggle_touch));

    _tracker->addUnit(Util::UnitTable::get().unit("%"));

    // Use StyleContext to check if the child is a context item (an item that is disabled if there is no selection).
    for (auto &child : UI::children(_toolbar)) {
        if (child.has_css_class("context_item")) {
            _context_items.push_back(&child);
        }
    }

    _transform_stroke_btn.set_active(prefs->getBool("/options/transform/stroke", true));
    _transform_stroke_btn.signal_toggled().connect(sigc::mem_fun(*this, &SelectToolbar::toggle_stroke));

    _transform_corners_btn.set_active(prefs->getBool("/options/transform/rectcorners", true));
    _transform_corners_btn.signal_toggled().connect(sigc::mem_fun(*this, &SelectToolbar::toggle_corners));

    _transform_gradient_btn.set_active(prefs->getBool("/options/transform/gradient", true));
    _transform_gradient_btn.signal_toggled().connect(sigc::mem_fun(*this, &SelectToolbar::toggle_gradient));

    _transform_pattern_btn.set_active(prefs->getBool("/options/transform/pattern", true));
    _transform_pattern_btn.signal_toggled().connect(sigc::mem_fun(*this, &SelectToolbar::toggle_pattern));

    _lock_btn.signal_toggled().connect(sigc::mem_fun(*this, &SelectToolbar::toggle_lock));
    _lock_btn.set_active(prefs->getBool("/tools/select/lock_aspect_ratio", false));
    toggle_lock();

    _box_observer = prefs->createObserver("/tools/bounding_box", [this](const Preferences::Entry& entry) {
        layout_widget_update();
    });

    _initMenuBtns();
}

SelectToolbar::~SelectToolbar() = default;

void SelectToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        _selection_changed_conn.disconnect();
        _selection_modified_conn.disconnect();
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        // Force update when selection changes.
        auto sel = _desktop->getSelection();
        _selection_changed_conn = sel->connectChanged(sigc::mem_fun(*this, &SelectToolbar::_selectionChanged));
        _selection_modified_conn = sel->connectModified(sigc::mem_fun(*this, &SelectToolbar::_selectionModified));

        _sensitize();
    }
}

void SelectToolbar::documentReplaced()
{
    _page_origin_conn.disconnect();
    if (_desktop) {
        auto &pm = _desktop->getDocument()->getPageManager();
        _page_origin_conn = pm.connectSelectedPageOriginChanged(
            sigc::mem_fun(*this, &SelectToolbar::layout_widget_update));
    }

    layout_widget_update();
}

void SelectToolbar::setActiveUnit(Util::Unit const *unit)
{
    _tracker->setActiveUnit(unit);
}

void SelectToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name)
{
    auto const path = "/tools/select/" + name;
    auto const val = Preferences::get()->getDouble(path, 0.0);
    auto const adj = btn.get_adjustment();
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &SelectToolbar::any_value_changed), adj));
    _tracker->addAdjustment(adj->gobj());

    btn.addUnitTracker(_tracker.get());
    btn.setDefocusTarget(this);

    // select toolbar spin buttons increment by 1.0 with key up/down, and 0.1 with spinner buttons
    btn.set_increment(1.0);
}

void SelectToolbar::_sensitize()
{
    auto const selection = _desktop->getSelection();
    bool const sensitive = selection && !selection->isEmpty();
    for (auto item : _context_items) {
        item->set_sensitive(sensitive);
    }
}

void SelectToolbar::any_value_changed(Glib::RefPtr<Gtk::Adjustment> const &adj)
{
    // quit if run by the XML listener or a unit change
    if (_blocker.pending() || _tracker->isUpdating() || !_desktop) {
        return;
    }

    // in turn, prevent XML listener from responding
    auto guard = _blocker.block();

    auto prefs = Preferences::get();
    auto selection = _desktop->getSelection();
    auto document = _desktop->getDocument();

    document->ensureUpToDate();

    auto &pm = document->getPageManager();
    auto origin = pm.getSelectedPageOrigin();

    Geom::OptRect bbox_vis = selection->visualBounds();
    Geom::OptRect bbox_geom = selection->geometricBounds();
    Geom::OptRect bbox_user = selection->preferredBounds();

    if (!bbox_user) {
        return;
    }

    auto const unit = _tracker->getActiveUnit();

    double old_w = bbox_user->width();
    double old_h = bbox_user->height();
    double new_w, new_h, new_x, new_y = 0;

    auto _adj_x = _x_item.get_adjustment();
    auto _adj_y = _y_item.get_adjustment();
    auto _adj_w = _w_item.get_adjustment();
    auto _adj_h = _h_item.get_adjustment();

    if (unit->type == Util::UNIT_TYPE_LINEAR) {
        new_w = Quantity::convert(_adj_w->get_value(), unit, "px");
        new_h = Quantity::convert(_adj_h->get_value(), unit, "px");
        new_x = Quantity::convert(_adj_x->get_value(), unit, "px");
        new_y = Quantity::convert(_adj_y->get_value(), unit, "px");

    } else {
        double old_x = bbox_user->min()[Geom::X] + (old_w * selection->anchor.x());
        double old_y = bbox_user->min()[Geom::Y] + (old_h * selection->anchor.y());

        // Adjust against selected page origin, so later correction isn't broken.
        old_x -= origin.x();
        old_y -= origin.y();

        new_x = old_x * (_adj_x->get_value() / 100 / unit->factor);
        new_y = old_y * (_adj_y->get_value() / 100 / unit->factor);
        new_w = old_w * (_adj_w->get_value() / 100 / unit->factor);
        new_h = old_h * (_adj_h->get_value() / 100 / unit->factor);
    }

    // Adjust depending on the selected anchor.
    double x0 = (new_x - (old_w * selection->anchor.x())) - ((new_w - old_w) * selection->anchor.x());
    double y0 = (new_y - (old_h * selection->anchor.y())) - ((new_h - old_h) * selection->anchor.y());

    // Adjust according to the selected page origin
    x0 += origin.x();
    y0 += origin.y();

    double x1 = x0 + new_w;
    double xrel = new_w / old_w;
    double y1 = y0 + new_h;
    double yrel = new_h / old_h;

    // Keep proportions if lock is on
    if (_lock_btn.get_active()) {
        if (adj == _adj_h) {
            x1 = x0 + yrel * bbox_user->dimensions()[Geom::X];
        } else if (adj == _adj_w) {
            y1 = y0 + xrel * bbox_user->dimensions()[Geom::Y];
        }
    }

    // scales and moves, in px
    double mh = fabs(x0 - bbox_user->min()[Geom::X]);
    double sh = fabs(x1 - bbox_user->max()[Geom::X]);
    double mv = fabs(y0 - bbox_user->min()[Geom::Y]);
    double sv = fabs(y1 - bbox_user->max()[Geom::Y]);

    // unless the unit is %, convert the scales and moves to the unit
    if (unit->type == Util::UNIT_TYPE_LINEAR) {
        mh = Quantity::convert(mh, "px", unit);
        sh = Quantity::convert(sh, "px", unit);
        mv = Quantity::convert(mv, "px", unit);
        sv = Quantity::convert(sv, "px", unit);
    }

    auto const actionkey = get_action_key(mh, sh, mv, sv);

    if (actionkey) {

        bool transform_stroke = prefs->getBool("/options/transform/stroke", true);
        bool preserve = prefs->getBool("/options/preservetransform/value", false);

        Geom::Affine scaler;
        if (prefs->getInt("/tools/bounding_box") == 0) { // SPItem::VISUAL_BBOX
            scaler = get_scale_transform_for_variable_stroke(*bbox_vis, *bbox_geom, transform_stroke, preserve, x0, y0, x1, y1);
        } else {
            // 1) We could have use the newer get_scale_transform_for_variable_stroke() here, but to avoid regressions
            // we'll just use the old get_scale_transform_for_uniform_stroke() for now.
            // 2) get_scale_transform_for_uniform_stroke() is intended for visual bounding boxes, not geometrical ones!
            // we'll trick it into using a geometric bounding box though, by setting the stroke width to zero
            scaler = get_scale_transform_for_uniform_stroke(*bbox_geom, 0, 0, false, false, x0, y0, x1, y1);
        }

        selection->applyAffine(scaler);
        DocumentUndo::maybeDone(document, actionkey, RC_("Undo", "Transform by toolbar"), INKSCAPE_ICON("tool-pointer"));
    }
}

void SelectToolbar::layout_widget_update()
{
    if (_blocker.pending() || !_desktop) {
        return;
    }

    auto guard = _blocker.block();
    auto sel = _desktop->getSelection();

    if (sel && !sel->isEmpty()) {
        if (auto const bbox = sel->preferredBounds()) {
            auto const unit = _tracker->getActiveUnit();

            auto width = bbox->width();
            auto height = bbox->height();
            auto x = bbox->left() + width * sel->anchor.x();
            auto y = bbox->top() + height * sel->anchor.y();

            auto &pm = _desktop->getDocument()->getPageManager();
            auto origin = pm.getSelectedPageOrigin();
            x -= origin.x();
            y -= origin.y();

            auto _adj_x = _x_item.get_adjustment();
            auto _adj_y = _y_item.get_adjustment();
            auto _adj_w = _w_item.get_adjustment();
            auto _adj_h = _h_item.get_adjustment();

            if (unit->type == Util::UNIT_TYPE_DIMENSIONLESS) {
                double const val = unit->factor * 100;
                _adj_x->set_value(val);
                _adj_y->set_value(val);
                _adj_w->set_value(val);
                _adj_h->set_value(val);
                _tracker->setFullVal(_adj_x->gobj(), x);
                _tracker->setFullVal(_adj_y->gobj(), y);
                _tracker->setFullVal(_adj_w->gobj(), width);
                _tracker->setFullVal(_adj_h->gobj(), height);
            } else {
                _adj_x->set_value(Quantity::convert(x, "px", unit));
                _adj_y->set_value(Quantity::convert(y, "px", unit));
                _adj_w->set_value(Quantity::convert(width, "px", unit));
                _adj_h->set_value(Quantity::convert(height, "px", unit));
            }
        }
    }
}

void SelectToolbar::_selectionChanged(Selection *selection)
{
    assert(_desktop->getSelection() == selection);
    layout_widget_update();
    _sensitize();
}

void SelectToolbar::_selectionModified(Selection *selection, unsigned flags)
{
    assert(_desktop->getSelection() == selection);
    if (flags & (SP_OBJECT_MODIFIED_FLAG        |
                 SP_OBJECT_PARENT_MODIFIED_FLAG |
                 SP_OBJECT_CHILD_MODIFIED_FLAG  ))
    {
        layout_widget_update();
    }
}

char const *SelectToolbar::get_action_key(double mh, double sh, double mv, double sv)
{
    // do the action only if one of the scales/moves is greater than half the last significant
    // digit in the spinbox (currently spinboxes have 3 fractional digits, so that makes 0.0005). If
    // the value was changed by the user, the difference will be at least that much; otherwise it's
    // just rounding difference between the spinbox value and actual value, so no action is
    // performed
    double const threshold = 5e-4;
    char const *const action = mh > threshold ? "move:horizontal:" :
                               sh > threshold ? "scale:horizontal:" :
                               mv > threshold ? "move:vertical:" :
                               sv > threshold ? "scale:vertical:" : nullptr;
    if (!action) {
        return nullptr;
    }
    _action_key = _action_prefix + action;
    return _action_key.c_str();
}

void SelectToolbar::toggle_lock()
{
    Preferences::get()->setBool("/tools/select/lock_aspect_ratio", _lock_btn.get_active());

    _lock_btn.set_image_from_icon_name(_lock_btn.get_active() ? "object-locked" : "object-unlocked");
}

void SelectToolbar::toggle_touch()
{
    Preferences::get()->setBool("/tools/select/touch_box", _select_touch_btn.get_active());
}

void SelectToolbar::toggle_stroke()
{
    bool active = _transform_stroke_btn.get_active();
    Preferences::get()->setBool("/options/transform/stroke", active);
    if (active) {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>stroke width</b> is <b>scaled</b> when objects are scaled."));
    } else {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>stroke width</b> is <b>not scaled</b> when objects are scaled."));
    }
}

void SelectToolbar::toggle_corners()
{
    bool active = _transform_corners_btn.get_active();
    Preferences::get()->setBool("/options/transform/rectcorners", active);
    if (active) {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>rounded rectangle corners</b> are <b>scaled</b> when rectangles are scaled."));
    } else {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>rounded rectangle corners</b> are <b>not scaled</b> when rectangles are scaled."));
    }
}

void SelectToolbar::toggle_gradient()
{
    bool active = _transform_gradient_btn.get_active();
    Preferences::get()->setBool("/options/transform/gradient", active);
    if (active) {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>gradients</b> are <b>transformed</b> along with their objects when those are transformed (moved, scaled, rotated, or skewed)."));
    } else {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>gradients</b> remain <b>fixed</b> when objects are transformed (moved, scaled, rotated, or skewed)."));
    }
}

void SelectToolbar::toggle_pattern()
{
    bool active = _transform_pattern_btn.get_active();
    Preferences::get()->setInt("/options/transform/pattern", active);
    if (active) {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>patterns</b> are <b>transformed</b> along with their objects when those are transformed (moved, scaled, rotated, or skewed)."));
    } else {
        _desktop->messageStack()->flash(INFORMATION_MESSAGE, _("Now <b>patterns</b> remain <b>fixed</b> when objects are transformed (moved, scaled, rotated, or skewed)."));
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
