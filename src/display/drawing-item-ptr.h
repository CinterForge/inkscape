// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_DISPLAY_DRAWINGITEM_PTR_H
#define INKSCAPE_DISPLAY_DRAWINGITEM_PTR_H

#include <memory>
#include <type_traits>

namespace Inkscape { class DrawingItem; }

/**
 * Deleter object which calls the unlink() method of DrawingItem to schedule deferred destruction.
 */
struct UnlinkDeleter
{
    template <typename T>
    void operator()(T *t)
    {
        static_assert(std::is_base_of_v<Inkscape::DrawingItem, T>);
        t->unlink();
    }
};

/**
 * Smart pointer used by the Object Tree to hold items in the Display Tree, like std::unique_ptr.
 *
 * Upon deletion, the pointed-to object and its subtree will be destroyed immediately if not currently in use by a snapshot.
 * Otherwise, destruction is deferred to after the snapshot is released.
 */
template <typename T>
using DrawingItemPtr = std::unique_ptr<T, UnlinkDeleter>;

/**
 * Convienence function to create a DrawingItemPtr, like std::make_unique.
 */
template <typename T, typename... Args>
auto make_drawingitem(Args&&... args)
{
    return DrawingItemPtr<T>(new T(std::forward<Args>(args)...));
};

#endif // INKSCAPE_DISPLAY_DRAWINGITEM_PTR_H
