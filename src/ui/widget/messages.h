// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Messages dialog
 *
 * A very simple dialog for displaying Inkscape messages. Messages
 * sent to g_log(), g_warning(), g_message(), ets, are routed here,
 * in order to avoid messing with the startup console.
 */
/* Authors:
 *   Bob Jamison
 *   Other dudes from The Inkscape Organization
 *
 * Copyright (C) 2004, 2005 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_MESSAGES_H
#define INKSCAPE_UI_WIDGET_MESSAGES_H

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>

namespace Inkscape::UI::Widget {

class Messages final : public Gtk::Box
{
public:
    Messages();

    /**
     * Clear all information from the dialog
     */
    void clear();

    /**
     * Display a message
     */
    void message(char const *msg);

    /**
     * Redirect g_log() messages to this widget
     */
    void captureLogMessages();

    /**
     * Return g_log() messages to normal handling
     */
    void releaseLogMessages();

    void toggleCapture();

private:
    Gtk::ScrolledWindow textScroll;
    Gtk::TextView messageText;
    Gtk::Box buttonBox;
    Gtk::Button buttonClear;
    Gtk::CheckButton checkCapture;

    // Handler IDs
    unsigned handlerDefault = 0;
    unsigned handlerGlibmm = 0;
    unsigned handlerAtkmm = 0;
    unsigned handlerPangomm = 0;
    unsigned handlerGdkmm = 0;
    unsigned handlerGtkmm = 0;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_MESSAGES

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
