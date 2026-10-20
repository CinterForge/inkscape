// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UTIL_OPTSTR_H
#define INKSCAPE_UTIL_OPTSTR_H
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <optional>

namespace Inkscape {
namespace Util {

inline bool equal(std::optional<std::string> const &a, char const *b)
{
    return a && b ? *a == b : !a && !b;
}

inline auto to_opt(char const *s)
{
    return s ? std::make_optional<std::string>(s) : std::nullopt;
}

inline auto to_cstr(std::optional<std::string> const &s)
{
    return s ? s->c_str() : nullptr;
}

inline bool assign(std::optional<std::string> &a, char const *b)
{
    if (equal(a, b)) return false;
    a = to_opt(b);
    return true;
}

} // namespace Util
} // namespace Inkscape

#endif // INKSCAPE_UTIL_OPTSTR_H
