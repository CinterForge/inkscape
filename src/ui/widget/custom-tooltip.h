// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_CUSTOM_TOOLTIP_H
#define INKSCAPE_UI_WIDGET_CUSTOM_TOOLTIP_H

#include <gtkmm/enums.h>

namespace Gtk {
class Tooltip;
class Widget;
} // namespace Gtk

bool
sp_query_custom_tooltip(
    Gtk::Widget *widg,
    const Glib::RefPtr<Gtk::Tooltip>& tooltipw, 
    Glib::ustring tooltip, 
    Glib::ustring icon = "", 
    Gtk::IconSize iconsize = Gtk::IconSize::LARGE);

#endif // INKSCAPE_UI_WIDGET_CUSTOM_TOOLTIP_H

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
