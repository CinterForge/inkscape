// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A base class for all dialogs.
 *
 * Authors: see git history
 *   Tavmjong Bah
 *
 * Copyright (c) 2018 Tavmjong Bah, Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_DIALOG_BASE_H
#define INK_DIALOG_BASE_H

#include <gtkmm/box.h>

#include "inkscape-application.h"
#include "ui/defocus-target.h"

namespace Gtk {
class EventControllerKey;
class ScrolledWindow;
} // namespace Gtk

class SPDesktop;

namespace Inkscape::UI::Dialog {

/**
 * DialogBase is the base class for the dialog system.
 *
 * Each dialog has a reference to the application, in order to update its inner focus
 * (be it of the active desktop, document, selection, etc.) in the update() method.
 *
 * DialogBase derived classes' instances live in DialogNotebook classes and are managed by
 * DialogContainer classes. DialogContainer instances can have at most one type of dialog,
 * differentiated by the associated type.
 */
class DialogBase : public Gtk::Box, protected DefocusTarget
{
    using parent_type = Gtk::Box;

public:
    DialogBase(char const *prefs_path = nullptr, Glib::ustring dialog_type = {});
    DialogBase(DialogBase const &) = delete;
    DialogBase &operator=(DialogBase const &) = delete;
    ~DialogBase() override;

    /**
     * The update() method is essential to Gtk state management. DialogBase implementations get updated whenever
     * a new focus event happens if they are in a DialogWindow or if they are in the currently focused window.
     *
     * DO NOT use update to keep SPDesktop, SPDocument or Selection states, use the virtual functions below.
     */
    virtual void update() {}

    // Public for future use, say if the desktop is smartly set when docking dialogs.
    void setDesktop(SPDesktop *new_desktop);

    void on_map() override;

    Glib::ustring const &get_name    () const { return _name       ; }
    Glib::ustring const &getPrefsPath() const { return _prefs_path ; }
    Glib::ustring const &get_type    () const { return _dialog_type; }
    const Glib::ustring& get_icon() const { return _icon_name; }

    void blink();
    // find focusable widget to grab focus
    virtual void focus_dialog();
    // return focus back to canvas
    void defocus_dialog();
    bool getShowing() { return _showing; }
    // fix children scrolled windows to send outer scroll when his own reach limits
    void fix_inner_scroll(Gtk::ScrolledWindow &scrollwin);

    // Too many dialogs have unprotected calls to ask for this data
    SPDesktop *getDesktop() const { return desktop; }

protected:
    InkscapeApplication *getApp() const { return _app; }
    SPDocument *getDocument() const { return document; }
    Selection *getSelection() const { return selection; }
    friend class DialogNotebook;
    void setShowing(bool showing);
    Glib::ustring _name;             // Gtk widget name (must be set!)
    Glib::ustring const _prefs_path; // Stores characteristic path for loading/saving the dialog position.
    Glib::ustring const _dialog_type; // Type of dialog (we could just use _pref_path?).

private:
    void onDefocus() override;
    bool blink_off(); // timer callback
    bool on_key_pressed(Gtk::EventControllerKey const &controller,
                        unsigned keyval, unsigned keycode, Gdk::ModifierType state);
    // return if dialog is on visible tab
    bool _showing = true;
    void unsetDesktop();
    void desktopDestroyed(SPDesktop* old_desktop);
    void setDocument(SPDocument *new_document);
    /**
     * Called when the desktop has certainly changed. It may have changed to nullptr
     * when destructing the dialog, so the override should expect nullptr too.
     */
    virtual void desktopReplaced() {}
    virtual void documentReplaced() {}
    virtual void selectionChanged(Inkscape::Selection *selection) {};
    virtual void selectionModified(Inkscape::Selection *selection, guint flags) {};

    sigc::connection _desktop_destroyed;
    sigc::connection _doc_replaced;
    sigc::connection _select_changed;
    sigc::connection _select_modified;

    int _modified_flags = 0;
    bool _modified_while_hidden = false;
    bool _changed_while_hidden = false;

    InkscapeApplication *_app; // Used for state management
    SPDesktop  *desktop   = nullptr;
    SPDocument *document  = nullptr;
    Selection  *selection = nullptr;
    Glib::ustring _icon_name;
};

} // namespace Inkscape::UI::Dialog

#endif // INK_DIALOG_BASE_H

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
