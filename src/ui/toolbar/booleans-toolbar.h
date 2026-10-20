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

#ifndef INKSCAPE_UI_TOOLBAR_BOOLEANS_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_BOOLEANS_TOOLBAR_H

#include "toolbar.h"

namespace Gtk { class Builder; }

namespace Inkscape::UI::Toolbar {

class BooleansToolbar : public Toolbar
{
public:
    BooleansToolbar();

private:
    BooleansToolbar(Glib::RefPtr<Gtk::Builder> const &builder);
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_BOOLEANS_TOOLBAR_H

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
