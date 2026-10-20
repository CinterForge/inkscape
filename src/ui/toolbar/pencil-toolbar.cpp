// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Pencil and pen toolbars
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

#include "pencil-toolbar.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/liststore.h>
#include <gtkmm/togglebutton.h>

#include "live_effects/lpe-bendpath.h"
#include "live_effects/lpe-bspline.h"
#include "live_effects/lpe-patternalongpath.h"
#include "live_effects/lpe-powerstroke.h"
#include "live_effects/lpe-simplify.h"
#include "live_effects/lpe-spiro.h"
#include "live_effects/lpeobject-reference.h"
#include "object/sp-shape.h"
#include "selection.h"
#include "ui/builder-utils.h"
#include "ui/tools/pen-tool.h"
#include "ui/util.h"
#include "ui/widget/spinbutton.h"

namespace Inkscape::UI::Toolbar {

PencilToolbar::PencilToolbar(bool pencil_mode)
    : PencilToolbar{create_builder("toolbar-pencil.ui"), pencil_mode}
{}

PencilToolbar::PencilToolbar(Glib::RefPtr<Gtk::Builder> const &builder, bool pencil_mode)
    : Toolbar{get_widget<Gtk::Box>(builder, "pencil-toolbar")}
    , _tool_is_pencil(pencil_mode)
    , _flatten_spiro_bspline_btn(get_widget<Gtk::Button>(builder, "_flatten_spiro_bspline_btn"))
    , _usepressure_btn(get_widget<Gtk::ToggleButton>(builder, "_usepressure_btn"))
    , _minpressure_box(get_widget<Gtk::Box>(builder, "_minpressure_box"))
    , _minpressure_item(get_derived_widget<UI::Widget::SpinButton>(builder, "_minpressure_item"))
    , _maxpressure_box(get_widget<Gtk::Box>(builder, "_maxpressure_box"))
    , _maxpressure_item(get_derived_widget<UI::Widget::SpinButton>(builder, "_maxpressure_item"))
    , _tolerance_item(get_derived_widget<UI::Widget::SpinButton>(builder, "_tolerance_item"))
    , _simplify_btn(get_widget<Gtk::ToggleButton>(builder, "_simplify_btn"))
    , _flatten_simplify_btn(get_widget<Gtk::Button>(builder, "_flatten_simplify_btn"))
    , _shapescale_box(get_widget<Gtk::Box>(builder, "_shapescale_box"))
    , _shapescale_item(get_derived_widget<UI::Widget::SpinButton>(builder, "_shapescale_item"))
    , _shape_box(get_widget<Gtk::Box>(builder, "shape_box"))
    , _shape_item(get_derived_widget<UI::Widget::DropDownList>(builder, "shape-list"))
    , _cap_box(get_widget<Gtk::Box>(builder, "powerstroke_cap_box"))
    , _cap_item(get_derived_widget<UI::Widget::DropDownList>(builder, "cap-list"))
{
    auto prefs = Preferences::get();

    // Configure mode buttons
    int btn_index = 0;
    for (auto &item : children(get_widget<Gtk::Box>(builder, "mode_buttons_box"))) {
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        _mode_buttons.push_back(&btn);
        btn.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PencilToolbar::mode_changed), btn_index++));
    }

    // Configure LPE bspline spiro flatten button.
    _flatten_spiro_bspline_btn.signal_clicked().connect([this] {
        _flattenLPE<LivePathEffect::LPEBSpline, LivePathEffect::LPESpiro>();
    });

    int freehandMode = prefs->getInt(freehand_tool_name() + "/freehand-mode", 0);

    // freehandMode range is (0,5] for the pen tool, (0,3] for the pencil tool
    // freehandMode = 3 is an old way of signifying pressure, set it to 0.
    _mode_buttons[freehandMode < _mode_buttons.size() ? freehandMode : 0]->set_active();

    if (_tool_is_pencil) {
        // Setup the spin buttons.
        setup_derived_spin_button(_minpressure_item, "minpressure", 0, &PencilToolbar::minpressure_value_changed);
        setup_derived_spin_button(_maxpressure_item, "maxpressure", 30, &PencilToolbar::maxpressure_value_changed);
        setup_derived_spin_button(_tolerance_item, "tolerance", 3.0, &PencilToolbar::tolerance_value_changed);

        _minpressure_item.set_custom_numeric_menu_data({});
        _maxpressure_item.set_custom_numeric_menu_data({});

        // Smoothing
        _tolerance_item.set_custom_numeric_menu_data({
            {1, _("(many nodes, rough)")},
            {10, _("(default)")},
            {20, ""},
            {30, ""},
            {50, ""},
            {75, ""},
            {100, _("(few nodes, smooth)")}
        });

        // Configure usepressure button.
        bool pressure = prefs->getBool("/tools/freehand/pencil/pressure", false);
        _usepressure_btn.set_active(pressure);
        _usepressure_btn.signal_toggled().connect(sigc::mem_fun(*this, &PencilToolbar::use_pencil_pressure));

        // Powerstoke combo item.
        add_powerstroke_cap(builder);

        // Configure LPE simplify based tolerance button.
        _simplify_btn.set_active(prefs->getInt("/tools/freehand/pencil/simplify", 0));
        _simplify_btn.signal_toggled().connect(sigc::mem_fun(*this, &PencilToolbar::simplify_lpe));

        // Configure LPE simplify flatten button.
        _flatten_simplify_btn.signal_clicked().connect([this] { _flattenLPE<LivePathEffect::LPESimplify>(); });
    }

    // Advanced shape options.
    add_shape_option(builder);

    // Setup the spin buttons.
    setup_derived_spin_button(_shapescale_item, "shapescale", 2.0, &PencilToolbar::shapewidth_value_changed);

    // Values auto-calculated.
    _shapescale_item.set_custom_numeric_menu_data({});

    hide_extra_widgets(builder);

    _initMenuBtns();
}

