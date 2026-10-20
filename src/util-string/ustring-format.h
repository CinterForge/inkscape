// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2002 The gtkmm Development Team
 *
 * Functions to format output in one of:
 *   1. The default locale:     format_default()
 *   2. A specified locale:     format_locale()
 *   3. The classic "C" locale: format_classic()
 *
 * Based on Glib::ustring::format()  (glibmm/glib/glibmm/ustring.cc)
 * (FormatStream is a private class, thus we provide a copy here.)
 *
 * Can be replaced when Apple Clang supports std::format.
 */

#ifndef SEEN_INKSCAPE_USTRING_FORMAT_H
#define SEEN_INKSCAPE_USTRING_FORMAT_H

#include <sstream>
#include <glibmm/ustring.h>

namespace Inkscape::ustring {

template <typename... T>
inline Glib::ustring format_classic(T const &... args)
{
    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    (ss << ... << args);
    return ss.str();
}

} // namespace Inkscape::ustring

#endif // SEEN_INKSCAPE_USTRING_FORMAT_H

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
