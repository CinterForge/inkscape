// SPDX-License-Identifier: GPL-2.0-or-later

#include "string-convert.h"

#include <cassert>
#include <codecvt>
#include <locale>
#include <string>

#include <glib.h>

namespace Inkscape {

// std::wstring_convert is deprecated and will be removed in C++26,
// but it doesn't have a replacement yet in std library.
// It is expected that the two functions below using wstring_convert will
// no longer be needed (due to newer lcms2) before this happens.

std::wstring utf8_to_wstring(std::string const &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

std::string wstring_to_utf8(wchar_t const *wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

std::string unicode_char_to_utf8(uint32_t unicode)
{
    std::string result;
    result.resize(6);
    auto size = g_unichar_to_utf8(unicode, result.data());
    result.resize(size);
    return result;
}

} // namespace Inkscape
