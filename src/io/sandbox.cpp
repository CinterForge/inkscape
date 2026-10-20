// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Utility functions for running Inkscape inside a "sandboxed" filesystem.
 *
 * For documentation, see sandbox.h
 */

#include "sandbox.h"

#include <giomm/file.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <optional>

namespace Inkscape::IO::Sandbox {

bool filesystem_is_sandboxed()
{
    // Simplification: Whenever xdg portal is active, we assume that the full filesystem is hidden behind the portal.
    // In reality, it may be more complex, e.g., we could have access to the home directory but not to external media.

    // Linux (flatpak/snap):
    return !Glib::getenv("GTK_USE_PORTAL").empty();
    // FUTURE: Add MacOS App Sandbox?
}

Glib::ustring filesystem_get_display_path(std::optional<Glib::RefPtr<Gio::File const>> path,
                                          Glib::ustring placeholder_if_empty)
{
    // Note: If File points to a URL, get_path() may be empty even if get_parse_name() is not.
    // Therefore, to test for emptiness we use get_parse_name().
    if (!path.has_value() || path.value()->get_parse_name().empty()) {
        return placeholder_if_empty;
    }
    auto path_native = path.value()->get_path();
    if (filesystem_is_sandboxed()) {
        /* FUTURE: Try to get the true path. For xdg-portal, see https://gitlab.gnome.org/GNOME/gtk/-/issues/7102 .
         * Extra care is needed to avoid that IO operations freeze Inkscape if the path has become
         * inaccessible. Otherwise we would cause bugs such as:
         * https://gitlab.com/inkscape/inkscape/-/merge_requests/6294
         *
         * WORKAROUND: We just display the last part of the path, i.e.,
         * the filename or the name of the lowest directory.
         *
         * FUTURE: This method should be moved upstream to Glib::filename_display_name or some property of Gio::File
         */
        return Glib::filename_display_basename(path_native);
    }
    // FUTURE: Improve display, e.g. "My Documents" instead of /home/user/Documents
    return path.value()->get_parse_name();
}

} // namespace Inkscape::IO::Sandbox
