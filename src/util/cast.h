// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Hand-rolled LLVM-style RTTI system for class hierarchies where dynamic_cast isn't fast enough.
 */
#ifndef INKSCAPE_UTIL_CAST_H
#define INKSCAPE_UTIL_CAST_H

#include <type_traits>

/*
 * In order to use this system with a class hierarchy, specialize the following templates for
 * every member of the hierarchy. The only requirement is that
 *
 *     first_tag<T> <= first_tag<S> <= last_tag<T>
 *
 * exactly when S is a derived class of T. Then add to each class the line of boilerplate
 *
 *     int tag() const override { return tag_of<decltype(*this)>; }
 */
template <typename T> inline constexpr int first_tag = std::enable_if<!sizeof(T), void>::value;
template <typename T> inline constexpr int last_tag  = std::enable_if<!sizeof(T), void>::value;

/**
 * Convenience function to retrieve the tag (class id) of a given type.
 */
template <typename T> inline constexpr int tag_of = first_tag<std::remove_cv_t<std::remove_reference_t<T>>>;

/**
 * Equivalent to the boolean value of dynamic_cast<T const *>(...).
 *
 * If the supplied pointer is null, the check fails.
 *
 * The function can also be called with an object or reference,
 * where it is equivalent to the boolean value of dynamic_cast<T const *>(& ...).
 *
 * To help catch redundant checks, checks that are known at compile time currently generate
 * a compile error. Please feel free to remove these static_asserts if they become unhelpful.
 */
template <typename T>
inline constexpr auto is = [] <typename P> (P const &p)
{
    auto object_is = [] <typename S> (S const &s) {
        if constexpr (std::is_base_of_v<T, S>) {
            static_assert(!sizeof(T), "check is always true");
            return true;
        } else if constexpr (std::is_base_of_v<S, T>) {
            auto const s_tag = s.tag();
            return first_tag<T> <= s_tag && s_tag <= last_tag<T>;
        } else {
            static_assert(!sizeof(T), "check is always false");
            return false;
        }
    };

    if constexpr (std::is_pointer_v<P>) {
        return p && object_is(*p);
    } else {
        return object_is(p);
    }
};

/**
 * Equivalent to static_cast<T [const] *>(...) where the const is deduced.
 */
template <typename T>
inline constexpr auto cast_unsafe = [] <typename S> (S *s)
{
    if constexpr (std::is_const_v<S>) {
        return static_cast<T const *>(s);
    } else {
        return static_cast<T *>(s);
    }
};

/**
 * Equivalent to dynamic_cast<T [const] *>(...) where the const is deduced.
 *
 * If the supplied pointer is null, the result is null.
 *
 * To help catch redundant casts, casts that are known at compile time currently generate
 * a compile error. Please feel free to remove these static_asserts if they become unhelpful.
 */
template <typename T>
inline constexpr auto cast = [] <typename S> (S *s)
{
    if constexpr (std::is_base_of_v<T, S>) {
        // Removed static assert; it complicates template "collect_items"
        // static_assert(!sizeof(T), "cast is unnecessary");
        return cast_unsafe<T>(s);
    } else if constexpr (std::is_base_of_v<S, T>) {
        return is<T>(s) ? cast_unsafe<T>(s) : nullptr;
    } else {
        static_assert(!sizeof(T), "cast is impossible");
        return nullptr;
    }
};

#endif // INKSCAPE_UTIL_CAST_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
