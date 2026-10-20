// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Carl Hetherington <inkscape@carlh.net>
 *   Derek P. Moore <derekm@hackunix.org>
 *
 * Copyright (C) 2004 Carl Hetherington
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_LABELLED_H
#define INKSCAPE_UI_WIDGET_LABELLED_H

#include <gtkmm/box.h>

namespace Glib {
class ustring;
} // namespace Glib

namespace Gtk {
class Label;
} // namespace Gtk

namespace Inkscape::UI::Widget {

/**
 * Adds a label with optional icon to another widget.
 */
class Labelled : public Gtk::Box
{
public:
    /** Construct a Labelled Widget.
     * @param label     Label text.
     * @param tooltip   Tooltip markup to set on this Box.
     * @param widget    Widget to label; should be allocated with new, as it will
     *                  be passed to Gtk::manage().
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the text
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to true).
     */
    Labelled(Glib::ustring const &label, Glib::ustring const &tooltip,
             Gtk::Widget *widget,
             Glib::ustring const &icon = {},
             bool mnemonic = true);

    inline Gtk::Widget const *getWidget() const { return _widget; }
    inline Gtk::Widget       *getWidget()       { return _widget; }

    inline Gtk::Label  const *getLabel () const { return _label ; }
    inline Gtk::Label        *getLabel ()       { return _label ; }

private:
    Gtk::Widget *_widget = nullptr;
    Gtk::Label  *_label  = nullptr;

    bool on_mnemonic_activate(bool group_cycling) final;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_LABELLED_H

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
