// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Gradient editor widget for "Fill and Stroke" dialog
 *
 * Author:
 *   Michael Kowalski
 *
 * Copyright (C) 2020-2021 Michael Kowalski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gradient-editor.h"

#include <glibmm/i18n.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/expander.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/togglebutton.h>

#include "display/cairo-utils.h"
#include "color-picker-panel.h"
#include "document-undo.h"
#include "gradient-chemistry.h"
#include "gradient-selector.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-stop.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/util.h"
#include "ui/widget/color-notebook.h"
#include "ui/widget/generic/spin-button.h"
#include "ui/widget/generic/popover-menu.h"

namespace Inkscape::UI::Widget {

using namespace Inkscape::IO;
using Inkscape::UI::Widget::ColorNotebook;

const std::array<std::tuple<SPGradientSpread, const char*, const char*>, 3>& sp_get_spread_repeats() {
    static auto const repeats = std::to_array({std::tuple
        {SP_GRADIENT_SPREAD_PAD    , C_("Gradient repeat type", "None"),      "gradient-spread-pad"},
        {SP_GRADIENT_SPREAD_REPEAT , C_("Gradient repeat type", "Direct"),    "gradient-spread-repeat"},
        {SP_GRADIENT_SPREAD_REFLECT, C_("Gradient repeat type", "Reflected"), "gradient-spread-reflect"},
    });
    return repeats;
}

GradientEditor::GradientEditor(const char* prefs, Space::Type space, bool show_type_selector, bool show_colorwheel_expander):
    _prefs(prefs),
    _builder(Inkscape::UI::create_builder("gradient-edit.glade")),
    _selector(Gtk::make_managed<GradientSelector>()),
    _colors(new Colors::ColorSet()),
    _repeat_popover{std::make_unique<UI::Widget::PopoverMenu>(Gtk::PositionType::BOTTOM)},
    _offset_btn(get_widget<InkSpinButton>(_builder, "offsetSpin")),
    _turn_gradient(get_widget<Gtk::Button>(_builder, "turnBtn")),
    _angle_adj(get_object<Gtk::Adjustment>(_builder, "adjustmentAngle")),
    _angle_btn(get_widget<InkSpinButton>(_builder, "angle")),
    _main_box(get_widget<Gtk::Box>(_builder, "main-box")),
    _color_picker(ColorPickerPanel::create(space, get_plate_type_preference(prefs, ColorPickerPanel::None), _colors)),
    _linear_btn(get_widget<Gtk::ToggleButton>(_builder, "type-linear")),
    _radial_btn(get_widget<Gtk::ToggleButton>(_builder, "type-radial")),
    _repeat_mode_btn(get_widget<Gtk::MenuButton>(_builder, "repeat-mode")),
    _prev_stop_btn(get_widget<Gtk::Button>(_builder, "prevStopBtn")),
    _next_stop_btn(get_widget<Gtk::Button>(_builder, "nextStopBtn")),
    _add_stop_btn(get_widget<Gtk::Button>(_builder, "addStopBtn")),
    _insert_stop_btn(get_widget<Gtk::MenuButton>(_builder, "insertStopBtn")),
    _remove_stop_btn(get_widget<Gtk::Button>(_builder, "removeStopBtn"))
{
    // gradient type buttons
    _linear_btn.set_active();
    _linear_btn.signal_clicked().connect([this]{ fire_change_type(true); });
    _radial_btn.signal_clicked().connect([this]{ fire_change_type(false); });
    if (!show_type_selector) {
        _linear_btn.set_visible(false);
        _radial_btn.set_visible(false);
    }

    auto& reverse = get_widget<Gtk::Button>(_builder, "reverseBtn");
    reverse.signal_clicked().connect([this]{ reverse_gradient(); });

    _turn_gradient.signal_clicked().connect([this]{ turn_gradient(90, true); });
    _angle_adj->signal_value_changed().connect([this]{
        turn_gradient(_angle_adj->get_value(), false);
    });

    auto& gradBox = get_widget<Gtk::Box>(_builder, "gradientBox");
    // gradient stop selected in a gradient widget; sync list selection
    _gradient_image.signal_stop_selected().connect([this](size_t index) {
        select_stop(index);
    });
    _gradient_image.signal_stop_offset_changed().connect([this](size_t index, double offset) {
        set_stop_offset(index, offset);
    });
    _gradient_image.signal_add_stop_at().connect([this](double offset) {
        insert_stop_at(offset);
    });
    _gradient_image.signal_delete_stop().connect([this](size_t index) {
        delete_stop(index);
    });
    gradBox.append(_gradient_image);

    if (show_colorwheel_expander) {
        auto expander = Gtk::make_managed<Gtk::Expander>();
        expander->set_label(_("Color wheel"));
        expander->set_margin_top(8);
        expander->property_expanded().signal_changed().connect([this, expander]{
            _color_picker->set_plate_type(expander->get_expanded() ? ColorPickerPanel::Circle : ColorPickerPanel::None);
        });
        _main_box.append(*expander);
    }

    // add color selector
    _main_box.append(*_color_picker);

    // gradient library in a popup
    get_widget<Gtk::Popover>(_builder, "libraryPopover").set_child(*_selector);
    const int h = 5;
    const int v = 3;
    _selector->set_margin_start(h);
    _selector->set_margin_end(h);
    _selector->set_margin_top(v);
    _selector->set_margin_bottom(v);
    _selector->set_visible(true);
    _selector->show_edit_button(false);
    _selector->set_gradient_size(160, 20);
    _selector->set_name_col_size(120);
    // gradient changed is currently the only signal that GradientSelector can emit:
    _selector->signal_changed().connect([this](SPGradient* gradient) {
        // new gradient selected from the library
        _signal_changed.emit(gradient);
    });

    // connect gradient repeat modes menu
    for (auto [mode, text, icon] : sp_get_spread_repeats()) {
        auto const item = Gtk::make_managed<UI::Widget::PopoverMenuItem>(text, false, icon);
        item->signal_activate().connect([this, mode]{ set_repeat_mode(mode); });
        _repeat_popover->append(*item);
    }
    _repeat_mode_btn.set_popover(*_repeat_popover);
    set_repeat_icon(SP_GRADIENT_SPREAD_PAD);

    _colors->signal_changed.connect([this]() {
        set_stop_color(_colors->getAverage());
    });

    _offset_btn.signal_value_changed().connect([this](double offset) {
        if (auto index = current_stop_index()) {
            set_stop_offset(index.value(), offset / 100.0);
        }
    });

    auto group = Gio::SimpleActionGroup::create();
    _insert_stop_left = group->add_action("stop-insert-left",  sigc::mem_fun(*this, &GradientEditor::insertStopLeft));
    _insert_stop_right = group->add_action("stop-insert-right", sigc::mem_fun(*this, &GradientEditor::insertStopRight));
    insert_action_group("gradient-editor", std::move(group));

    _prev_stop_btn.signal_clicked().connect([this]{ select_stop(_current_stop_index - 1); });
    _next_stop_btn.signal_clicked().connect([this]{ select_stop(_current_stop_index + 1); });
    _add_stop_btn.signal_clicked().connect([this]{ add_stop(_current_stop_index); });
    _remove_stop_btn.signal_clicked().connect([this]{ delete_stop(_current_stop_index); });
    signal_stop_selected().connect([this](SPStop*){ updateButtonSensitivity(); });

    _color_picker->get_last_column_size()->add_widget(get_widget<Gtk::MenuButton>(_builder, "libraryBtn"));
    _color_picker->get_last_column_size()->add_widget(get_widget<Gtk::MenuButton>(_builder, "repeat-mode"));

    append(_main_box);
}

void GradientEditor::set_stop_color(Inkscape::Colors::Color const &color)
{
    if (_update.pending()) return;

    SPGradient* vector = get_gradient_vector();
    if (!vector) return;

    if (auto stop = current_stop()) {
        if (_document) {
            auto scoped(_update.block());
            sp_set_gradient_stop_color(_document, stop, color);
        }
    }
}

SPStop* GradientEditor::current_stop() {
    SPGradient* vector = _gradient ? _gradient->getVector() : nullptr;

    if (!vector || !vector->hasStops()) return nullptr;

    vector->ensureVector();
    int index = 0;
    for (auto& child : vector->children) {
        if (auto stop = cast<SPStop>(&child)) {
            if (index == _current_stop_index) return stop;

            ++index;
        }
    }

    return nullptr;
}

std::optional<int> GradientEditor::current_stop_index() {
    if (current_stop()) {
        return _current_stop_index;
    }
    return std::nullopt;
}

std::optional<int> GradientEditor::get_stop_index(SPStop* stop) {
    SPGradient* vector = _gradient ? _gradient->getVector() : nullptr;
    if (!vector || !stop) return std::nullopt;

    return sp_number_of_stops_before_stop(vector, stop);
}

SPStop* GradientEditor::get_nth_stop(size_t index) {
    if (SPGradient* vector = get_gradient_vector()) {
        return sp_get_nth_stop(vector, index);
    }
    return nullptr;
}

// stop has been selected in a list view
void GradientEditor::stop_selected() {
    auto scoped(_update.block());
    _colors->clear();

    if (auto stop = current_stop()) {
        _colors->set(stop->getId(), stop->getColor());

        auto [before, after] = sp_get_before_after_stops(stop);
        _offset_btn.set_range(before ? before->offset * 100 : 0, after ? after->offset * 100 : 100);
        _offset_btn.set_sensitive();
        _offset_btn.set_value(stop->offset * 100);

        _gradient_image.setSelectedStop(current_stop_index().value_or(-1));
    }
    else {
        // no selection
        _offset_btn.set_range(0, 0);
        _offset_btn.set_value(0);
        _offset_btn.set_sensitive(false);
    }
}

void GradientEditor::insert_stop_at(double offset) {
    if (SPGradient* vector = get_gradient_vector()) {
        // only insert a new stop if there are some stops present
        if (vector->hasStops()) {
            SPStop* stop = sp_gradient_add_stop_at(vector, offset);
            // select the next stop
            select_stop(sp_number_of_stops_before_stop(vector, stop));
        }
    }
}

// Returns -1 if inserting isn't valid
int GradientEditor::getLeftStopInsertIndex()
{
    if (auto vector = get_gradient_vector()) {
        if (gtk_get_locale_direction() == GTK_TEXT_DIR_RTL) {
            if (_current_stop_index < vector->getStopCount() - 1) {
                return _current_stop_index;
            }
        } else {
            if (_current_stop_index > 0) {
                return _current_stop_index - 1;
            }
        }
    }

    return -1;
}


// Returns -1 if inserting isn't valid
int GradientEditor::getRightStopInsertIndex()
{
    if (auto vector = get_gradient_vector()) {
        if (gtk_get_locale_direction() == GTK_TEXT_DIR_RTL) {
            if (_current_stop_index > 0) {
                return _current_stop_index - 1;
            }
        } else {
            if (_current_stop_index < vector->getStopCount() - 1) {
                return _current_stop_index;
            }
        }
    }

    return -1;
}

void GradientEditor::insertStopLeft() {
    auto where = getLeftStopInsertIndex();
    if (where >= 0) {
        add_stop(where);
    }
}

void GradientEditor::insertStopRight() {
    auto where = getRightStopInsertIndex();
    if (where >= 0) {
        add_stop(where);
    }
}

void GradientEditor::add_stop(int index) {
    if (SPGradient* vector = get_gradient_vector()) {
        if (SPStop* current = sp_get_nth_stop(vector, index)) {
            SPStop* stop = sp_gradient_add_stop(vector, current);
            // select the next stop
            select_stop(sp_number_of_stops_before_stop(vector, stop));
        }
    }
}

void GradientEditor::delete_stop(int index) {
    if (SPGradient* vector = get_gradient_vector()) {
        if (SPStop* stop = sp_get_nth_stop(vector, index)) {
            // try deleting a stop if it can be
            if (sp_gradient_delete_stop(vector, stop)) {
                // Select last stop if our selection "fell off the end" of the stop list
                if (_current_stop_index >= vector->getStopCount()) {
                    select_stop(vector->getStopCount() - 1);
                }
            }
        }
    }
}

double line_angle(const Geom::Line& line) {
    auto d = line.finalPoint() - line.initialPoint();
    return std::atan2(d.y(), d.x());
}

// turn linear gradient 90 degrees
void GradientEditor::turn_gradient(double angle, bool relative) {
    if (_update.pending() || !_document || !_gradient) return;

    if (auto linear = cast<SPLinearGradient>(_gradient)) {
        auto scoped(_update.block());

        auto line = Geom::Line(
            Geom::Point(linear->x1.computed, linear->y1.computed),
            Geom::Point(linear->x2.computed, linear->y2.computed)
        );
        auto center = line.pointAt(0.5);
        auto radians = angle / 180 * M_PI;
        if (!relative) {
            radians -= line_angle(line);
        }
        auto rotate = Geom::Translate(-center) * Geom::Rotate(radians) * Geom::Translate(center);
        auto rotated = line.transformed(rotate);

        linear->x1 = rotated.initialPoint().x();
        linear->y1 = rotated.initialPoint().y();
        linear->x2 = rotated.finalPoint().x();
        linear->y2 = rotated.finalPoint().y();

        _gradient->updateRepr();

        DocumentUndo::done(_document, RC_("Undo", "Rotate gradient"), INKSCAPE_ICON("color-gradient"));
    }
}

void GradientEditor::reverse_gradient() {
    if (_document && _gradient) {
        // reverse works on a gradient definition, the one with stops:
        if (SPGradient* vector = get_gradient_vector()) {
            sp_gradient_reverse_vector(vector);
            DocumentUndo::done(_document, RC_("Undo", "Reverse gradient"), INKSCAPE_ICON("color-gradient"));
        }
    }
}

void GradientEditor::set_repeat_mode(SPGradientSpread mode) {
    if (_update.pending()) return;

    if (_document && _gradient) {
        auto scoped(_update.block());

        // spread is set on a gradient reference, which is _gradient object
        _gradient->setSpread(mode);
        _gradient->updateRepr();

        DocumentUndo::done(_document, RC_("Undo", "Set gradient repeat"), INKSCAPE_ICON("color-gradient"));

        set_repeat_icon(mode);
    }
}

void GradientEditor::set_repeat_icon(SPGradientSpread mode) {
    for (auto [repeat_mode, _label, icon_name] : sp_get_spread_repeats()) {
        if (mode == repeat_mode) {
             _repeat_mode_btn.set_icon_name(icon_name);
             break;
        }
    }
}

void GradientEditor::setGradient(SPGradient* gradient) {
    auto scoped(_update.block());
    auto scoped2(_notification.block());
    _gradient = gradient;
    _document = gradient ? gradient->document : nullptr;
    set_gradient(gradient);
}

SPGradient* GradientEditor::getVector() {
    return _selector->getVector();
}

void GradientEditor::setVector(SPDocument* doc, SPGradient* vector) {
    auto scoped(_update.block());
    _selector->setVector(doc, vector);
}

void GradientEditor::setMode(SelectorMode mode) {
    _selector->setMode(mode);
}

void GradientEditor::setUnits(SPGradientUnits units) {
    _selector->setUnits(units);
}

SPGradientUnits GradientEditor::getUnits() {
    return _selector->getUnits();
}

void GradientEditor::setSpread(SPGradientSpread spread) {
    _selector->setSpread(spread);
}

SPGradientSpread GradientEditor::getSpread() {
    return _selector->getSpread();
}

void GradientEditor::selectStop(SPStop* selected) {
    if (_notification.pending()) return;

    auto scoped(_notification.block());
    select_stop(get_stop_index(selected).value_or(-1));
}

void GradientEditor::set_color_picker_plate(ColorPickerPanel::PlateType type) {
    _color_picker->set_plate_type(type);
    set_plate_type_preference(_prefs.c_str(), type);
}

ColorPickerPanel::PlateType GradientEditor::get_color_picker_plate() const {
    return _color_picker->get_plate_type();
}

SPGradient* GradientEditor::get_gradient_vector() {
    if (!_gradient) return nullptr;
    return sp_gradient_get_forked_vector_if_necessary(_gradient, false);
}

SPGradientType GradientEditor::get_type() const {
    return _linear_btn.get_active() ? SP_GRADIENT_TYPE_LINEAR : SP_GRADIENT_TYPE_RADIAL;
}

void GradientEditor::updateButtonSensitivity()
{
    auto vector = get_gradient_vector();
    _prev_stop_btn.set_sensitive(vector && _current_stop_index > 0);
    _next_stop_btn.set_sensitive(vector && _current_stop_index < vector->getStopCount() - 1);
    _insert_stop_left->set_enabled(getLeftStopInsertIndex() >= 0);
    _insert_stop_right->set_enabled(getRightStopInsertIndex() >= 0);
    _remove_stop_btn.set_sensitive(vector && vector->getStopCount() > 2);
}

void GradientEditor::set_gradient(SPGradient* gradient) {
    auto scoped(_update.block());

    SPGradient* vector = gradient ? gradient->getVector() : nullptr;

    if (vector) {
        vector->ensureVector();
    }

    _gradient_image.set_gradient(vector);
    _vector_modified_connection.disconnect();

    if (!vector || !vector->hasStops()) return;

    auto mode = gradient->isSpreadSet() ? gradient->getSpread() : SP_GRADIENT_SPREAD_PAD;
    set_repeat_icon(mode);

    // Watch for changes to the number of stops, so we can set button sensitivity as needed.
    // This is a little noisy. Is there a better way to watch for gradient stop removals/additions?
    _vector_modified_connection = vector->connectModified([this](SPObject *obj, guint flags) {
        if (flags & SP_OBJECT_MODIFIED_FLAG) {
            updateButtonSensitivity();
        }
    });
    updateButtonSensitivity();

    auto can_rotate = false;
    // only linear gradient can be rotated currently
    if (auto linear = cast<SPLinearGradient>(gradient)) {
        can_rotate = true;
        auto line = Geom::Line(
            Geom::Point(linear->x1.computed, linear->y1.computed),
            Geom::Point(linear->x2.computed, linear->y2.computed)
        );
        auto angle = line_angle(line) * 180 / M_PI;
        _angle_adj->set_value(angle);

        _linear_btn.set_active();
    }
    else {
        _radial_btn.set_active();
    }
    _turn_gradient.set_sensitive(can_rotate);
    _angle_btn.set_sensitive(can_rotate);

    select_stop(_current_stop_index);
}

void GradientEditor::set_stop_offset(size_t index, double offset) {
    if (_update.pending()) return;

    // adjust stop's offset after the user edits it in offset spin button or drags stop handle
    if (auto stop = get_nth_stop(index)) {
        auto scoped(_update.block());

        stop->offset = offset;
        if (auto repr = stop->getRepr()) {
            repr->setAttributeCssDouble("offset", stop->offset);
        }

        DocumentUndo::maybeDone(stop->document, "gradient:stop:offset", RC_("Undo", "Change gradient stop offset"), INKSCAPE_ICON("color-gradient"));
    }
}

// select the requested stop
bool GradientEditor::select_stop(int index) {
    if (get_nth_stop(index)) {
        _current_stop_index = index;
        // update related widgets
        stop_selected();
        fire_stop_selected(current_stop());
        return true;
    }
     return false;
}

void GradientEditor::fire_stop_selected(SPStop* stop) {
    if (!_notification.pending()) {
        auto scoped(_notification.block());
        emit_stop_selected(stop);
    }
}

void GradientEditor::fire_change_type(bool linear) {
    if (_notification.pending()) return;

    auto scoped(_notification.block());
    _signal_changed.emit(_gradient);
}

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
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
