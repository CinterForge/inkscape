// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Page properties widget
 */
/*
 * Authors:
 *   Michael Kowalski
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H
#define INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H

#include <gtkmm/box.h>

namespace Inkscape {

namespace Colors { class Color; }
namespace Util { class Unit; }

namespace UI::Widget {

class PageProperties : public Gtk::Box {
public:
    static PageProperties* create();

    enum class Color { Background, Desk, Border };
    virtual void set_color(Color element, Colors::Color const &) = 0;

    enum class Check { Checkerboard, Border, Shadow, BorderOnTop, AntiAlias, NonuniformScale,
                       DisabledScale, UnsupportedSize, ClipToPage, PageLabelStyle, YAxisPointsDown, OriginCurrentPage };
    virtual void set_check(Check element, bool checked) = 0;

    enum class Dimension { PageSize, ViewboxSize, ViewboxPosition, Scale, ScaleContent, PageTemplate };
    virtual void set_dimension(Dimension dim, double x, double y) = 0;

    enum class Units { Display, Document };
    virtual void set_unit(Units unit, const Glib::ustring& abbr) = 0;

    auto signal_color_changed    () { return _signal_color_changed    ; }
    auto signal_check_toggled    () { return _signal_check_toggled    ; }
    auto signal_dimension_changed() { return _signal_dimension_changed; }
    auto signal_unit_changed     () { return _signal_unit_changed     ; }
    auto signal_resize_to_fit    () { return _signal_resize_to_fit    ; }
    auto signal_origin_changed   () { return _signal_origin_changed   ; }

protected:
    sigc::signal<void (Colors::Color const &, Color)> _signal_color_changed;
    sigc::signal<void (bool, Check)> _signal_check_toggled;
    sigc::signal<void (double, double, const Util::Unit*, Dimension)> _signal_dimension_changed;
    sigc::signal<void (const Util::Unit*, Units)> _signal_unit_changed;
    sigc::signal<void ()> _signal_resize_to_fit;
    sigc::signal<void (bool)> _signal_origin_changed;
};

} // namespace UI::Widget

} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_PAGE_PROPERTIES_H

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
