// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Event handler for dialog windows.
 */
/* Authors:
 *   bulia byak <bulia@dr.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2003-2014 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtkmm/entry.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/version.h>
#include <gtkmm/window.h>

#include "widget/spinbutton.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#include "desktop.h"
#include "enums.h"
#include "inkscape.h"
#include "preferences.h"
#include "ui/util.h"
#include "widget/generic/spin-button.h"

/**
 * Remove focus from window to whoever it is transient for.
 */
void sp_dialog_defocus(Gtk::Window *win)
{
    // find out the document window we're transient for
    if (auto w = win->get_transient_for()) {
        // switch to it
        w->present();
    }
}

void sp_dialog_defocus_on_enter(Gtk::Entry *e)
{
    e->signal_activate().connect([e] {
        sp_dialog_defocus(dynamic_cast<Gtk::Window*>(e->get_root()));
    });
}

void sp_dialog_defocus_on_enter(Gtk::SpinButton &s)
{
#if GTKMM_CHECK_VERSION(4, 14, 0)
    s.signal_activate().connect([&] {
        sp_dialog_defocus(dynamic_cast<Gtk::Window*>(s.get_root()));
    });
#endif
}

void sp_dialog_defocus_on_enter(Inkscape::UI::Widget::SpinButton& s) {
    s.signal_activate().connect([&] {
        sp_dialog_defocus(dynamic_cast<Gtk::Window*>(s.get_root()));
    });
}

/**
 * Make the argument dialog transient to the currently active document window.
 */
void sp_transientize(Gtk::Window &window)
{
    auto prefs = Inkscape::Preferences::get();

#ifdef GDK_WINDOWING_X11
    if (prefs->getBool("/options/dialogsskiptaskbar/value")) {
        // This is an X11-specific feature (wayland and other platforms don't support it).
        // See https://discourse.gnome.org/t/how-to-hide-app-from-taskbar-in-gtk4/7084
        if (is_x11_display()) {
            auto native = GTK_NATIVE(window.gobj());
            auto surface = gtk_native_get_surface(native);
            if (surface) {
                gdk_x11_surface_set_skip_taskbar_hint(surface, TRUE);
            }
        }
    }
#endif

    int transient_policy = prefs->getIntLimited("/options/transientpolicy/value", PREFS_DIALOGS_WINDOWS_NORMAL,
                                                PREFS_DIALOGS_WINDOWS_NONE, PREFS_DIALOGS_WINDOWS_AGGRESSIVE);

#ifdef _WIN32 // Win32 special code to enable transient dialogs
    transient_policy = PREFS_DIALOGS_WINDOWS_AGGRESSIVE;
#endif

    if (transient_policy) {
        // if there's an active document window, attach dialog to it as a transient:
        if (SP_ACTIVE_DESKTOP) {
            SP_ACTIVE_DESKTOP->setWindowTransient(window, transient_policy);
        }
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
