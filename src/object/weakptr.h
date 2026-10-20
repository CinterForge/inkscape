// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_OBJECT_WEAKPTR_H
#define INKSCAPE_OBJECT_WEAKPTR_H

#include <sigc++/connection.h>

namespace Inkscape {

/**
 * A weak pointer to an SPObject: it nulls itself upon the object's destruction.
 */
template <typename T>
class SPWeakPtr final
{
public:
    SPWeakPtr() = default;
    explicit SPWeakPtr(T *obj) : _obj(obj) { attach(); }
    SPWeakPtr &operator=(T *obj) { reset(obj); return *this; }
    SPWeakPtr(SPWeakPtr const &other) : SPWeakPtr(other._obj) {}
    SPWeakPtr &operator=(SPWeakPtr const &other) { reset(other._obj); return *this; }
    ~SPWeakPtr() { detach(); }

    void reset() { detach(); _obj = nullptr; }
    void reset(T *obj) { detach(); _obj = obj; attach(); }
    explicit operator bool() const { return _obj; }
    T *get() const { return _obj; }
    T &operator*() const { return *_obj; }
    T *operator->() const { return _obj; }

private:
    T *_obj = nullptr;
    sigc::connection _conn;

    void attach() { if (_obj) _conn = _obj->connectRelease([this] (auto) { _conn.disconnect(); _obj = nullptr; }); }
    void detach() { if (_obj) _conn.disconnect(); }
};

} // namespace Inkscape

#endif // INKSCAPE_OBJECT_WEAKPTR_H
