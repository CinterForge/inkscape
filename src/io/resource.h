// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape::IO::Resource - simple resource API
 *//*
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Martin Owens <doctormo@gmail.com>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_IO_RESOURCE_H
#define SEEN_INKSCAPE_IO_RESOURCE_H

#include <string>
#include <vector>

#include "util/share.h"

namespace Inkscape::IO::Resource {

enum Type {
    ATTRIBUTES,
    EXAMPLES,
    EXTENSIONS,
    FONTS,
    FONTCOLLECTIONS,
    ICONS,
    KEYS,
    MARKERS,
    NONE,
    PAINT,
    PALETTES,
    SCREENS,
    TEMPLATES,
    TUTORIALS,
    SYMBOLS,
    FILTERS,
    THEMES,
    UIS,
    DOCS
};

enum Domain {
    SYSTEM,
    CREATE,
    CACHE,
    SHARED,
    USER
};

[[nodiscard]]
Util::ptr_shared get_path(Domain domain, Type type,
                          char const *filename=nullptr,
                          char const *extra=nullptr);

[[nodiscard]]
std::string get_path_string(Domain domain, Type type,
                            char const *filename = nullptr,
                            char const *extra=nullptr);

std::string get_filename(Type type, char const *filename, bool localized = false, bool silent = false);

// TODO: GTK4: Glib::StdStringView
std::string get_filename(std::string const &path, std::string const& filename);

// TODO: C++20: Use std::span instead of std::vector.

[[nodiscard]]
std::vector<std::string> get_filenames(Type type,
                                       std::vector<const char *> const &extensions={},
                                       std::vector<const char *> const &exclusions={});

[[nodiscard]]
std::vector<std::string> get_filenames(Domain domain, Type type,
                                       std::vector<const char *> const &extensions={},
                                       std::vector<const char *> const &exclusions={});

[[nodiscard]]
std::vector<std::string> get_filenames(std::string path,
                                       std::vector<const char *> const &extensions={},
                                       std::vector<const char *> const &exclusions={});

[[nodiscard]]
std::vector<std::string> get_foldernames(Type type,
                                         std::vector<const char *> const &exclusions = {});

[[nodiscard]]
std::vector<std::string> get_foldernames(Domain domain, Type type,
                                         std::vector<const char *> const &exclusions = {});

[[nodiscard]]
std::vector<std::string> get_foldernames(std::string const &path,
                                         std::vector<const char *> const &exclusions = {});

void get_foldernames_from_path(std::vector<std::string> &folders, std::string const &path,
                               std::vector<const char *> const &exclusions = {});

void get_filenames_from_path(std::vector<std::string> &files, std::string const &path,
                             std::vector<const char *> const &extensions = {},
                             std::vector<const char *> const &exclusions = {});

[[nodiscard]] std::string profile_path();
[[nodiscard]] std::string profile_path(const char *filename);
[[nodiscard]] std::string shared_path();
[[nodiscard]] std::string shared_path(const char *filename);
[[nodiscard]] std::string homedir_path();
[[nodiscard]] std::string log_path(const char *filename);

} // namespace Inkscape::IO::Resource

#endif // SEEN_INKSCAPE_IO_RESOURCE_H

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
