// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Auto-save
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Re-write of code formerly in inkscape.cpp and originally written by Jon Cruz and others.
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <glibmm/fileutils.h>
#include <glibmm/i18n.h> // Internationalization
#include <glibmm/main.h>
#include <glibmm/miscutils.h>

#include "auto-save.h"
#include "document.h"
#include "inkscape-application.h"
#include "io/recent-files.h"
#include "preferences.h"
#include "extension/output.h"
#include <sigc++/scoped_connection.h>
#include "io/sys.h"
#include "xml/repr.h"
#include "util/trim.h"

#ifdef _WIN32
#include <process.h>
typedef int uid_t;
#define getuid() 0
#endif

namespace Inkscape {

void
AutoSave::init(InkscapeApplication* app)
{
    _app = app;
    start();
}

void
AutoSave::start()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    static sigc::scoped_connection autosave_connection;

    // Turn off any previous timeout.
    autosave_connection.disconnect();

    if (prefs->getBool("/options/autosave/enable", true)) {
        // Turn on autosave (timeout is in seconds).
        guint32 timeout = std::max(prefs->getInt("/options/autosave/interval", 10), 1) * 60;
        if (timeout > 60 * 60 * 24) {
            // Sanity check
            std::cerr << "AutoSave::start: auto-save interval set to greater than one day. Not enabling." << std::endl;
            return;
        }
        autosave_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &AutoSave::save), timeout);
    }
}

bool
AutoSave::save()
{
    std::vector<SPDocument *> documents = _app->get_documents();
    if (documents.empty()) {
        // Nothing to save!
        return true;
    }

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // Find/create autosave directory
    std::string autosave_dir = prefs->getString("/options/autosave/path"); // Filenames should be std::string
    if (autosave_dir.empty()) {
        autosave_dir = Glib::build_filename(Glib::get_user_cache_dir(), "inkscape");
    }

    Glib::RefPtr<Gio::File> dir_file = Gio::File::create_for_path(autosave_dir);
    if (!dir_file->query_exists()) {
        if (!dir_file->make_directory_with_parents()) {
            std::cerr << "InkscapeApplication::document_autosave: Failed to create autosave directory: " << autosave_dir << std::endl;
            return true;
        }
    }

    // Get unique info
    uid_t uid = getuid(); // Avoid naming conflicts between users
    int pid = ::getpid(); // Avoid naming conflicts between processes

    // Get time stamp
    std::time_t time = std::time(nullptr);
    std::tm tm = *std::localtime(&time);
    std::stringstream datetime;
    datetime << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");

    int docnum = 0;
    int autosave_max = prefs->getInt("/options/autosave/max", 10);
    std::string preset_name = "autosave";

    // Remove old files over the max autosave number
    for (auto document : documents) {

        ++docnum; // Give each document a unique number.

        if (document->isModifiedSinceAutoSave()) {
            // Base name: document filename + user ID
            auto document_filename = document->getDocumentFilename();
            Glib::ustring doc_name =
                (document_filename ? Glib::path_get_basename(document->getDocumentFilename())
                                                 : "untitled");
            Util::trim(doc_name, ".svg");
            std::string base_name = doc_name + "-" + std::to_string(uid);

            // The following we do for each document (rather wasteful...) so that
            // we make room for each document that needs saving. We probably should
            // be counting per document and not overall documents.

            // Open directory
            Glib::Dir directory(autosave_dir);
            std::vector<std::string> file_names(directory.begin(), directory.end());

            // Sort them so that oldest are last (file name encodes time).
            std::sort(file_names.begin(), file_names.end(), std::greater<std::string>());

            // Delete oldest files.
            int count = 0;
            for (auto &file_name : file_names) {
                if (file_name.compare(0, preset_name.size(), preset_name) == 0) {
                    ++count;
                    if (count >= autosave_max) {
                        // Delete (making room for one more).
                        std::string path = Glib::build_filename(autosave_dir, file_name);
                        if (unlink(path.c_str()) == -1) {
                            std::cerr << "InkscapeApplication::document_autosave: Failed to unlink file: "
                                      << path << ": " << strerror(errno) << std::endl;
                        } else {
                            Inkscape::IO::removeInkscapeRecent(path);
                        }
                    }
                }
            }

            // Construct save file path
            // datetime MUST happen first, otherwise the above sorting will fail
            std::string filename = preset_name + "-" + datetime.str() + "-" + base_name + "-" + std::to_string(pid) +
                                   "-" + std::to_string(docnum) + ".svg";
            std::string path = Glib::build_filename(autosave_dir, filename.c_str());

            // Try to save the file
            // Following code needs to be reviewed
            FILE *file = Inkscape::IO::fopen_utf8name(path.c_str(), "w");
            gchar *errortext = nullptr;
            if (file) {
                try {
                    Inkscape::XML::Node *repr = document->getReprRoot();
                    sp_repr_save_stream(repr->document(), file, SP_SVG_NS_URI);
                    Inkscape::IO::addInkscapeRecentSvg(path, document->getDocumentName() ? document->getDocumentName() : "unnamed", {"Auto"}, document_filename ? document_filename : "");
                } catch (Inkscape::Extension::Output::no_extension_found &e) {
                    errortext = g_strdup(_("Autosave failed! Could not find inkscape extension to save document."));
                } catch (Inkscape::Extension::Output::save_failed &e) {
                    auto const safeUri = Inkscape::IO::sanitizeString(path.c_str());
                    errortext = g_strdup_printf(_("Autosave failed! File %s could not be saved."), safeUri.c_str());
                }
                fclose(file);
            } else {
                auto const safeUri = Inkscape::IO::sanitizeString(path.c_str());
                errortext = g_strdup_printf(_("Autosave failed! File %s could not be saved."), safeUri.c_str());
            }

            if (errortext) {
                g_warning("%s", errortext);
                g_free(errortext);
            } else {
                document->setModifiedSinceAutoSaveFalse();
            }
        }
    } // Loop over documents

    return true;
}

void
AutoSave::restart()
{
    AutoSave::getInstance().start();
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
