// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Notebook page widget.
 *
 * Author:
 *   Bryce Harrington <bryce@bryceharrington.org>
 *
 * Copyright (C) 2004 Bryce Harrington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "notebook-page.h"

#include <gtkmm/grid.h>
#include <gtkmm/scrolledwindow.h>

#include "ui/pack.h"

namespace Inkscape {
namespace UI {
namespace Widget {

NotebookPage::NotebookPage(int n_rows, int n_columns, bool add_scroll_wnd)
    : Gtk::Box(Gtk::Orientation::VERTICAL)
    , _table(Gtk::make_managed<Gtk::Grid>())
{
    _table->set_name("NotebookPage");

    _table->set_row_spacing(4);
    _table->set_column_spacing(4);
    _table->set_margin(4);
    Gtk::Widget* child = _table;

    // if requested, add scrolling to individual pages
    if (add_scroll_wnd) {
        auto wnd = Gtk::make_managed<Gtk::ScrolledWindow>();
        wnd->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
        wnd->set_has_frame(false);
        wnd->set_expand();
        wnd->set_valign(Gtk::Align::FILL);
        wnd->set_halign(Gtk::Align::FILL);
        wnd->set_child(*_table);
        child = wnd;
    }

    UI::pack_start(*this, *child, true, true, 0);
}

} // namespace Widget
} // namespace UI
} // namespace Inkscape

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
