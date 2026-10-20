// SPDX-License-Identifier: GPL-2.0-or-later
//
// String conversion routines

#ifndef INKSCAPE_UTIL_STRING_STRING_CONVERT_H
#define INKSCAPE_UTIL_STRING_STRING_CONVERT_H

#include <string>
#include <cstdint>

namespace Inkscape {

// Convert UTF8-encoded string to wide-character string
std::wstring utf8_to_wstring(std::string const &str);

// Convert null-terminated wide string to UTF8-encoded string
std::string wstring_to_utf8(wchar_t const *wstr);

// Convert single Unicode character into UTF8-encoded string
std::string unicode_char_to_utf8(uint32_t unicode);

} // namespace Inkscape

#endif // INKSCAPE_UTIL_STRING_STRING_CONVERT_H
