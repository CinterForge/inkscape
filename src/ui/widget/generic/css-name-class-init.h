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

#ifndef SEEN_UI_WIDGET_CSS_NAME_CLASS_INIT_H
#define SEEN_UI_WIDGET_CSS_NAME_CLASS_INIT_H

#include <glibmm/extraclassinit.h>

namespace Inkscape::UI::Widget {

/// A class that can be inherited to set the CSS name of a Gtk::Widget subclass.
/// The subclass must also inherit from Gtk::Widget or a subclass thereof.
class CssNameClassInit : public Glib::ExtraClassInit {
protected:
    [[nodiscard]] explicit CssNameClassInit(Glib::ustring const &css_name);

private:
    Glib::ustring _css_name;
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_UI_WIDGET_CSS_NAME_CLASS_INIT_H

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
