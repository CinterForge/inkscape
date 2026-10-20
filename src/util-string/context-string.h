// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * ContextString - wrapper type for context-marked translatable strings.
 *
 * Used to enforce at compile time that certain APIs receive strings
 * translated with gettext context (via C_() macro).
 */
/* Authors:
 *   Marcin Floryan <marcin.floryan@gmail.com>
 *
 * Copyright (C) 2026 Authors
 */

#ifndef INKSCAPE_UTIL_CONTEXT_STRING_H
#define INKSCAPE_UTIL_CONTEXT_STRING_H

#include <glibmm/i18n.h>  // for C_() macro

namespace Inkscape::Util::Internal {

/**
 * A wrapper around a translated string that enforces context-marked translation.
 *
 * Use the RC_() macro to create instances:
 *
 *     DocumentUndo::done(doc, RC_("Undo", "Fit Page to Drawing"), "");
 *
 * The explicit constructor prevents implicit conversion from const char*,
 * so passing _("text") or a raw literal will fail to compile.
 * You still have to provide a relevant context string.
 * For `done` and `maybeDone` this should be "Undo".
 */

class ContextString {
    const char* _str = nullptr;

public:
    ContextString() = delete;
    explicit ContextString(const char* s) : _str(s) {}

    const char* c_str() const { return _str; }
};

} // namespace Inkscape::Util::Internal

/**
 * Required Context translation macro.
 *
 * Like C_(context, text) but returns a ContextString, enforcing that
 * APIs requiring context-marked translations receive them.
 *
 * Configure xgettext with: --keyword=RC_:1c,2
 */
#define RC_(context, text) Inkscape::Util::Internal::ContextString(C_(context, text))

#endif // INKSCAPE_UTIL_CONTEXT_STRING_H
