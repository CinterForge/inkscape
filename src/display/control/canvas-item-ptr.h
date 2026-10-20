// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_CANVAS_ITEM_PTR_H
#define SEEN_CANVAS_ITEM_PTR_H

/*
 * An entirely analogous file to display/drawing-item-ptr.h.
 */

#include <memory>
#include <type_traits>

namespace Inkscape { class CanvasItem; }

/// Deleter object which calls the unlink() method of CanvasItem.
struct CanvasItemUnlinkDeleter
{
    template <typename T>
    void operator()(T *t)
    {
        static_assert(std::is_base_of_v<Inkscape::CanvasItem, T>);
        t->unlink();
    }
};

/// Smart pointer used to hold CanvasItems, like std::unique_ptr.
template <typename T>
using CanvasItemPtr = std::unique_ptr<T, CanvasItemUnlinkDeleter>;

/// Convienence function to create a CanvasItemPtr, like std::make_unique.
template <typename T, typename... Args>
auto make_canvasitem(Args&&... args)
{
    return CanvasItemPtr<T>(new T(std::forward<Args>(args)...));
};

#endif // SEEN_CANVAS_ITEM_PTR_H
