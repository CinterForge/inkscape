// SPDX-License-Identifier: GPL-2.0-or-later
//
// Safer replacement for sprintf()
//
// When invoked with a char buffer of size known to compiler, it will call
// snprintf passing correct buffer size without programmer specifying one explicitly.
//
// When buffer size is only known at runtime, one should use snprintf instead.

#ifndef INKSCAPE_SAFE_PRINTF_H
#define INKSCAPE_SAFE_PRINTF_H

#include <cstdarg>
#include <cstddef>
#include <glib/gmacros.h>

template<size_t N>
int G_GNUC_PRINTF(2, 3) safeprintf(char (&buf)[N], const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    auto len = vsnprintf(buf, N, fmt, args);
    va_end(args);
    return len;
}

#endif
