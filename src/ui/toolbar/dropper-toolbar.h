// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_DROPPER_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_DROPPER_TOOLBAR_H

/**
 * @file
 * Dropper toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"

namespace Gtk {
class Builder;
class ToggleButton;
} // namespace Gtk

namespace Inkscape::UI::Toolbar {

/**
 * \brief A toolbar for controlling the dropper tool
 */
class DropperToolbar : public Toolbar
{
public:
    DropperToolbar();

private:
    DropperToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    // Control whether to pick opacity.
    Gtk::ToggleButton &_pick_alpha_btn;

    // Control whether to set opacity.
    Gtk::ToggleButton &_set_alpha_btn;

    // Event handlers
    void on_pick_alpha_button_toggled();
    void on_set_alpha_button_toggled();
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_DROPPER_TOOLBAR_H

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
