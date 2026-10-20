// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Author: Rafał Siejakowski <rs@rs-math.net>
 *   Copyright (c) 2023
 *
 * Released under GNU GPL v2+, see the file 'COPYING' for more information.
*/
/** @file
 *  @brief
 *  A helper variant type wrapping either an owning pointer (std::unique_ptr) or a non-owning, plain pointer.
 */
#ifndef SEEN_INKSCAPE_UTIL_HYBRID_POINTER_H
#define SEEN_INKSCAPE_UTIL_HYBRID_POINTER_H

#include <concepts>
#include <memory>
#include <variant>

namespace Inkscape::Util {

/** @brief
 * A helper class holding an owning or non-owning pointer depending on the memory management requirements.
 * Useful when we need to uniformly handle objects allocated statically by an external dynamically loaded
 * library alongside objects of the same type created and managed directly by Inkscape.
 */
template<typename T>
class HybridPointer
{
    using OwningPtr = std::unique_ptr<T>;
    using NonOwningPtr = T *;

    std::variant<NonOwningPtr, OwningPtr> _pointer = nullptr;

    HybridPointer(OwningPtr owning_pointer) { _pointer.template emplace<OwningPtr>(std::move(owning_pointer)); }
    HybridPointer(NonOwningPtr nonowning_pointer) { _pointer.template emplace<NonOwningPtr>(nonowning_pointer); }

public:
    HybridPointer() = default;

    template<typename DerivedType, typename... Args> requires std::is_base_of_v<T, DerivedType>
    static HybridPointer make_owning(Args&& ...args) { return {std::make_unique<DerivedType>(args...)}; }
    static HybridPointer make_nonowning(T *plain_pointer) { return {plain_pointer}; }

    NonOwningPtr get() const {
        if (NonOwningPtr const *plain_pointer = std::get_if<NonOwningPtr>(&_pointer)) {
            return *plain_pointer;
        }
        return std::get<OwningPtr>(_pointer).get();
    }

    NonOwningPtr operator->() const { return get(); }
    explicit operator bool() const { return bool(get()); }

    /// Adopt an owning pointer.
    HybridPointer &operator=(OwningPtr &&adopt) { _pointer = std::move(adopt); return *this; }
};

}  // namespace Inkscape::Util

#endif // SEEN_INKSCAPE_UTIL_HYBRID_POINTER_H

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
