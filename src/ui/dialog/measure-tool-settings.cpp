// SPDX-License-Identifier: GPL-2.0-or-later
/** @file  measure-tool-settings.cpp
 * @brief  Used to show extra settings for the Measure tool
 *
 * Authors:
 *   Giambattista Caltabiano
 *
 * Copyright (C) 2025 Giambattista Caltabiano
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "measure-tool-settings.h"
#include "ui/tools/measure-tool.h"

#include <gtkmm/checkbutton.h>
#include <gtkmm/adjustment.h>

#include "desktop.h"
#include "ui/widget/spinbutton.h"
#include "ui/builder-utils.h"

namespace Inkscape::UI::Dialog {

MeasureToolSettingsDialog::MeasureToolSettingsDialog(const char* _prefPath)
    : DialogBase(_prefPath, "MeasureToolSettings"),
    builder(create_builder("dialog-measure-tool-settings.ui")),
    _main{get_widget<Gtk::Box>(builder, "main")},
    _show_angle{get_widget<Gtk::CheckButton>(builder, "show_angle_btn")},
    _show_deltas{get_widget<Gtk::CheckButton>(builder, "show_deltas_btn")},
    _show_deltas_label{get_widget<Gtk::CheckButton>(builder, "deltas_label_btn")},
    _show_segments_label{get_widget<Gtk::CheckButton>(builder, "segments_label_btn")},
    _segments_min_length{get_derived_widget<UI::Widget::SpinButton>(builder, "seg_min_length")},
    _labels_btn{get_widget<Gtk::CheckButton>(builder, "labels")},
    _units_btn{get_widget<Gtk::CheckButton>(builder, "units")},
    _tabs_btn{get_widget<Gtk::CheckButton>(builder, "tabs")},
    _length_btn{get_widget<Gtk::CheckButton>(builder, "length")},
    _between_btn{get_widget<Gtk::CheckButton>(builder, "between")},
    _angle_btn{get_widget<Gtk::CheckButton>(builder, "angle")},
    _dX_btn{get_widget<Gtk::CheckButton>(builder, "dX")},
    _dY_btn{get_widget<Gtk::CheckButton>(builder, "dY")},
    _segments_btn{get_widget<Gtk::CheckButton>(builder, "segments")},
    _shape_width_btn{get_widget<Gtk::CheckButton>(builder, "shape_width")},
    _shape_height_btn{get_widget<Gtk::CheckButton>(builder, "shape_height")},
    _shape_X_btn{get_widget<Gtk::CheckButton>(builder, "shape_X")},
    _shape_Y_btn{get_widget<Gtk::CheckButton>(builder, "shape_Y")},
    _shape_length_btn{get_widget<Gtk::CheckButton>(builder, "shape_length")}
{
    auto prefs = Preferences::get();
    Preferences::get()->setString("/tools/measure/MTSpath", _prefPath); // will use it to retrieve the settings
    
    _show_angle.set_active(prefs->getBool(_prefs_path + "/show_angle", false));
    _show_angle.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::_show_angle_change));
    _show_deltas.set_active(prefs->getBool(_prefs_path + "/show_deltas", false));
    _show_deltas.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::_show_deltas_change));
    _show_deltas_label.set_active(prefs->getBool(_prefs_path + "/show_deltas_label", false));
    _show_deltas_label.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::_show_deltas_label_change));
    _show_segments_label.set_active(prefs->getBool(_prefs_path + "/show_segments_label", false));
    _show_segments_label.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::_show_segments_label_change));
    _segments_min_length.set_value(prefs->getDouble(_prefs_path + "/segments_min_length", 0.1));
    _segments_min_length.signal_value_changed().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::_segments_min_length_change));
    _segments_min_length.set_custom_numeric_menu_data({});
    _labels_btn.set_active(prefs->getBool(_prefs_path + "/labels", true));
    _labels_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::labels_btn_change));
    _units_btn.set_active(prefs->getBool(_prefs_path + "/units", true));
    _units_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::units_btn_change));
    _tabs_btn.set_active(prefs->getBool(_prefs_path + "/tabs", true));
    _tabs_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::tabs_btn_change));
    _length_btn.set_active(prefs->getBool(_prefs_path + "/length", true));
    _length_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::length_btn_change));
    _between_btn.set_active(prefs->getBool(_prefs_path + "/between", true));
    _between_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::between_btn_change));
    _angle_btn.set_active(prefs->getBool(_prefs_path + "/angle", true));
    _angle_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::angle_btn_change));
    _dX_btn.set_active(prefs->getBool(_prefs_path + "/dX", true));
    _dX_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::dX_btn_change));
    _dY_btn.set_active(prefs->getBool(_prefs_path + "/dY", true));
    _dY_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::dY_btn_change));
    _segments_btn.set_active(prefs->getBool(_prefs_path + "/segments", true));
    _segments_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::segments_btn_change));
    _shape_width_btn.set_active(prefs->getBool(_prefs_path + "/shape_width", true));
    _shape_width_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::shape_width_btn_change));
    _shape_height_btn.set_active(prefs->getBool(_prefs_path + "/shape_height", true));
    _shape_height_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::shape_height_btn_change));
    _shape_X_btn.set_active(prefs->getBool(_prefs_path + "/shape_X", true));
    _shape_X_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::shape_X_btn_change));
    _shape_Y_btn.set_active(prefs->getBool(_prefs_path + "/shape_Y", true));
    _shape_Y_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::shape_Y_btn_change));
    _shape_length_btn.set_active(prefs->getBool(_prefs_path + "/shape_length", true));
    _shape_length_btn.signal_toggled().connect(sigc::mem_fun(*this, &MeasureToolSettingsDialog::shape_length_btn_change));

    get_widget<Gtk::Button>(builder, "copy-to-clipboard").signal_clicked().connect([this]() {
        if (auto desktop = getDesktop()) {
            if (auto tool = dynamic_cast<Tools::MeasureTool*>(desktop->getTool())) {
                tool->copyToClipboard();
            }
        }
    });

    append(_main);
}
MeasureToolSettingsDialog::~MeasureToolSettingsDialog() {}

void MeasureToolSettingsDialog::_show_angle_change()
{
    bool active = _show_angle.get_active();
    Preferences::get()->setBool(_prefs_path + "/show_angle", active);
}
void MeasureToolSettingsDialog::_show_deltas_change()
{
    bool active = _show_deltas.get_active();
    Preferences::get()->setBool(_prefs_path + "/show_deltas", active);
}
void MeasureToolSettingsDialog::_show_deltas_label_change()
{
    bool active = _show_deltas_label.get_active();
    Preferences::get()->setBool(_prefs_path + "/show_deltas_label", active);
}
void MeasureToolSettingsDialog::_show_segments_label_change()
{
    bool active = _show_segments_label.get_active();
    Preferences::get()->setBool(_prefs_path + "/show_segments_label", active);
}
void MeasureToolSettingsDialog::_segments_min_length_change()
{
   double value = _segments_min_length.get_adjustment()->get_value();
    Preferences::get()->setDouble(_prefs_path + "/segments_min_length", value);
}
void MeasureToolSettingsDialog::labels_btn_change()
{
    bool active = _labels_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/labels", active);
}
void MeasureToolSettingsDialog::units_btn_change()
{
    bool active = _units_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/units", active);
}
void MeasureToolSettingsDialog::tabs_btn_change()
{
    bool active = _tabs_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/tabs", active);
}
void MeasureToolSettingsDialog::length_btn_change()
{
    bool active = _length_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/length", active);
}
void MeasureToolSettingsDialog::between_btn_change()
{
    bool active = _between_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/between", active);
}
void MeasureToolSettingsDialog::angle_btn_change()
{
    bool active = _angle_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/angle", active);
}
void MeasureToolSettingsDialog::dX_btn_change()
{
    bool active = _dX_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/dX", active);
}
void MeasureToolSettingsDialog::dY_btn_change()
{
    bool active = _dY_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/dY", active);
}
void MeasureToolSettingsDialog::segments_btn_change()
{
    bool active = _segments_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/segments", active);
}
void MeasureToolSettingsDialog::shape_width_btn_change()
{
    bool active = _shape_width_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/shape_width", active);
}
void MeasureToolSettingsDialog::shape_height_btn_change()
{
    bool active = _shape_height_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/shape_height", active);
}
void MeasureToolSettingsDialog::shape_X_btn_change()
{
    bool active = _shape_X_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/shape_X", active);
}
void MeasureToolSettingsDialog::shape_Y_btn_change()
{
    bool active = _shape_Y_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/shape_Y", active);
}
void MeasureToolSettingsDialog::shape_length_btn_change()
{
    bool active = _shape_length_btn.get_active();
    Preferences::get()->setBool(_prefs_path + "/shape_length", active);
}

} // namespace Inkscape::UI::Dialog

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
