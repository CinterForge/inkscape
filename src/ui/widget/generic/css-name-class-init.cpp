// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A class that can be inherited to set the CSS name of a Gtk::Widget subclass.
 */
/*
 * Authors:
 *   Daniel Boles <dboles.src+inkscape@gmail.com>
 *
 * Copyright (C) 2023 Daniel Boles
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtk/gtk.h>
#include "css-name-class-init.h"

namespace Inkscape::UI::Widget {

namespace {

extern "C"
{
static void class_init_function(void * const g_class, void * const class_data)
{
    g_return_if_fail(GTK_IS_WIDGET_CLASS(g_class));

    auto const klass = static_cast<GtkWidgetClass *>(g_class);
    auto const css_name = static_cast<Glib::ustring const *>(class_data);
    gtk_widget_class_set_css_name(klass, css_name->c_str());
}

static void instance_init_function(GTypeInstance * const instance, void * /* g_class */)
{
    g_return_if_fail(GTK_IS_WIDGET(instance));
}
} // extern "C"

} // anonymous namespace

CssNameClassInit::CssNameClassInit(const Glib::ustring &css_name)
    : Glib::ExtraClassInit{class_init_function, &_css_name, instance_init_function}
    , _css_name{std::move(css_name)}
{
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
