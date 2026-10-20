// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for file handling tied to the application and without GUI.
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-file.h"

#include <iostream>

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>

#include "actions-helper.h"
#include "document.h"
#include "document-undo.h"
#include "inkscape.h"             // Inkscape::Application
#include "inkscape-application.h"


// Actions for file handling (should be integrated with file dialog).

void
file_open(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);

    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(s.get());
    if (!file->query_exists()) {
        show_output(Glib::ustring("file_open: file '") + s.get().raw() + "' does not exist.");
        return;
    }
    auto document = app->document_open(file).first;
    if (!document) {
        return;
    }

    app->set_active_document(document);
    app->set_active_selection(document->getSelection());
    app->set_active_desktop(nullptr);

    document->ensureUpToDate();
}

void
file_open_with_window(const Glib::VariantBase& value, InkscapeApplication *app)
{
    auto window = app->get_active_window();
    if (!window) {
        show_output("You cannot run this action without an active window");
        return;
    }

    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(s.get());
    if (!file->query_exists()) {
        show_output(Glib::ustring("file_open: file '") + s.get().raw() + "' does not exist.");
        // Falling through to `app->create_window()` will show the user an error dialog.
    }
    app->create_window(file);
}


void
file_new(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);

    auto document = app->document_new(s.get());
    if (!document) {
        return;
    }

    app->set_active_document(document);
    app->set_active_selection(document->getSelection());
    app->set_active_desktop(nullptr); // No desktop (yet).

    document->ensureUpToDate();
}

void
file_rebase(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> s = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    SPDocument *document = app->get_active_document();
    document->rebase(s.get());

    document->ensureUpToDate();
    Inkscape::DocumentUndo::done(document, RC_("Undo", "Replace file contents"), "");
}

// Need to create a document_revert that doesn't depend on windows.
// void
// file_revert(InkscapeApplication *app)
// {
//     app->document_revert(app->get_current_document());
// }

// No checks for dataloss are performed. Useful for scripts.
void
file_close(InkscapeApplication *app)
{
    SPDocument *document = app->get_active_document();
    app->document_close(document);

    app->set_active_document(nullptr);
    app->set_active_selection(nullptr);
    app->set_active_desktop(nullptr);
}

const Glib::ustring SECTION = NC_("Action Section", "File");

std::vector<std::vector<Glib::ustring>> raw_data_file =
{
    // clang-format off
    {"app.file-open",              N_("File Open"),                SECTION,       N_("Open file")                                         },
    {"app.file-new",               N_("File New"),                 SECTION,       N_("Open new document using template")                  },
    {"app.file-close",             N_("File Close"),               SECTION,       N_("Close active document")                             },
    {"app.file-open-window",       N_("File Open Window"),         SECTION,       N_("Open file window")                                  },
    {"app.file-rebase",            N_("File Contents Replace"),    SECTION,       N_("Replace current document's contents by contents of another file")                 }
    // clang-format on
};

std::vector<std::vector<Glib::ustring>> hint_data_file =
{
    // clang-format off
    {"app.file-open",               N_("Enter file name")},
    {"app.file-new",                N_("Enter file name")},
    {"app.file-open-window",        N_("Enter file name")},
    {"app.file-rebase-from-saved",  N_("Namedview; Update=1, Replace=0")}
    // clang-format on
};

void
add_actions_file(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);
    Glib::VariantType BString(Glib::VARIANT_TYPE_BYTESTRING);

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)
    auto *gapp = app->gio_app();

    // clang-format off
    gapp->add_action_with_parameter( "file-open",                 String, sigc::bind(sigc::ptr_fun(&file_open),               app));
    gapp->add_action_with_parameter( "file-new",                  String, sigc::bind(sigc::ptr_fun(&file_new),                app));
    gapp->add_action_with_parameter( "file-open-window",          String, sigc::bind(sigc::ptr_fun(&file_open_with_window),   app));
    gapp->add_action(                "file-close",                        sigc::bind(sigc::ptr_fun(&file_close),              app));
    gapp->add_action_with_parameter( "file-rebase",               Bool,   sigc::bind(sigc::ptr_fun(&file_rebase),             app));
    // clang-format on
#else
            show_output("add_actions: Some actions require Glibmm 2.52, compiled with: " << glib_major_version << "." << glib_minor_version);
#endif

    app->get_action_extra_data().add_data(raw_data_file);
    app->get_action_hint_data().add_data(hint_data_file);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