void PencilToolbar::add_powerstroke_cap(Glib::RefPtr<Gtk::Builder> const &builder) {
    // Powerstroke cap combo tool item.
    for (auto item : std::vector<char const *>{C_("Cap", "Butt"), _("Square"), _("Round"), _("Peak"), _("Zero width")}) {
        _cap_item.append(item);
    }

    auto prefs = Preferences::get();

    int cap = prefs->getInt("/live_effects/powerstroke/powerpencilcap", 2);
    _cap_item.set_selected(cap);

    _cap_item.signal_changed().connect([this] { change_cap(_cap_item.get_selected()); });
}

void PencilToolbar::add_shape_option(Glib::RefPtr<Gtk::Builder> const &builder)
{
    std::array<const char*, 7> freehand_shape_dropdown_items_list = {(C_("Freehand shape", "None")),
                                                               _("Triangle in"),
                                                               _("Triangle out"),
                                                               _("Ellipse"),
                                                               _("From clipboard"),
                                                               _("Bend from clipboard"),
                                                               _("Last applied")};

    for (auto item : freehand_shape_dropdown_items_list) {
        _shape_item.append(item);
    }

    int shape =
        Preferences::get()->getInt(_tool_is_pencil ? "/tools/freehand/pencil/shape" : "/tools/freehand/pen/shape", 0);
    _shape_item.set_selected(shape);

    _shape_item.signal_changed().connect([this] { change_shape(_shape_item.get_selected()); });
}

void PencilToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name,
                                              double default_value, ValueChangedMemFun value_changed_mem_fun)
{
    auto const prefs = Preferences::get();
    auto const path = "/tools/freehand/pencil/" + name;
    auto const val = prefs->getDouble(path, default_value);

    auto adj = btn.get_adjustment();
    adj->set_value(val);
    adj->signal_value_changed().connect(sigc::mem_fun(*this, value_changed_mem_fun));

    btn.setDefocusTarget(this);
}

void PencilToolbar::hide_extra_widgets(Glib::RefPtr<Gtk::Builder> const &builder)
{
    auto const pen_only_items = std::vector<Gtk::Widget *>{
        &get_widget<Gtk::Widget>(builder, "zigzag_btn"),
        &get_widget<Gtk::Widget>(builder, "paraxial_btn")
    };

    auto const pencil_only_items = std::vector<Gtk::Widget *>{
        &get_widget<Gtk::Widget>(builder, "pencil_only_box")
    };

    for (auto child : pen_only_items) {
        child->set_visible(!_tool_is_pencil);
    }

    for (auto child : pencil_only_items) {
        child->set_visible(_tool_is_pencil);
    }

    // Elements must be hidden after being initially visible.
    int freehandMode = Preferences::get()->getInt(freehand_tool_name() + "/freehand-mode", 0);

    if (freehandMode != 1 && freehandMode != 2) {
        _flatten_spiro_bspline_btn.set_visible(false);
    }
    if (_tool_is_pencil) {
        use_pencil_pressure();
    }
}

PencilToolbar::~PencilToolbar() = default;

void PencilToolbar::setDesktop(SPDesktop *desktop)
{
    Toolbar::setDesktop(desktop);

    if (_desktop) {
        if (!_set_shape) {
            int shape = Preferences::get()->getInt(freehand_tool_name() + "/shape", 0);
            update_width_value(shape);
            _set_shape = true;
        }
    }
}

