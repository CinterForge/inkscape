// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Abstraction of the store/snapshot mechanism.
 */
/*
 * Copyright (C) 2022 PBS <pbs3141@gmail.com>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_CANVAS_STORES_H
#define INKSCAPE_UI_WIDGET_CANVAS_STORES_H

#include "fragment.h"
#include "ui/util.h"

namespace Inkscape::UI::Widget {

struct Fragment;
class Prefs;
class Graphics;

class Stores
{
public:
    enum class Mode
    {
        None,     /// Not initialised or just reset; no stores exist yet.
        Normal,   /// Normal mode consisting of just a backing store.
        Decoupled /// Decoupled mode consisting of both a backing store and a snapshot store.
    };

    enum class Action
    {
        None,      /// The backing store was not changed.
        Recreated, /// The backing store was completely recreated.
        Shifted    /// The backing store was shifted into a new rectangle.
    };
    
    struct Store : Fragment
    {
        /**
         * The region of space containing drawn content.
         * For the snapshot, this region is transformed to store space and approximated inwards.
         */
        Cairo::RefPtr<Cairo::Region> drawn;
    };

    /// Construct a blank object with no stores.
    Stores(Prefs const &prefs)
        : _mode(Mode::None)
        , _graphics(nullptr)
        , _prefs(prefs) {}

    /// Set the pointer to the graphics object.
    void set_graphics(Graphics *g) { _graphics = g; }

    /// Discards all stores. (The actual operation on the graphics is performed on the next update().)
    void reset();

    /// Respond to a viewport change. (Requires a valid graphics.)
    Action update(Fragment const &view);

    /// Respond to drawing of the backing store having finished. (Requires a valid graphics.)
    Action finished_draw(Fragment const &view);

    /// Record a rectangle as being drawn to the store.
    void mark_drawn(Geom::IntRect const &rect) { _store.drawn->do_union(geom_to_cairo(rect)); }

    // Getters.
    Store const &store() const { return _store; }
    Store const &snapshot() const { return _snapshot; }
    Mode mode() const { return _mode; }

private:
    // Internal state.
    Mode _mode;
    Store _store, _snapshot;

    // The graphics object that executes the operations on the stores.
    Graphics *_graphics;

    // The preferences object we read preferences from.
    Prefs const &_prefs;

    // Internal actions.
    Geom::IntRect centered(Fragment const &view) const;
    void recreate_store(Fragment const &view);
    void shift_store(Fragment const &view);
    void take_snapshot(Fragment const &view);
    void snapshot_combine(Fragment const &view);
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_STORES_H

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
