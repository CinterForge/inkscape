// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Rewrite of code originally in desktop-widget.cpp.
 */
/*
 * Author:
 *   Tavmjong Bah
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

// The scrollbars, and canvas are tightly coupled so it makes sense to have a dedicated
// widget to handle their interactions. The buttons are along for the ride. I don't see
// how to add the buttons easily via a .ui file (which would allow the user to put any
// buttons they want in their place).

#include "canvas-grid.h"

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/label.h>

#include "desktop.h" // Hopefully temp.
#include "display/control/canvas-item-guideline.h"
#include "document-undo.h"
#include "message-context.h"
#include "object/sp-grid.h"
#include "object/sp-root.h"
#include "page-manager.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/controller.h"
#include "ui/dialog/command-palette.h"
#include "ui/drag-and-drop.h"
#include "ui/tools/tool-base.h"
#include "ui/widget/canvas-notice.h"
#include "ui/widget/canvas.h"
#include "ui/widget/desktop-tab-controls.h"
#include "ui/widget/desktop-widget.h" // Hopefully temp.
#include "ui/widget/events/canvas-event.h"
#include "ui/widget/ink-ruler.h"
#include "ui/widget/stack.h"
#include "util/units.h"

namespace Inkscape::UI::Widget {

CanvasGrid::CanvasGrid(SPDesktopWidget *dtw)
    : Glib::ObjectBase("CanvasGrid")
{
    _dtw = dtw;
    set_name("CanvasGrid");

    // Tabs widget
    _tabs_widget = std::make_unique<Inkscape::UI::Widget::DesktopTabControls>(dtw);

    // Command palette
    _command_palette = std::make_unique<Inkscape::UI::Dialog::CommandPalette>();

    // Notice overlay, note using unique_ptr will cause destruction race conditions
    _notice = CanvasNotice::create();

    // Canvas overlay
    _canvas_overlay.set_child(_popoverbin);
    _canvas_overlay.add_overlay(_command_palette->get_base_widget());
    _canvas_overlay.add_overlay(*_notice);
    _canvas_overlay.set_expand();

    _canvas_stack = Gtk::make_managed<Inkscape::UI::Widget::Stack>();
    _popoverbin.setChild(_canvas_stack);
    ink_drag_setup(_dtw, _canvas_stack);

    // Horizontal Ruler
    _hruler = std::make_unique<Inkscape::UI::Widget::Ruler>(Gtk::Orientation::HORIZONTAL);
    _hruler->set_hexpand(true);
    // Tooltip/Unit set elsewhere

    // Vertical Ruler
    _vruler = std::make_unique<Inkscape::UI::Widget::Ruler>(Gtk::Orientation::VERTICAL);
    _vruler->set_vexpand(true);
    // Tooltip/Unit set elsewhere.

    // Guide Lock
    _guide_lock.set_name("LockGuides");
    _guide_lock.set_action_name("doc.lock-all-guides");
    auto set_lock_icon = [this](){
        _guide_lock.set_image_from_icon_name(_guide_lock.get_active() ? "object-locked" : "object-unlocked");
    };
    // To be replaced by Gio::Action:
    _guide_lock.signal_toggled().connect([=,this](){
        set_lock_icon();
    });
    _guide_lock.signal_clicked().connect([this] {
        bool down = !_guide_lock.get_active(); // Hack: Reversed since button state only changes after click.
        _dtw->get_desktop()->guidesMessageContext()->flash(Inkscape::NORMAL_MESSAGE, down ? _("Locked all guides") : _("Unlocked all guides"));
    });
    set_lock_icon();
    _guide_lock.set_tooltip_text(_("Toggle lock of all guides in the document"));

    // Subgrid
    _subgrid.attach(_guide_lock,     0, 0, 1, 1);
    _subgrid.attach(*_vruler,        0, 1, 1, 1);
    _subgrid.attach(*_hruler,        1, 0, 1, 1);
    _subgrid.attach(_canvas_overlay, 1, 1, 1, 1);
    _subgrid.set_expand();

    // Horizontal Scrollbar
    _hadj = Gtk::Adjustment::create(0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0);
    _hadj->signal_value_changed().connect(sigc::mem_fun(*this, &CanvasGrid::_adjustmentChanged));
    _hscrollbar = Gtk::Scrollbar(_hadj, Gtk::Orientation::HORIZONTAL);
    _hscrollbar.set_name("CanvasScrollbar");
    _hscrollbar.set_hexpand(true);

    // Vertical Scrollbar
    _vadj = Gtk::Adjustment::create(0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0);
    _vadj->signal_value_changed().connect(sigc::mem_fun(*this, &CanvasGrid::_adjustmentChanged));
    _vscrollbar = Gtk::Scrollbar(_vadj, Gtk::Orientation::VERTICAL);
    _vscrollbar.set_name("CanvasScrollbar");
    _vscrollbar.set_vexpand(true);

    // CMS Adjust (To be replaced by Gio::Action)
    _cms_adjust.set_name("CMS_Adjust");
    _cms_adjust.set_action_name("win.canvas-color-manage");
    _cms_adjust.set_tooltip_text(_("Toggle color-managed display for this document window"));
    auto set_cms_icon = [this](){
        _cms_adjust.set_image_from_icon_name(_cms_adjust.get_active() ? "color-management" : "color-management-off");
    };
    set_cms_icon();
    _cms_adjust.signal_toggled().connect([=,this](){ set_cms_icon(); });

    // popover with some common display mode related options
    _builder_display_popup = create_builder("display-popup.glade");
    auto popover     = &get_widget<Gtk::Popover>    (_builder_display_popup, "popover");
    auto sticky_zoom = &get_widget<Gtk::CheckButton>(_builder_display_popup, "zoom-resize");

    // To be replaced by Gio::Action:
    sticky_zoom->signal_toggled().connect([this](){ _dtw->sticky_zoom_toggled(); });

    _quick_actions.set_name("QuickActions");
    _quick_actions.set_popover(*popover);
    _quick_actions.set_icon_name("display-symbolic");
    _quick_actions.set_direction(Gtk::ArrowType::LEFT);
    _quick_actions.set_tooltip_text(_("Display options"));

    _quick_preview_label = &get_widget<Gtk::Label>(_builder_display_popup, "quick_preview_label");
    _quick_zoom_label = &get_widget<Gtk::Label>(_builder_display_popup, "quick_zoom_label");

    auto quick_preview_shortcut = _preview_accel.getShortcutText();
    auto quick_zoom_shortcut = _zoom_accel.getShortcutText();

    if (!quick_preview_shortcut.empty()) {
        _quick_preview_label->set_label("<b>" + quick_preview_shortcut[0] + "</b>");
    } else {
        _quick_preview_label->set_label("");
    }

    if (!quick_zoom_shortcut.empty()) {
        _quick_zoom_label->set_label("<b>" + quick_zoom_shortcut[0] + "</b>");
    } else {
        _quick_zoom_label->set_label("");
    }

    _update_preview_connection = _preview_accel.connectModified([this]() {
        if(_preview_accel.getShortcutText().empty()) {
            _quick_preview_label->set_label("");
            return;
        }
        _quick_preview_label->set_label("<b>" + _preview_accel.getShortcutText()[0] + "</b>");
    });

    _update_zoom_connection = _zoom_accel.connectModified([this]() {
        if(_zoom_accel.getShortcutText().empty()) {
            _quick_zoom_label->set_label("");
            return;
        }
        _quick_zoom_label->set_label("<b>" + _zoom_accel.getShortcutText()[0] + "</b>");
    });

    // Main grid
    attach(*_tabs_widget,  0, 0, 2, 1);
    attach(_subgrid,       0, 1, 1, 2);
    attach(_hscrollbar,    0, 3, 1, 1);
    attach(_cms_adjust,    1, 3, 1, 1);
    attach(_quick_actions, 1, 1, 1, 1);
    attach(_vscrollbar,    1, 2, 1, 1);

    // For creating guides, etc.
    auto const bind_controllers = [&](auto& ruler, RulerOrientation orientation) {
        auto const click = Gtk::GestureClick::create();
        click->set_button(1); // left
        click->signal_pressed().connect(Controller::use_state([this](auto &&...args) { return _rulerButtonPress(args...); }, *click));
        click->signal_released().connect(Controller::use_state([this, orientation](auto &&...args) { return _rulerButtonRelease(args..., orientation); }, *click));
        ruler->add_controller(click);

        auto const motion = Gtk::EventControllerMotion::create();
        motion->signal_motion().connect([this, orientation, &motion = *motion](auto &&...args) { _rulerMotion(motion, args..., orientation); });
        ruler->add_controller(motion);
    };

    bind_controllers(_hruler, RulerOrientation::horizontal);
    bind_controllers(_vruler, RulerOrientation::vertical);

    auto prefs = Inkscape::Preferences::get();
    _box_observer = prefs->createObserver("/tools/bounding_box", [this](const Preferences::Entry& entry) {
        updateRulers();
    });
}

CanvasGrid::~CanvasGrid() = default;

void CanvasGrid::addTab(Canvas *canvas)
{
    canvas->set_hexpand(true);
    canvas->set_vexpand(true);
    canvas->set_focusable(true);
    _canvas_stack->add(*canvas);
}

void CanvasGrid::removeTab(Canvas *canvas)
{
    _canvas_stack->remove(*canvas);
}

void CanvasGrid::switchTab(Canvas *canvas)
{
    if (_canvas) {
        _hruler->clear_track_widget();
        _vruler->clear_track_widget();
    }

    _canvas = canvas;

    _canvas_stack->setActive(_canvas);

    if (_canvas) {
        _hruler->set_track_widget(*_canvas);
        _vruler->set_track_widget(*_canvas);
    }
}

void CanvasGrid::on_realize()
{
    // actions should be available now
    parent_type::on_realize();

    auto const map = _dtw->get_action_map();
    if (!map) {
        g_warning("No action map available to canvas-grid");
        return;
    }

    auto const cms_action = std::dynamic_pointer_cast<Gio::SimpleAction>(map->lookup_action("canvas-color-manage"));
    auto const disp_action = std::dynamic_pointer_cast<Gio::SimpleAction>(map->lookup_action("canvas-display-mode"));
    if (!cms_action || !disp_action) {
        g_warning("No canvas-display-mode and/or canvas-color-manage action available to canvas-grid");
        return;
    }

    auto set_display_icon = [=, this] {
        int display_mode;
        disp_action->get_state<int>(display_mode);

        Glib::ustring id;
        switch (static_cast<Inkscape::RenderMode>(display_mode)) {
            case RenderMode::NORMAL:
                id = "display";
                break;
            case RenderMode::OUTLINE:
                id = "display-outline";
                break;
            case RenderMode::OUTLINE_OVERLAY:
                id = "display-outline-overlay";
                break;
            case RenderMode::VISIBLE_HAIRLINES:
                id = "display-enhance-stroke";
                break;
            case RenderMode::NO_FILTERS:
                id = "display-no-filter";
                break;
            default:
                g_warning("Unknown display mode in canvas-grid");
                return;
        }

        bool cms_mode;
        cms_action->get_state<bool>(cms_mode);

        // if CMS is ON show alternative icons
        if (cms_mode) {
            id += "-alt";
        }

        _quick_actions.set_icon_name(id + "-symbolic");
    };

    // when display mode state changes, update icon
    disp_action->property_state().signal_changed().connect([=] { set_display_icon(); });
    cms_action-> property_state().signal_changed().connect([=] { set_display_icon(); });
    set_display_icon();
}

// TODO: remove when sticky zoom gets replaced by Gio::Action:
Gtk::CheckButton *CanvasGrid::GetStickyZoom() {
    return &get_widget<Gtk::CheckButton>(_builder_display_popup, "zoom-resize");
}

// _dt2r should be a member of _canvas.
// get_display_area should be a member of _canvas.
void CanvasGrid::updateRulers()
{
    auto const desktop = _dtw->get_desktop();
    auto document = desktop->getDocument();
    auto &pm = document->getPageManager();
    auto sel = desktop->getSelection();

    // Our connections to the document are handled with a lazy pattern to avoid
    // having to refactor the SPDesktopWidget class. We know UpdateRulers is
    // called in all situations when documents are loaded and replaced.
    if (document != _document) {
        _document = document;

        _page_selected_connection = pm.connectPageSelected([this](SPPage const *) { updateRulers(); });
        _page_modified_connection = pm.connectPageModified([this](SPPage const *) { updateRulers(); });

        _sel_modified_connection.disconnect();
        _sel_changed_connection .disconnect();
        if (sel) {
            _sel_modified_connection = sel->connectModified([this](Inkscape::Selection const *, int) { updateRulers(); });
            _sel_changed_connection  = sel->connectChanged ([this](Inkscape::Selection const *     ) { updateRulers(); });
        }
    }

    Geom::Rect viewbox = _canvas->get_area_world();
    Geom::Rect startbox = viewbox;
    // Move viewbox according to the selected page's position
    auto page_transform = pm.getSelectedPageOriginAffine().inverse() * desktop->d2w();
    startbox += page_transform.translation();

    // Scale coordinates to current display units
    auto d2c_scalerot = _canvas->get_affine();
    // w2r and c2r scale should be the same
    // c2r = c2d * d2r = (1/d2c)*d2r
    double w2r_scale = _dtw->get_dt2r() / d2c_scalerot.expansionX();
    auto const rulerbox = startbox * Geom::Scale{w2r_scale};
    _hruler->set_range(rulerbox.left(), rulerbox.right());
    if (desktop->yaxisdown()) {
        _vruler->set_range(rulerbox.top(), rulerbox.bottom());
    } else {
        _vruler->set_range(-rulerbox.top(), -rulerbox.bottom());
    }

    Geom::Point pos(_canvas->get_pos());
    auto d2c = d2c_scalerot * Geom::Translate(-pos);
    auto pagebox = (pm.getSelectedPageRect() * d2c);
    _hruler->set_page(pagebox.left(), pagebox.right());
    _vruler->set_page(pagebox.top(), pagebox.bottom());

    Geom::Rect selbox = Geom::IntRect(0, 0, 0, 0);
    if (sel) {
        if (auto const bbox = sel->preferredBounds()) {
            selbox = (*bbox * d2c);
        }
    }
    _hruler->set_selection(selbox.left(), selbox.right());
    _vruler->set_selection(selbox.top(), selbox.bottom());
}

void
CanvasGrid::ShowScrollbars(bool state)
{
    if (_show_scrollbars == state) return;

    _show_scrollbars = state;
    _hscrollbar   .set_visible(_show_scrollbars);
    _vscrollbar   .set_visible(_show_scrollbars);
    _cms_adjust   .set_visible(_show_scrollbars);
    _quick_actions.set_visible(_show_scrollbars);
}

void
CanvasGrid::ToggleScrollbars()
{
    _show_scrollbars = !_show_scrollbars;
    ShowScrollbars(_show_scrollbars);

    // Will be replaced by actions
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/fullscreen/scrollbars/state", _show_scrollbars);
    prefs->setBool("/window/scrollbars/state", _show_scrollbars);
}

void
CanvasGrid::ShowRulers(bool state)
{
    if (_show_rulers == state) return;

    _show_rulers = state;

    _hruler   ->set_visible(_show_rulers);
    _vruler   ->set_visible(_show_rulers);
    _guide_lock.set_visible(_show_rulers);
}

void CanvasGrid::ToggleRulers()
{
    _show_rulers = !_show_rulers;
    ShowRulers(_show_rulers);

    // Will be replaced by actions
    auto prefs = Inkscape::Preferences::get();
    prefs->setBool("/fullscreen/rulers/state", _show_rulers);
    prefs->setBool("/window/rulers/state", _show_rulers);
}

void CanvasGrid::ToggleCommandPalette()
{
    _command_palette->toggle();
}

void CanvasGrid::showNotice(Glib::ustring const &msg, int timeout)
{
    _notice->show(msg, timeout);
}

void
CanvasGrid::ShowCommandPalette(bool state)
{
    if (state) {
        _command_palette->open();
    } else {
        _command_palette->close();
    }
}

// Update rulers on change of widget size, but only if allocation really changed.
void
CanvasGrid::size_allocate_vfunc(int const width, int const height, int const baseline)
{
    Gtk::Grid::size_allocate_vfunc(width, height, baseline);

    if (std::exchange(_width , width ) != width  ||
        std::exchange(_height, height) != height)
    {
        updateRulers();
    }
}

Geom::IntPoint CanvasGrid::_rulerToCanvas(bool horiz) const
{
    Geom::Point result;
    (horiz ? _hruler : _vruler)->translate_coordinates(*_canvas, 0, 0, result.x(), result.y());
    return result.round();
}

// Start guide creation by dragging from ruler.
Gtk::EventSequenceState CanvasGrid::_rulerButtonPress(Gtk::GestureClick const &gesture,
                                                      int /*n_press*/, double x, double y)
{
    if (_ruler_clicked) {
        return Gtk::EventSequenceState::NONE;
    }

    auto const state = gesture.get_current_event_state();

    _ruler_clicked = true;
    _ruler_dragged = false;
    _ruler_ctrl_clicked = Controller::has_flag(state, Gdk::ModifierType::CONTROL_MASK);
    _ruler_drag_origin = Geom::Point(x, y).floor();

    return Gtk::EventSequenceState::CLAIMED;
}