void PencilToolbar::mode_changed(int mode)
{
    Preferences::get()->setInt(freehand_tool_name() + "/freehand-mode", mode);

    _flatten_spiro_bspline_btn.set_visible(mode == 1 || mode == 2);

    bool visible = mode != 2;

    _simplify_btn.set_visible(visible);
    _flatten_simplify_btn.set_visible(visible && _simplify_btn.get_active());

    // Recall, the PencilToolbar is also used as the PenToolbar with minor changes.
    if (auto pt = dynamic_cast<Tools::PenTool *>(_desktop->getTool())) {
        pt->setPolylineMode();
    }
}

// This is used in generic functions below to share large portions of code between pen and pencil tool.
Glib::ustring PencilToolbar::freehand_tool_name() const
{
    return _tool_is_pencil ? "/tools/freehand/pencil" : "/tools/freehand/pen";
}

void PencilToolbar::minpressure_value_changed()
{
    assert(_tool_is_pencil);

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    Preferences::get()->setDouble("/tools/freehand/pencil/minpressure", _minpressure_item.get_adjustment()->get_value());
}

void PencilToolbar::maxpressure_value_changed()
{
    assert(_tool_is_pencil);

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    Preferences::get()->setDouble("/tools/freehand/pencil/maxpressure", _maxpressure_item.get_adjustment()->get_value());
}

void PencilToolbar::shapewidth_value_changed()
{
    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    auto prefs = Preferences::get();
    auto selection = _desktop->getSelection();
    auto lpeitem = cast<SPLPEItem>(selection->singleItem());
    double width = _shapescale_item.get_adjustment()->get_value();

    using namespace LivePathEffect;
    switch (_shape_item.get_selected()) {
        case Tools::TRIANGLE_IN:
        case Tools::TRIANGLE_OUT:
            prefs->setDouble("/live_effects/powerstroke/width", width);
            if (lpeitem) {
                if (auto effect = dynamic_cast<LPEPowerStroke *>(lpeitem->getFirstPathEffectOfType(POWERSTROKE))) {
                    auto points = effect->offset_points.data();
                    if (points.size() == 1) {
                        points[0].y() = width;
                        effect->offset_points.param_set_and_write_new_value(points);
                    }
                }
            }
            break;
        case Tools::ELLIPSE:
        case Tools::CLIPBOARD:
            // The scale of the clipboard isn't known, so getting it to the right size isn't possible.
            prefs->setDouble("/live_effects/skeletal/width", width);
            if (lpeitem) {
                if (auto effect = dynamic_cast<LPEPatternAlongPath *>(lpeitem->getFirstPathEffectOfType(PATTERN_ALONG_PATH))) {
                    effect->prop_scale.param_set_value(width);
                    sp_lpe_item_update_patheffect(lpeitem, false, true);
                }
            }
            break;
        case Tools::BEND_CLIPBOARD:
            prefs->setDouble("/live_effects/bend_path/width", width);
            if (lpeitem) {
                if (auto effect = dynamic_cast<LPEBendPath *>(lpeitem->getFirstPathEffectOfType(BEND_PATH))) {
                    effect->prop_scale.param_set_value(width);
                    sp_lpe_item_update_patheffect(lpeitem, false, true);
                }
            }
            break;
        case Tools::NONE:
        case Tools::LAST_APPLIED:
        default:
            break;
    }
}

void PencilToolbar::use_pencil_pressure()
{
    assert(_tool_is_pencil);

    bool pressure = _usepressure_btn.get_active();
    auto prefs = Preferences::get();
    prefs->setBool("/tools/freehand/pencil/pressure", pressure);

    _minpressure_box.set_visible(pressure);
    _maxpressure_box.set_visible(pressure);
    _cap_box.set_visible(pressure);
    _shape_box.set_visible(!pressure);
    _shapescale_box.set_visible(!pressure);

    if (pressure) {
        _simplify_btn.set_visible(false);
        _flatten_spiro_bspline_btn.set_visible(false);
        _flatten_simplify_btn.set_visible(false);
    } else {
        int freehandMode = prefs->getInt("/tools/freehand/pencil/freehand-mode", 0);
        bool simplify_visible = freehandMode != 2;
        _simplify_btn.set_visible(simplify_visible);
        _flatten_simplify_btn.set_visible(simplify_visible && _simplify_btn.get_active());
        _flatten_spiro_bspline_btn.set_visible(freehandMode == 1 || freehandMode == 2);
    }

    for (auto button : _mode_buttons) {
        button->set_sensitive(!pressure);
    }
}

void PencilToolbar::change_shape(int shape)
{
    Preferences::get()->setInt(freehand_tool_name() + "/shape", shape);
    update_width_value(shape);
}

