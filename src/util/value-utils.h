// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UTIL_VALUE_UTILS_H
#define INKSCAPE_UTIL_VALUE_UTILS_H

/**
 * @file
 * Wrapper for the GLib value API. Differs from the Glibmm wrapper by
 * - allowing non-default-constructible types,
 * - eliminating copies,
 * - single-line construction and type testing,
 * - thread-safe type registration.
 *
 * Glib::ValueBase is used to repesent unique ownership of a value.
 * 
 * All objects are stored in values as boxed types recognisable only to Inkscape.
 * For more efficient storage of basic types (e.g. int, string),
 * or interoperability with GTK (e.g. dragging/pasting text),
 * the Glibmm wrapper must still be used.
 */

#include <glibmm/utility.h>
#include <glibmm/value.h>
#include <gdkmm/contentprovider.h>

namespace Inkscape::Util::GlibValue {

/// Returns the type used for storing an object of type T inside a value.
template <class T>
GType type()
{
    static auto const type = [] {
        auto name = std::string{"inkscape_glibvalue_"};
        Glib::append_canonical_typename(name, typeid(T).name());

        return g_boxed_type_register_static(
            name.c_str(),
            +[] (void *p) -> void * {
                return new (std::nothrow) T{*static_cast<T *>(p)};
            },
            +[] (void *p) {
                delete static_cast<T *>(p);
            }
        );
    }();
    return type;
}

/// Tests whether a value contains an object of type T.
template <typename T>
bool holds(GValue const *value)
{
    return G_VALUE_HOLDS(value, type<T>());
}

/// Tests whether a value contains an object of type T.
template <typename T>
bool holds(Glib::ValueBase const &value)
{
    return holds<T>(value.gobj());
}

/// Returns a borrowed pointer to the T held by a value if it holds one, else nullptr.
template <typename T>
T *get(GValue *value)
{
    if (holds<T>(value)) {
        return static_cast<T *>(g_value_get_boxed(value));
    }
    return nullptr;
}

/// Returns a borrowed pointer to the T held by a value if it holds one, else nullptr.
template <typename T>
T *get(Glib::ValueBase &value)
{
    return get<T>(value.gobj());
}

/// Returns a borrowed pointer to the T held by a value if it holds one, else nullptr.
template <typename T>
T const *get(GValue const *value)
{
    if (holds<T>(value)) {
        return static_cast<T const *>(g_value_get_boxed(value));
    }
    return nullptr;
}

/// Returns a borrowed pointer to the T held by a value if it holds one, else nullptr.
template <typename T>
T const *get(Glib::ValueBase const &value)
{
    return get<T>(value.gobj());
}

/**
 * Return a value containing and taking ownership of the given T instance.
 * The argument must not be null.
 */
template <typename T>
Glib::ValueBase own(std::unique_ptr<T> t)
{
    assert(t);
    Glib::ValueBase value;
    value.init(type<T>());
    g_value_take_boxed(value.gobj(), t.release());
    return value;
}

/// Return a value containing and owning a newly-created T instance.
template <typename T, typename... Args>
Glib::ValueBase create(Args&&... args)
{
    return own(std::make_unique<T>(std::forward<Args>(args)...));
}

/// Release the value from its owning wrapper, leaving the original in an empty state.
inline GValue release(Glib::ValueBase &&value)
{
    return std::exchange(*value.gobj(), GValue(G_VALUE_INIT));
}

/// Attempt to get a value of type T from a content provider, returning it on success, otherwise nullptr.
template <typename T>
std::unique_ptr<T> from_content_provider(Gdk::ContentProvider const &content_provider)
{
    Glib::ValueBase value;
    value.init(type<T>());
    // Fixme: Value inside content provider is copied rather than returned directly.
    if (!gdk_content_provider_get_value(const_cast<Gdk::ContentProvider &>(content_provider).gobj(), value.gobj(), nullptr)) {
        return {};
    }
    auto const t = static_cast<T *>(g_value_get_boxed(value.gobj()));
    *value.gobj() = G_VALUE_INIT;
    return std::unique_ptr<T>(t);
}

} // namespace Inkscape::Util::GlibValue

#endif // INKSCAPE_UTIL_VALUE_UTILS_H