void CanvasGrid::_createGuideItem(Geom::Point const &pos, bool horiz)
{
    auto const desktop = _dtw->get_desktop();

    // Ensure new guide is visible
    desktop->getNamedView()->setShowGuides(true);

    // Calculate the normal of the guidelines when dragged from the edges of rulers.
    auto const y_dir = desktop->yaxisdir();
    auto normal_bl_to_tr = Geom::Point( 1, y_dir).normalized(); // Bottom-left to top-right
    auto normal_tr_to_bl = Geom::Point(-1, y_dir).normalized(); // Top-right to bottom-left

    if (auto const grid = desktop->getNamedView()->getFirstEnabledGrid();
        grid && grid->getType() == GridType::AXONOMETRIC)
    {
        auto const angle_x = Geom::rad_from_deg(grid->getAngleX());
        auto const angle_z = Geom::rad_from_deg(grid->getAngleZ());
        if (_ruler_ctrl_clicked) {
            // guidelines normal to gridlines
            normal_bl_to_tr = Geom::Point::polar(angle_x * y_dir, 1.0);
            normal_tr_to_bl = Geom::Point::polar(-angle_z * y_dir, 1.0);
        } else {
            normal_bl_to_tr = Geom::Point::polar(-angle_z * y_dir, 1.0).cw();
            normal_tr_to_bl = Geom::Point::polar(angle_x * y_dir, 1.0).cw();
        }
    }

    if (horiz) {
        if (pos.x() < 50) {
            _normal = normal_bl_to_tr;
        } else if (pos.x() > _canvas->get_width() - 50) {
            _normal = normal_tr_to_bl;
        } else {
            _normal = Geom::Point(0, 1);
        }
    } else {
        if (pos.y() < 50) {
            _normal = normal_bl_to_tr;
        } else if (pos.y() > _canvas->get_height() - 50) {
            _normal = normal_tr_to_bl;
        } else {
            _normal = Geom::Point(1, 0);
        }
    }

    _active_guide = make_canvasitem<CanvasItemGuideLine>(desktop->getCanvasGuides(), Glib::ustring(), Geom::Point(), Geom::Point());
    _active_guide->set_stroke(desktop->getNamedView()->getGuideHiColor().toRGBA());
}

