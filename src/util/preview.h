// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Utility functions for previewing icon representation.
 */
/* Authors:
 *   Jon A. Cruz
 *   Bob Jamison
 *   Other dudes from The Inkscape Organization
 *   Abhishek Sharma
 *   Anshudhar Kumar Singh <anshudhar2001@gmail.com>
 *
 * Copyright (C) 2004 Bob Jamison
 * Copyright (C) 2005,2010 Jon A. Cruz
 * Copyright (C) 2021 Anshudhar Kumar Singh
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UTIL_PREVIEW_H
#define INKSCAPE_UTIL_PREVIEW_H

#include <cstdint>
#include <cairomm/surface.h>

#include "display/drawing.h"
#include "async/channel.h"

class SPDocument;
class SPItem;

namespace Inkscape {
namespace UI {
namespace Preview {

/**
 * Launch a background task to render a drawing to a surface.
 *
 * If the area to render is invalid, nothing is returned and no action is taken.
 * Otherwise, first the drawing is snapshotted, then an async task is launched to render the drawing to a surface.
 * Upon completion, the drawing is unsnapshotted on the calling thread and the result passed to onfinished().
 * If the return object is destroyed before this happens, then the drawing will instead be destroyed on an unspecified
 * thread while still in the snapshotted state.
 *
 * Contracts: (This isn't Rust, so we need a comment instead, and great trust in the caller.)
 *
 *    - The caller must ensure onfinished() remains valid to call during the lifetime of the return object.
 *      (This is the same as for sigc::slots and connections.)
 *
 *    - The caller must not call drawing->unsnapshot(), or any other method that bypasses snapshotting.
 *      However, it is ok to modify or destroy drawing in any other way, because the background task has shared
 *      ownership of the drawing (=> Sync), and snapshotting prevents modification of the data being read by the
 *      background task (=> Send/const).
 */
Cairo::RefPtr<Cairo::ImageSurface>
render_preview(SPDocument *doc, std::shared_ptr<Inkscape::Drawing> drawing, uint32_t bg_color, Inkscape::DrawingItem *item,
                                    unsigned width_in, unsigned height_in, Geom::Rect const &dboxIn);

} // namespace Preview
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UTIL_PREVIEW_H
