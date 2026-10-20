// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File operations (independent of GUI)
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_FILE_IO_H
#define INK_FILE_IO_H

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <string>
#include <memory>
#include <span>

namespace Gio {
class File;
} // namespace Gio

class SPDocument;

std::unique_ptr<SPDocument> ink_file_new(std::string const &Template = "");
std::unique_ptr<SPDocument> ink_file_open(std::span<char const> buffer);
std::pair<std::unique_ptr<SPDocument>, bool /*cancelled*/> ink_file_open(Glib::RefPtr<Gio::File> const &file);

namespace Inkscape::IO {

class TempFilename {
public:
    TempFilename(const std::string &pattern);
    ~TempFilename();

    std::string const &get_filename() const { return _filename; }

private:
    std::string _filename;
    int _tempfd = 0;
};

std::string find_original_file(Glib::StdStringView filepath, Glib::StdStringView name);

} // namespace Inkscape::IO

// To do:
// ink_file_save()
// ink_file_export()
// ink_file_import()

#endif // INK_FILE_IO_H

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