void CanvasGrid::_rulerMotion(Gtk::EventControllerMotion const &controller, double x, double y, RulerOrientation orientation)
{
    if (!_ruler_clicked) {
        return;
    }

    // Get the position in canvas coordinates.
    auto const horiz = orientation == RulerOrientation::horizontal;
    auto const pos = Geom::Point(x, y) + _rulerToCanvas(horiz);

    if (!_ruler_dragged) {
        // Discard small movements without starting a drag.
        auto prefs = Preferences::get();
        int tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);
        if (Geom::LInfty(Geom::Point(x, y).floor() - _ruler_drag_origin) < tolerance) {
            return;
        }

        _createGuideItem(pos, horiz);

        _ruler_dragged = true;
    }

    // Synthesize the CanvasEvent.
    auto event = MotionEvent();
    event.modifiers = (unsigned)controller.get_current_event_state();
    event.device = controller.get_current_event_device();
    event.pos = pos;
    event.time = controller.get_current_event_time();
    event.extinput = extinput_from_gdkevent(*controller.get_current_event());

    rulerMotion(event, horiz);
}

static void ruler_snap_new_guide(SPDesktop *desktop, Geom::Point &event_dt, Geom::Point &normal)
{
    desktop->getCanvas()->grab_focus();

    auto &m = desktop->getNamedView()->snap_manager;
    m.setup(desktop);

    // We're dragging a brand new guide, just pulled out of the rulers seconds ago. When snapping to a
    // path this guide will change it slope to become either tangential or perpendicular to that path. It's
    // therefore not useful to try tangential or perpendicular snapping, so this will be disabled temporarily
    bool pref_perp = m.snapprefs.isTargetSnappable(SNAPTARGET_PATH_PERPENDICULAR);
    bool pref_tang = m.snapprefs.isTargetSnappable(SNAPTARGET_PATH_TANGENTIAL);
    m.snapprefs.setTargetSnappable(SNAPTARGET_PATH_PERPENDICULAR, false);
    m.snapprefs.setTargetSnappable(SNAPTARGET_PATH_TANGENTIAL, false);

    // We only have a temporary guide which is not stored in our document yet.
    // Because the guide snapper only looks in the document for guides to snap to,
    // we don't have to worry about a guide snapping to itself here
    auto const normal_orig = normal;
    auto snapped = m.guideFreeSnap(event_dt, normal, false, false);

    // After snapping, both event_dt and normal have been modified accordingly; we'll take the normal (of the
    // curve we snapped to) to set the normal the guide. And rotate it by 90 deg. if needed
    if (snapped && pref_perp) { // Perpendicular snapping to paths is requested by the user, so let's do that
        normal = Geom::rot90(normal);
    }

    if (!(pref_tang || pref_perp)) { // if we don't want to snap either perpendicularly or tangentially, then
        normal = normal_orig; // we must restore the normal to its original state
    }

    // Restore the preferences
    m.snapprefs.setTargetSnappable(SNAPTARGET_PATH_PERPENDICULAR, pref_perp);
    m.snapprefs.setTargetSnappable(SNAPTARGET_PATH_TANGENTIAL, pref_tang);
    m.unSetup();
}

