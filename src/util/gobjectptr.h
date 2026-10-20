// SPDX-License-Identifier: GPL-2.0-or-later
/** \file GObjectPtr
 * A smart pointer for wrapping a GObject.
 */

#ifndef INKSCAPE_UTIL_GOBJECTPTR_H
#define INKSCAPE_UTIL_GOBJECTPTR_H

#include <glib.h>

namespace Inkscape::Util {

/**
 * A smart pointer that shares ownership of a GObject.
 */
template <typename T>
class GObjectPtr
{
public:
    GObjectPtr() = default;
    explicit GObjectPtr(T *p, bool add_ref = false) : _p(p) { if (add_ref) _ref(); }
    GObjectPtr(GObjectPtr const &other) : _p(other._p) { _ref(); }
    GObjectPtr &operator=(GObjectPtr const &other) { if (&other != this) { _unref(); _p = other._p; _ref(); } return *this; }
    GObjectPtr(GObjectPtr &&other) noexcept : _p(other._p) { other._p = nullptr; }
    GObjectPtr &operator=(GObjectPtr &&other) { if (&other != this) { _unref(); _p = other._p; other._p = nullptr; } return *this; }
    ~GObjectPtr() { _unref(); }

    void reset() { _unref(); _p = nullptr; }
    explicit operator bool() const { return _p; }
    T *get() const { return _p; }
    T &operator*() const { return *_p; }
    T *operator->() const { return _p; }

private:
    T *_p = nullptr;

    void _ref() { if (_p) g_object_ref(_p); }
    void _unref() { if (_p) g_object_unref(_p); }
};

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_GOBJECTPTR_H
