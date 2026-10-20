// SPDX-License-Identifier: GPL-2.0-or-later
/** @file  measure-tool-settings.h
 * @brief  Used to show extra settings for the Measure tool
 *
 * Authors:
 *   Giambattista Caltabiano
 *
 * Copyright (C) 2025 Giambattista Caltabiano
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_DIALOG_MEASURE_TOOL_SETTINGS_H
#define INKSCAPE_DIALOG_MEASURE_TOOL_SETTINGS_H

#include "ui/dialog/dialog-base.h"

namespace Gtk {
class Builder;
class Box;
class CheckButton;
} // namespace Gtk

namespace Inkscape::UI::Widget {
class SpinButton;
} // namespace Inkscape::UI::Widget

namespace Inkscape::UI::Dialog {

class MeasureToolSettingsDialog final : public DialogBase
{
public:
    MeasureToolSettingsDialog(const char* _prefPath = "/dialogs/measureToolSettings");
    ~MeasureToolSettingsDialog() override;

protected:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Box &_main;
    Gtk::CheckButton &_show_angle;
    Gtk::CheckButton &_show_deltas;
    Gtk::CheckButton &_show_deltas_label;
    Gtk::CheckButton &_show_segments_label;
    UI::Widget::SpinButton &_segments_min_length;
    Gtk::CheckButton &_labels_btn;
    Gtk::CheckButton &_units_btn;
    Gtk::CheckButton &_tabs_btn;
    Gtk::CheckButton &_length_btn;
    Gtk::CheckButton &_between_btn;
    Gtk::CheckButton &_angle_btn;
    Gtk::CheckButton &_dX_btn;
    Gtk::CheckButton &_dY_btn;
    Gtk::CheckButton &_segments_btn;
    Gtk::CheckButton &_shape_width_btn;
    Gtk::CheckButton &_shape_height_btn;
    Gtk::CheckButton &_shape_X_btn;
    Gtk::CheckButton &_shape_Y_btn;
    Gtk::CheckButton &_shape_length_btn; 

    void _show_angle_change();
    void _show_deltas_change();
    void _show_deltas_label_change();
    void _show_segments_label_change();
    void _segments_min_length_change();
    void labels_btn_change();
    void units_btn_change();
    void tabs_btn_change();
    void length_btn_change();
    void between_btn_change();
    void angle_btn_change();
    void dX_btn_change();
    void dY_btn_change();
    void segments_btn_change();
    void shape_width_btn_change();
    void shape_height_btn_change();
    void shape_X_btn_change();
    void shape_Y_btn_change();
    void shape_length_btn_change();

};

} // namespace Inkscape::UI::Dialog

#endif // INKSCAPE_DIALOG_MEASURE_TOOL_SETTINGS_H

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