void CanvasGrid::rulerMotion(MotionEvent const &event, bool horiz)
{
    auto const desktop = _dtw->get_desktop();

    auto const origin = horiz ? Tools::DelayedSnapEvent::GUIDE_HRULER
                              : Tools::DelayedSnapEvent::GUIDE_VRULER;

    desktop->getTool()->snap_delay_handler(this, nullptr, event, origin);
    auto const event_w = _canvas->canvas_to_world(event.pos);
    auto event_dt = _dtw->get_desktop()->w2d(event_w);

    // Update the displayed coordinates.
    desktop->set_coordinate_status(event_dt);

    if (_active_guide) {
        // Get the snapped position and normal.
        auto normal = _normal;
        if (!(event.modifiers & GDK_SHIFT_MASK)) {
            ruler_snap_new_guide(desktop, event_dt, normal);
        }

        // Apply the position and normal to the guide.
        _active_guide->set_normal(normal);
        _active_guide->set_origin(event_dt);
    }
}

void CanvasGrid::_createGuide(Geom::Point origin, Geom::Point normal)
{
    auto const desktop = _dtw->get_desktop();
    auto const xml_doc = desktop->doc()->getReprDoc();
    auto const repr = xml_doc->createElement("sodipodi:guide");

    if (desktop->getNamedView()->getLockGuides()) {
        // This condition occurs when guides are locked
        _blinkLockButton();
        desktop->getNamedView()->setLockGuides(false);
    }

    // <sodipodi:guide> stores inverted y-axis coordinates
    if (desktop->yaxisdown()) {
        origin.y() = desktop->doc()->getHeight().value("px") - origin.y();
        normal.y() *= -1.0;
    }

    // If root viewBox set, interpret guides in terms of viewBox (90/96)
    auto root = desktop->doc()->getRoot();
    if (root->viewBox_set) {
        origin.x() *= root->viewBox.width() / root->width.computed;
        origin.y() *= root->viewBox.height() / root->height.computed;
    }

    repr->setAttributePoint("position", origin);
    repr->setAttributePoint("orientation", normal);
    desktop->getNamedView()->appendChild(repr);
    GC::release(repr);
    DocumentUndo::done(desktop->getDocument(), RC_("Undo", "Create guide"), "");
}

