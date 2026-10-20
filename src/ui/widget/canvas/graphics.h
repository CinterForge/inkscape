// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Display backend interface.
 * Copyright (C) 2022 PBS <pbs3141@gmail.com>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_CANVAS_GRAPHICS_H
#define INKSCAPE_UI_WIDGET_CANVAS_GRAPHICS_H

#include <optional>
#include <vector>

#include <cairomm/refptr.h>
#include "display/rendermode.h"
#include "fragment.h"

namespace Cairo {
class Context;
class ImageSurface;
} // namespace Cairo

namespace Inkscape::UI::Widget {

class Stores;
class Prefs;

struct PageInfo
{
    std::vector<Geom::Rect> pages;
};

class Graphics
{
public:
    // Creation/destruction.
    static std::unique_ptr<Graphics> create_gl   (Prefs const &prefs, Stores const &stores, PageInfo const &pi);
    static std::unique_ptr<Graphics> create_cairo(Prefs const &prefs, Stores const &stores, PageInfo const &pi);
    virtual ~Graphics() = default;

    // State updating.
    virtual void set_scale_factor(int) = 0; ///< Set the HiDPI scale factor.
    virtual void set_outlines_enabled(bool) = 0; ///< Whether to maintain a second layer of outline content.
    virtual void set_background_in_stores(bool) = 0; ///< Whether to assume the first layer is drawn on top of background or transparency.

    /// Set colours for background/page shadow drawing.
    virtual void set_colours(std::uint32_t page, std::uint32_t desk, std::uint32_t shadow) = 0;
    virtual void set_shadow_size(float size) = 0;

    // Store manipulation.
    virtual void recreate_store(Geom::IntPoint const &dims) = 0; ///< Set the store to a surface of the given size, of unspecified contents.
    virtual void shift_store(Fragment const &dest) = 0; ///< Called when the store fragment shifts position to \a dest.
    virtual void swap_stores() = 0; ///< Exchange the store and snapshot surfaces.
    virtual void fast_snapshot_combine() = 0; ///< Paste the store onto the snapshot.
    virtual void snapshot_combine(Fragment const &dest) = 0; ///< Paste the snapshot followed by the store onto a new snapshot at \a dest.
    virtual void invalidate_snapshot() = 0; ///< Indicate that the content in the snapshot store is not going to be used again.

    // Misc.
    virtual bool is_opengl() const = 0; ///< Whether this is an OpenGL backend.
    virtual void invalidated_glstate() = 0; ///< Tells the Graphics to no longer rely on any OpenGL state it had set up.

    // Tile drawing.
    /// Return a surface for drawing on. If nogl is true, no GL commands are issued, as is a requirement off-main-thread. All such surfaces must be
    /// returned by passing them either to draw_tile() or junk_tile_surface().
    virtual Cairo::RefPtr<Cairo::ImageSurface> request_tile_surface(Geom::IntRect const &rect, bool nogl) = 0;
    /// Commit the contents of a surface previously issued by request_tile_surface() to the canvas. In outline mode, a second surface must be passed
    /// containing the outline content, otherwise it should be null.
    virtual void draw_tile(Fragment const &fragment, Cairo::RefPtr<Cairo::ImageSurface> surface, Cairo::RefPtr<Cairo::ImageSurface> outline_surface) = 0;
    /// Get rid of a surface previously issued by request_tile_surface() without committing it to the canvas. Usually useful only to dispose of
    /// surfaces which have gone into an error state while rendering, which is irreversible, and therefore we can't do anything useful with them.
    virtual void junk_tile_surface(Cairo::RefPtr<Cairo::ImageSurface> surface) = 0;

    // Widget painting.
    struct PaintArgs
    {
        std::optional<Geom::Point> mouse;
        RenderMode render_mode{};
        SplitMode splitmode{};
        Geom::Point splitfrac;
        SplitDirection splitdir{};
        SplitDirection hoverdir{};
        double yaxisdir{};
    };
    virtual void paint_widget(Fragment const &view, PaintArgs const &args, Cairo::RefPtr<Cairo::Context> const &cr) = 0;

    // Static functions providing common functionality.
    static bool check_single_page(Fragment const &view, PageInfo const &pi);
    static std::pair<Geom::IntRect, Geom::IntRect> calc_splitview_cliprects(Geom::IntPoint const &size, Geom::Point const &splitfrac, SplitDirection splitdir);
    static void paint_splitview_controller(Geom::IntPoint const &size, Geom::Point const &splitfrac, SplitDirection splitdir, SplitDirection hoverdir, Cairo::RefPtr<Cairo::Context> const &cr);
    static void paint_background(Fragment const &fragment, PageInfo const &pi,
                                 std::uint32_t page, std::uint32_t desk,
                                 Cairo::RefPtr<Cairo::Context> const &cr);
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_CANVAS_GRAPHICS_H

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
