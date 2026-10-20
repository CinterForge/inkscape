// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Access Inkscape's recent files
 *
 * Copyright 2025 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <vector>
#include <optional>

#include <gtkmm/recentmanager.h>
#include <gtkmm/recentinfo.h>

namespace Inkscape::IO {

void resetRecentInkscapeList();
std::vector<Glib::RefPtr<Gtk::RecentInfo>> getInkscapeRecentFiles(unsigned max_files = 0, bool is_autosave = false);
void addInkscapeRecentSvg(std::string const &filename, std::string const &name, std::vector<Glib::ustring> group = {}, std::optional<std::string> original = {});
void removeInkscapeRecent(std::string const &filename);
std::optional<std::string> openAsInkscapeRecentOriginalFile(std::string const &filename);
Glib::RefPtr<Gtk::RecentInfo> getInkscapeRecent(std::string const &filename);
std::map<Glib::ustring, std::string> getShortenedPathMap(std::vector<Glib::RefPtr<Gtk::RecentInfo>> const &recent_files);

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
