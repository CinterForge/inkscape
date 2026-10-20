// SPDX-License-Identifier: GPL-2.0-or-later

#include "custom-tooltip.h"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <gtkmm/tooltip.h>
#include <giomm/themedicon.h>

#include "ui/pack.h"

bool
sp_query_custom_tooltip(Gtk::Widget *widg, const Glib::RefPtr<Gtk::Tooltip>& tooltipw,
                        Glib::ustring tooltip, Glib::ustring icon, Gtk::IconSize iconsize)
{
    auto const box = Gtk::make_managed<Gtk::Box>();
    auto const label = Gtk::make_managed<Gtk::Label>();
    label->set_wrap(true);
    label->set_markup(tooltip);
    label->set_max_width_chars(40);
    if (icon != "") {
	    Inkscape::UI::pack_start(*box, *Gtk::make_managed<Gtk::Image>(Gio::ThemedIcon::create(icon)), true, true, 2);
    }
    Inkscape::UI::pack_start(*box, *label, true, true, 2);
    tooltipw->set_custom(*box);
    box->add_css_class("symbolic");
    return true;
}

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
