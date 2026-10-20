// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2020 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "scroll-utils.h"

#include <gtkmm/scrolledwindow.h>
#include "ui/util.h"

namespace Inkscape::UI::Widget {

Gtk::Widget *get_scrollable_ancestor(Gtk::Widget &widget)
{
    for (auto &parent : parent_chain(widget) | std::views::drop(1)) {
        if (dynamic_cast<Gtk::ScrolledWindow *>(&parent)) {
            return &parent;
        }
    }

    return nullptr;
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