// End guide creation or toggle guides on/off.
Gtk::EventSequenceState CanvasGrid::_rulerButtonRelease(Gtk::GestureClick const &gesture,
                                                        int /*n_press*/, double x, double y, RulerOrientation orientation)
{
    if (!_ruler_clicked) {
        return Gtk::EventSequenceState::NONE;
    }

    auto const horiz = orientation == RulerOrientation::horizontal;
    auto const desktop = _dtw->get_desktop();

    if (_active_guide) {
        desktop->getTool()->discard_delayed_snap_event();

        auto const pos = Geom::Point(x, y) + _rulerToCanvas(horiz);
        auto const state = gesture.get_current_event_state();

        // Get the snapped position and normal.
        auto const event_w = _canvas->canvas_to_world(pos);
        auto event_dt = desktop->w2d(event_w);
        auto normal = _normal;
        if (!(bool)(state & Gdk::ModifierType::SHIFT_MASK)) {
            ruler_snap_new_guide(desktop, event_dt, normal);
        }

        // Clear the guide on-canvas.
        _active_guide.reset();

        // FIXME: If possible, clear the snap indicator here too.

        // If the guide is on-screen, create the actual guide in the document.
        if (pos[horiz ? Geom::Y : Geom::X] >= 0) {
            _createGuide(event_dt, normal);
        }

        // Update the coordinate display.
        desktop->set_coordinate_status(event_dt);
    } else {
        // Ruler click (without drag) toggles the guide visibility on and off.
        desktop->getNamedView()->toggleShowGuides();
    }

    _ruler_clicked = false;
    _ruler_dragged = false;

    return Gtk::EventSequenceState::CLAIMED;
}

