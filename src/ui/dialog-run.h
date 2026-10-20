// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_DIALOG_RUN_H
#define INKSCAPE_UI_DIALOG_RUN_H

#include <memory>

namespace Gtk {
class Dialog;
class Root;
class Window;
} // namespace Gtk

namespace Inkscape::UI {

/**
 * Show a window modally, returning when it is closed.
 *
 * Todo: Attempt to port code that uses this function to the asynchronous API.
 */
void show_modal_synchronous(Gtk::Window &window);

/**
 * This is a GTK4 porting aid meant to replace the removal of the Gtk::Dialog synchronous API.
 *
 * It is intended as a temporary measure, although experience suggests it will be anything but.
 *
 * Todo: Attempt to port Gtk::Dialog to Gtk::Window and use show_modal_synchronous().
 */
int dialog_run(Gtk::Dialog &dialog);

/**
 * Show @p dialog modally, destroying it when the user dismisses it.
 * If @p root is not null, the dialog is shown as a transient for @p root.
 */
void dialog_show_modal_and_selfdestruct(std::unique_ptr<Gtk::Dialog> dialog, Gtk::Root *root = nullptr);

} // namespace Inkscape::UI

#endif // INKSCAPE_UI_DIALOG_RUN_H
