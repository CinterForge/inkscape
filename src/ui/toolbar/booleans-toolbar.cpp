// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A toolbar for the Builder tool.
 *
 * Authors:
 *   Martin Owens
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2022 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "booleans-toolbar.h"

#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>

#include "desktop.h"
#include "ui/builder-utils.h"
#include "ui/tools/booleans-tool.h"

namespace Inkscape::UI::Toolbar {

BooleansToolbar::BooleansToolbar()
    : BooleansToolbar{create_builder("toolbar-booleans.ui")}
{}

BooleansToolbar::BooleansToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "booleans-toolbar")}
{
    auto adj_opacity = get_object<Gtk::Adjustment>(builder, "opacity_adj");

    get_widget<Gtk::Button>(builder, "confirm_btn").signal_clicked().connect([this] {
        auto const tool = dynamic_cast<Tools::InteractiveBooleansTool *>(_desktop->getTool());
        tool->shape_commit();
    });

    get_widget<Gtk::Button>(builder, "cancel_btn").signal_clicked().connect([this] {
        auto const tool = dynamic_cast<Tools::InteractiveBooleansTool *>(_desktop->getTool());
        tool->shape_cancel();
    });

    adj_opacity->set_value(Preferences::get()->getDouble("/tools/booleans/opacity", 0.5) * 100);
    adj_opacity->signal_value_changed().connect([this, adj_opacity] {
        auto const tool = dynamic_cast<Tools::InteractiveBooleansTool *>(_desktop->getTool());
        double value = adj_opacity->get_value() / 100.0;
        Preferences::get()->setDouble("/tools/booleans/opacity", value);
        tool->set_opacity(value);
    });

    _initMenuBtns();
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
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