void CanvasGrid::_blinkLockButton()
{
   _guide_lock.get_style_context()->add_class("blink");
   _blink_lock_button_timeout = Glib::signal_timeout().connect([this] {
       _guide_lock.get_style_context()->remove_class("blink");
       return false;
   }, 500);
}

static void set_adjustment(Gtk::Adjustment *adj, double l, double u, double ps, double si, double pi)
{
    if (l != adj->get_lower() ||
        u != adj->get_upper() ||
        ps != adj->get_page_size() ||
        si != adj->get_step_increment() ||
        pi != adj->get_page_increment())
    {
        adj->set_lower(l);
        adj->set_upper(u);
        adj->set_page_size(ps);
        adj->set_step_increment(si);
        adj->set_page_increment(pi);
    }
}

void CanvasGrid::updateScrollbars(double scale)
{
    if (_updating) {
        return;
    }
    _updating = true;

    // The desktop region we always show unconditionally.
    auto const desktop = _dtw->get_desktop();
    auto const doc = desktop->doc();

    auto deskarea = *doc->preferredBounds();
    deskarea.expandBy(doc->getDimensions()); // Double size

    // The total size of pages should be added unconditionally.
    deskarea |= doc->getPageManager().getDesktopRect();

    if (Preferences::get()->getInt("/tools/bounding_box") == 0) {
        deskarea |= doc->getRoot()->desktopVisualBounds();
    } else {
        deskarea |= doc->getRoot()->desktopGeometricBounds();
    }

    // Account for desktop rotation, offset, etc - but ignore zoom
    deskarea *= desktop->d2w() * Geom::Zoom(desktop->current_zoom()).inverse();

    // Canvas region we always show unconditionally.
    double const y_dir = desktop->yaxisdir();
    auto carea = deskarea * Geom::Scale(scale, scale * y_dir);
    carea.expandBy(64);

    auto const viewbox = Geom::Rect(_canvas->get_area_world());

    // Viewbox is always included into scrollable region.
    carea |= viewbox;

    set_adjustment(_hadj.get(), carea.left(), carea.right(),
                   viewbox.width(),
                   0.1 * viewbox.width(),
                   viewbox.width());
    _hadj->set_value(viewbox.left());

    set_adjustment(_vadj.get(), carea.top(), carea.bottom(),
                   viewbox.height(),
                   0.1 * viewbox.height(),
                   viewbox.height());
    _vadj->set_value(viewbox.top());

    _updating = false;
}

void CanvasGrid::_adjustmentChanged()
{
    if (_updating) {
        return;
    }
    _updating = true;

    // Do not call canvas->scrollTo directly... messes up 'offset'.
    _dtw->get_desktop()->scroll_absolute({_hadj->get_value(), _vadj->get_value()});

    _updating = false;
}

// TODO Add actions so we can set shortcuts.
// * Sticky Zoom
// * CMS Adjust
// * Guide Lock

} // namespace Inkscape::UI::Widget

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
