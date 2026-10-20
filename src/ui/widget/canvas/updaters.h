// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Controls the order to update invalidated regions.
 * Copyright (C) 2022 PBS <pbs3141@gmail.com>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_CANVAS_UPDATERS_H
#define INKSCAPE_UI_WIDGET_CANVAS_UPDATERS_H

#include <2geom/int-rect.h>
#include <cairomm/region.h>

namespace Inkscape {
namespace UI {
namespace Widget {

// A class for tracking invalidation events and producing redraw regions.
class Updater
{
public:
    virtual ~Updater() = default;

    // The subregion of the store with up-to-date content.
    Cairo::RefPtr<Cairo::Region> clean_region;

    enum class Strategy
    {
        Responsive, // As soon as a region is invalidated, redraw it.
        FullRedraw, // When a region is invalidated, delay redraw until after the current redraw is completed.
        Multiscale, // Updates tiles near the mouse faster. Gives the best of both.
    };

    // Create an Updater using the given strategy.
    template <Strategy strategy>
    static std::unique_ptr<Updater> create();

    // Create an Updater using a choice of strategy specified at runtime.
    static std::unique_ptr<Updater> create(Strategy strategy);

    // Return the strategy in use.
    virtual Strategy get_strategy() const = 0;

    virtual void reset() = 0;                                          // Reset the clean region to empty.
    virtual void intersect (Geom::IntRect const &) = 0;                // Called when the store changes position; clip everything to the new store rectangle.
    virtual void mark_dirty(Geom::IntRect const &) = 0;                // Called on every invalidate event.
    virtual void mark_dirty(Cairo::RefPtr<Cairo::Region> const &) = 0; // Called on every invalidate event.
    virtual void mark_clean(Geom::IntRect const &) = 0;                // Called on every rectangle redrawn.

    // Called at the start of a redraw to determine what region to consider clean (i.e. will not be drawn).
    virtual Cairo::RefPtr<Cairo::Region> get_next_clean_region() = 0;

    // Called after a redraw has finished. Returns true to indicate that further redraws are required with different clean regions.
    virtual bool report_finished() = 0;

    // Called at the start of each frame. Some updaters (Multiscale) require this information.
    virtual void next_frame() = 0;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_CANVAS_UPDATERS_H

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
