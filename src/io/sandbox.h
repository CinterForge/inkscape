// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::IO::Sandbox
 *
 * Utility functions for running Inkscape inside a "sandboxed" filesystem.
 * (on Linux: xdg-portal with snap or flatpak).
 *
 * Background: To isolate different applications against each other,
 * some newer packaging formats do not allow Inkscape direct access
 * to the user home directory or other paths. Instead, Inkscape can
 * only access special "magic paths" returned by the file-choose dialog.
 * This brings some issues:
 *
 * 1. The file chooser doesn't always return the true path on the host filesystem,
 *   but some replacement, e.g.
 *   /run/user/1000/doc/fe812a2/Foldername instead of /home/user/Documents/path/to/my/Foldername.
 *   We can access the file via the first path, but want to show the second one to the user.
 *
 * 2. If we have access to one file "/path/a.svg", we can't just access other files in the same folder.
 *   Automatically suggesting filenames, e.g. for export, is not possible anymore.
 *   Similarly, editing paths in a text entry widget is not possible anymore.
 *
 *
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_IO_SANDBOX_H
#define SEEN_INKSCAPE_IO_SANDBOX_H

#include <giomm/file.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <optional>

namespace Inkscape::IO::Sandbox {

/**
 * Query if the filesystem is "sandboxed", e.g., by using xdg-portal in flatpak/snap.
 *
 * @returns
 *     False if we have traditional full access to the filesystem.
 *     True if we do not have full direct access to the host filesystem.
 *     As detailed on the top of this source file, "True" can mean that:
 *     - The paths we receive from the file chooser
 *       are not the true paths on the host filesystem
 *     - The user should not be able to manually enter or edit paths in a textbox,
 *       because we don't have access to these without calling the file chooser.
 */
bool filesystem_is_sandboxed();

/**
 * Translate raw filesystem path to a path suitable for display.
 *
 * This function is similar to Gio::File::get_parse_name() and
 * Glib::filename_display_name() but understands filesystem sandboxing.
 *
 * @param path File object representing the path (may be a folder or file).
 *             To represent empty values, use `std::nullopt` or `Gio::File::create_from_path("")`.
 * @param placeholder_if_empty Placeholder to be returned if the input path is empty.
 *                    Value is in UTF8 encoding.
 * @returns "Human-readable" path that can be shown to the user.
 * If possible, this is a full path. If not, it may only be a file or folder name.
 * This new path should not be used programmatically and should not be edited by the user.
 * Value is in UTF8 encoding.
 */
Glib::ustring filesystem_get_display_path(std::optional<Glib::RefPtr<Gio::File const>> path,
                                          Glib::ustring placeholder_if_empty = "");

} // namespace Inkscape::IO::Sandbox

#endif // SEEN_INKSCAPE_IO_SANDBOX_H

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
