// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Create a list of recentyly used files.
 *
 * Copyright 2025 Martin Owens <doctormo@geek-2.com>
 * Copyright 2024, 2025 Tavmjong Bah <tavmjong@free.fr>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "recent-files.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <gtk/gtk.h>

#include "io/split-path.h"

namespace Inkscape::IO {

/**
 * Return the default Gtk::RecentManager, or an empty RefPtr when GTK has not
 * been initialized (e.g. running headless without a display).
 *
 * Calling Gtk::RecentManager::get_default() before GTK is initialized emits a
 * spurious "Failed to wrap object of type 'GtkRecentManager'" warning, because
 * the gtkmm wrappers are only registered once GTK is set up. The recent files
 * list is a GUI-only concept, so there is nothing to do without a display.
 * See https://gitlab.com/inkscape/inkscape/-/work_items/5419.
 */
Glib::RefPtr<Gtk::RecentManager> get_recent_manager()
{
    if (!gtk_is_initialized()) {
        return {};
    }
    return Gtk::RecentManager::get_default();
}

#ifdef _WIN32
constexpr size_t platform_index = 1;
#else
constexpr size_t platform_index = 0;
#endif

static const Glib::ustring recent_app_name = "org.inkscape.Inkscape";

/**
 * Generate a vector of recently used Inkscape files.
 *
 * @arg max_files - Limits the output to this number of files, zero means no-maximum.
 * @arg is_autosave - Limit the list to just auto save files.
 *
 * @returns a vector of string pairs, a display label and the full uri.
 */
std::vector<Glib::RefPtr<Gtk::RecentInfo>> getInkscapeRecentFiles(unsigned max_files, bool is_autosave)
{
    std::vector<std::pair<std::string, Glib::ustring>> output;

    auto recent_manager = get_recent_manager();
    if (!recent_manager) {
        return {};
    }
    // All recent files, not necessarily inkscape only (std::vector)
    auto recent_files = recent_manager->get_items();

    // Remove non-inkscape files.
    std::erase_if(recent_files, [is_autosave](auto const &recent_file) -> bool {
        // Note: Do not check if the file exists, to avoid long delays. See https://gitlab.com/inkscape/inkscape/-/issues/2348.
        bool valid_file =
            recent_file->has_application(g_get_prgname())         ||
            recent_file->has_application(recent_app_name)         ||
            recent_file->has_application("inkscape")              ||
            recent_file->has_application("inkscape.exe");
        valid_file = valid_file && is_autosave == recent_file->has_group("Auto");
        return !valid_file;
    });

    // Ensure that display uri's are unique. It is possible that an XBEL file
    // has multiple entries for the same file as a path can be written in equivalent
    // ways: i.e. with a ';' or '%3B', or with a drive name of 'c' or 'C' on Windows.
    // These entries may have the same display uri's. This causes segfaults in
    // getShortendPathmap().
    auto sort_comparator_uri =
        [](auto const a, auto const b) -> bool { return a->get_uri_display() < b->get_uri_display(); };
    std::sort (recent_files.begin(), recent_files.end(), sort_comparator_uri);

    auto unique_comparator_uri =
        [](auto const a, auto const b) -> bool { return a->get_uri_display() == b->get_uri_display(); };
    auto it_u = std::unique (recent_files.begin(), recent_files.end(), unique_comparator_uri);
    recent_files.erase(it_u, recent_files.end());

    // Sort by "last modified" time, which puts the most recently opened files first.
    std::sort(std::begin(recent_files), std::end(recent_files), [](auto const &a, auto const &b) -> bool {
        // a should precede b if a->get_modified() is later than b->get_modified()
        return a->get_modified().compare(b->get_modified()) > 0;
    });

    // Truncate to user-specified max_files.
    if (max_files && recent_files.size() > max_files) {
        recent_files.resize(max_files);
    }

    return recent_files;
}

/**
 * Add a recent file to the Gtk RecentFiles manager for an SVG file.
 *
 * @arg filename - An absolute local filename of the document in question
 * @arg name     - The name of the document
 * @arg groups   - Optional groups, used for AutoSave and Crash
 * @arg original - The filename to the original document, where available. If used this save is marked as private.
 */
void addInkscapeRecentSvg(std::string const &filename, std::string const &name, std::vector<Glib::ustring> groups, std::optional<std::string> original)
{
    auto recentmanager = get_recent_manager();
    if (recentmanager && Glib::path_is_absolute(filename)) {
        Glib::ustring uri = Glib::filename_to_uri(filename);
        Glib::ustring original_uri = "";
        if (original && Glib::path_is_absolute(*original)) {
            original_uri = Glib::filename_to_uri(*original);
        }
        recentmanager->add_item(uri, {
            name,                     // Name
            original_uri,             // Description used for original filename
            "image/svg+xml",          // Mime type
            recent_app_name,          // App name
            "",                       // Execute
            groups,    // Groups
            (bool)original,           // Private if points to another document
        });
    }
}

/**
 * Remove a recent file entry, call when deleting files.
 */
void removeInkscapeRecent(std::string const &filename)
{
    if (auto recentmanager = get_recent_manager()) {
        try {
            Glib::ustring uri = Glib::filename_to_uri(filename);
            recentmanager->remove_item(uri);
        } catch (Glib::Error const &) { // lookup failed
        }
    }
}

/**
 * Remove inkscape recent items, but preserve items opened by other programs
 * auto any auto-saves which are considered not user accessable.
 */
void resetRecentInkscapeList()
{
    if (auto recentmanager = get_recent_manager()) {
        for (auto info : recentmanager->get_items()) {
            bool is_ink, is_other = false;
            for (auto &app : info->get_applications()) {
                if ( app == g_get_prgname()
                  || app == recent_app_name
                  || app == "inkscape"
                  || app == "inkscape.exe") {
                    is_ink = true;
                } else {
                    is_other = true;
                }
            }
            if (is_ink && !is_other && !info->has_group("Auto")) {
                recentmanager->remove_item(info->get_uri());
            }
        }
    }
}

/**
 * Get the file recent info for the given path, if there is one.
 */
Glib::RefPtr<Gtk::RecentInfo> getInkscapeRecent(std::string const &filename)
{
    if (auto recentmanager = get_recent_manager()) {
        try {
            Glib::ustring uri = Glib::filename_to_uri(filename);
            return recentmanager->lookup_item(uri);
        } catch (Glib::Error const &) { // lookup failed
        }
    }
    return {};
}

/**
 * Get the original filename for the given file, and remove the recent files entry if it's a crash.
 *
 * @arg filename - The auto save or crash file we are opening.
 *
 * @returns - False optional if this isn't an auto save or crash, an empty string if is is
 *            but doesn't have an original filename because it was unsaved. Otherwise the
 *            original local filename is provided.
 */
std::optional<std::string> openAsInkscapeRecentOriginalFile(std::string const &filename)
{
    if (auto info = getInkscapeRecent(filename)) {
        // Original filename is stored as a URI in the description, see addInkscapeRecentSvg above.
        std::string original;
        if (!info->get_description().empty()) {
            try {
                original = Glib::filename_from_uri(info->get_description());
            } catch (Glib::ConvertError e) {
                g_warning("Could not parse original file URI from recent file: %s", e.what());
            }
        }
        if (info->has_group("Auto")) {
            return original;
        }
        if (info->has_group("Crash")) {
            // Crash files are removed from recent-files tracker on opening
            removeInkscapeRecent(filename);
            return original;
        }
    }
    return {};
}

/**
 * Generate the shortened labeles for a list of recently used files.
 * recent_files must not contain entries with duplicate uri display values.
 */
std::map<Glib::ustring, std::string> getShortenedPathMap(std::vector<Glib::RefPtr<Gtk::RecentInfo>> const &recent_files)
{
    // Create a map of path to shortened path, and prefill.
    std::map<Glib::ustring, std::string> shortened_path_map;
    std::vector<Glib::RefPtr<Gtk::RecentInfo>> copy = recent_files;
    for (auto recent_file : copy) {
        shortened_path_map[recent_file->get_uri_display()] = recent_file->get_display_name();
    }

    // Look for duplicate short names. These are the only ones that matter here.
    auto equal_comparator = [](auto const a, auto const b) -> bool { return a->get_display_name() == b->get_display_name(); };
    auto it = copy.begin();

    while (it != (copy.end() - 1)) {
        it = std::adjacent_find(it, copy.end(), equal_comparator);
        if (it != copy.end()) {

            // Found duplicate display name!
            std::vector<Glib::ustring> display_uris;
            display_uris.emplace_back(( * it   )->get_uri_display());
            display_uris.emplace_back(( *(it+1))->get_uri_display());

            std::vector<Inkscape::IO::PathParts> path_parts;
            path_parts.emplace_back(Inkscape::IO::split_path(display_uris[0].raw()));
            path_parts.emplace_back(Inkscape::IO::split_path(display_uris[1].raw()));

            // Find first directory difference from root down.
            auto max_size = std::min(path_parts[0].size(), path_parts[1].size());
            unsigned i = 0;
            for (; i < max_size; ++i) {
                if (path_parts[0][i] != path_parts[1][i]) {
                    break;
                }
            }

            // If no difference, skip. Should not happen, but possible with ill-formed recently-used.xbel.
            if (i >= max_size) {
                g_warning("Duplicate paths in recently-used.xbel");
                ++it;
                continue;
            }

            // Override map of path to shortened path.
            for (int j = 0; j < 2; j++) {

                auto display_uri = display_uris[j]; // We always use display_uri as map index.
                // Size is always one first element such as '/' or 'C:\' and the last element is the filename
                auto size = path_parts[j].size();

                if (size <= 3) {
                    // If file is in root directory or child of root directory, just use display uri.
                    shortened_path_map[display_uri] = display_uri;
                } else if (i == size - 1) {
                    // If difference is at last path part (file name), use that.
                    shortened_path_map[display_uri] = std::string(path_parts[j].basename());
                } else if (i == size - 2) {

                    // If difference is last directory level (file name), use that + file name.
                    shortened_path_map[display_uri] = std::format("..{0}{1}{0}{2}", G_DIR_SEPARATOR,
                                                                  path_parts[j][size - 2], path_parts[j][size - 1]);
                } else if (i <= platform_index) {
                    // parts[j][i] is actually a root folder or drive
                    shortened_path_map[display_uri] =
                        std::format("{0}{1}{2}{1}..{1}{3}", path_parts[j].prefix(), G_DIR_SEPARATOR,
                                    path_parts[j][platform_index], path_parts[j][size - 1]);
                } else {
                    shortened_path_map[display_uri] =
                        std::format("..{0}{1}{0}..{0}{2}", G_DIR_SEPARATOR, path_parts[j][i], path_parts[j][size - 1]);
                }
            }
        } else {
            // At end!
            break;
        }

        // Test next entry.
        ++it;
    }

    return shortened_path_map;
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
