// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A class hierarchy implementing various ways of streaming pixel buffers to the GPU.
 * Copyright (C) 2022 PBS <pbs3141@gmail.com>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_CANVAS_PIXELSTREAMER_H
#define INKSCAPE_UI_WIDGET_CANVAS_PIXELSTREAMER_H

#include <2geom/int-point.h>
#include <cairomm/surface.h>

namespace Inkscape {
namespace UI {
namespace Widget {

// A class for turning Cairo image surfaces into OpenGL textures.
class PixelStreamer
{
public:
    virtual ~PixelStreamer() = default;

    // Method for streaming pixels to the GPU.
    enum class Method
    {
        Auto,         // Use the best option available at runtime.
        Persistent,   // Persistent buffer mapping. (Best, requires OpenGL 4.4.)
        Asynchronous, // Ordinary buffer mapping. (Almost as good, requires OpenGL 3.0.)
        Synchronous   // Synchronous texture uploads. (Worst but still tolerable, requires OpenGL 1.1.)
    };

    // Create a PixelStreamer using a choice of method specified at runtime, falling back if unsupported.
    static std::unique_ptr<PixelStreamer> create_supported(Method method);

    // Return the method in use.
    virtual Method get_method() const = 0;

    /**
     * Request a drawing surface of the given dimensions. If nogl is true, no GL commands will be issued,
     * but the request may fail. An effort is made to keep such failures to a minimum.
     *
     * The surface must be returned to the PixelStreamer by calling finish(), in order to deallocate
     * GL resourecs.
     */
    virtual Cairo::RefPtr<Cairo::ImageSurface> request(Geom::IntPoint const &dimensions, bool nogl = false) = 0;

    /**
     * Give back a drawing surface produced by request(), uploading the contents to the currently bound texture.
     * The texture must be at least as big as the surface.
     *
     * If junk is true, then the surface will be junked instead, meaning nothing will be done with the contents,
     * and its GL resources will simply be deallocated.
     */
    virtual void finish(Cairo::RefPtr<Cairo::ImageSurface> surface, bool junk = false) = 0;
};

} // namespace Widget
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_WIDGET_CANVAS_PIXELSTREAMER_H

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