void PencilToolbar::update_width_value(int shape)
{
    // Update shape width with correct width.
    auto prefs = Preferences::get();
    double width = 1.0;
    _shapescale_item.set_sensitive(true);
    double powerstrokedefsize = 10 / (0.265 * _desktop->getDocument()->getDocumentScale()[0] * 2.0);
    switch (shape) {
        case Tools::TRIANGLE_IN:
        case Tools::TRIANGLE_OUT:
            width = prefs->getDouble("/live_effects/powerstroke/width", powerstrokedefsize);
            break;
        case Tools::ELLIPSE:
        case Tools::CLIPBOARD:
            width = prefs->getDouble("/live_effects/skeletal/width", 1.0);
            break;
        case Tools::BEND_CLIPBOARD:
            width = prefs->getDouble("/live_effects/bend_path/width", 1.0);
            break;
        case Tools::NONE: // Apply width from style?
        case Tools::LAST_APPLIED:
        default:
            _shapescale_item.set_sensitive(false);
            break;
    }
    _shapescale_item.get_adjustment()->set_value(width);
}

void PencilToolbar::change_cap(int cap)
{
    Preferences::get()->setInt("/live_effects/powerstroke/powerpencilcap", cap);
}

void PencilToolbar::simplify_lpe()
{
    bool simplify = _simplify_btn.get_active();
    Preferences::get()->setBool(freehand_tool_name() + "/simplify", simplify);
    _flatten_simplify_btn.set_visible(simplify);
}

template <typename... T>
void PencilToolbar::_flattenLPE()
{
    for (auto const item : _desktop->getSelection()->items()) {
        auto const shape = cast<SPShape>(item);
        if (shape && shape->hasPathEffect()){
            auto const lpelist = shape->getEffectList();
            for (auto const i : lpelist) {
                if (auto const lpeobj = i->lpeobject) {
                    auto const lpe = lpeobj->get_lpe();
                    if ((dynamic_cast<T const *>(lpe) || ...)) { // if lpe is any T
                        auto c = *shape->curveForEdit();
                        lpe->doEffect(c);
                        shape->setCurrentPathEffect(i);
                        if (lpelist.size() > 1) {
                            shape->removeCurrentPathEffect(true);
                            shape->setCurveBeforeLPE(std::move(c));
                        } else {
                            shape->removeCurrentPathEffect(false);
                            shape->setCurve(std::move(c));
                        }
                        sp_lpe_item_update_patheffect(shape, false, false);
                        break;
                    }
                }
            }
        }
    }
}

void PencilToolbar::tolerance_value_changed()
{
    assert(_tool_is_pencil);

    // quit if run by the attr_changed listener
    if (_blocker.pending()) {
        return;
    }

    auto const tol_pref = _tolerance_item.get_adjustment()->get_value();
    auto const tol = tol_pref / (100.0 * (102.0 - tol_pref));
    auto const tol_str = (std::ostringstream{} << tol).str();

    {
        // in turn, prevent listener from responding
        auto guard = _blocker.block();
        Preferences::get()->setDouble("/tools/freehand/pencil/tolerance", tol_pref);
    }

    for (auto const item : _desktop->getSelection()->items()) {
        auto const lpeitem = cast<SPLPEItem>(item);
        if (lpeitem && lpeitem->hasPathEffect()) {
            if (auto const simplify = lpeitem->getFirstPathEffectOfType(LivePathEffect::SIMPLIFY)) {
                if (auto const lpe_simplify = dynamic_cast<LivePathEffect::LPESimplify *>(simplify->getLPEObj()->get_lpe())) {

                    bool simplified = false;
                    if (auto const powerstroke = lpeitem->getFirstPathEffectOfType(LivePathEffect::POWERSTROKE)) {
                        if (auto const lpe_powerstroke = dynamic_cast<LivePathEffect::LPEPowerStroke *>(powerstroke->getLPEObj()->get_lpe())) {
                            lpe_powerstroke->getRepr()->setAttribute("is_visible", "false");
                            sp_lpe_item_update_patheffect(lpeitem, false, false);
                            if (auto const sp_shape = cast<SPShape>(lpeitem)) {
                                auto const previous_curve_length = sp_shape->curve()->curveCount();
                                lpe_simplify->getRepr()->setAttribute("threshold", tol_str);
                                sp_lpe_item_update_patheffect(lpeitem, false, false);
                                simplified = true;
                                auto const curve_length = sp_shape->curve()->curveCount();
                                auto const factor = (double)curve_length / previous_curve_length;
                                auto ts = lpe_powerstroke->offset_points.data();
                                for (auto &t : ts) {
                                    t.x() *= factor;
                                }
                                lpe_powerstroke->offset_points.param_setValue(ts);
                            }
                            lpe_powerstroke->getRepr()->setAttribute("is_visible", "true");
                            sp_lpe_item_update_patheffect(lpeitem, false, false);
                        }
                    }

                    if (!simplified) {
                        lpe_simplify->getRepr()->setAttribute("threshold", tol_str);
                    }
                }
            }
        }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
