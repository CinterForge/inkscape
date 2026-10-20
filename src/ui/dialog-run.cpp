// SPDX-License-Identifier: GPL-2.0-or-later

#include <glibmm/main.h>
#include <gtkmm/dialog.h>

#include "dialog-run.h"

namespace Inkscape::UI {

void show_modal_synchronous(Gtk::Window &window)
{
    bool hidden = false;

    sigc::scoped_connection hide_conn = window.signal_hide().connect([&] {
        hidden = true;
    });

    window.set_hide_on_close();
    window.set_modal();
    window.present();

    auto main_context = Glib::MainContext::get_default();
    while (!hidden) {
        main_context->iteration(true);
    }
}

int dialog_run(Gtk::Dialog &dialog)
{
    int result = Gtk::ResponseType::NONE;

    sigc::scoped_connection response_conn = dialog.signal_response().connect([&] (int response) {
        result = response;
        dialog.set_visible(false);
    });

    show_modal_synchronous(dialog);

    return result;
}

void dialog_show_modal_and_selfdestruct(std::unique_ptr<Gtk::Dialog> dialog, Gtk::Root *root)
{
    if (auto const window = dynamic_cast<Gtk::Window *>(root)) {
        dialog->set_transient_for(*window);
    }
    dialog->set_modal();
    dialog->signal_response().connect([d = dialog.get()] (auto) { delete d; });
    dialog->set_visible(true);
    dialog.release(); // deleted by signal_response handler
}

} // namespace Inkscape::UI
