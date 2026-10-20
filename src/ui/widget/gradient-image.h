// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * A simple gradient preview
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_GRADIENT_IMAGE_H
#define SEEN_SP_GRADIENT_IMAGE_H

#include <gtkmm/drawingarea.h>

class SPGradient;
class SPObject;
class SPStop;

namespace Gdk {
class Pixbuf;
} // namespace Gdk

namespace Inkscape::UI::Widget {

class GradientImage : public Gtk::DrawingArea
{
public:
    explicit GradientImage(SPGradient *gradient);
    void set_gradient(SPGradient *gr);

private:
    SPGradient *_gradient = nullptr;
    sigc::scoped_connection _release_connection;
    sigc::scoped_connection _modified_connection;

    void draw_func(Cairo::RefPtr<Cairo::Context> const &cr, int width, int height);
};

} // namespace Inkscape::UI::Widget

Glib::RefPtr<Gdk::Pixbuf> sp_gradient_to_pixbuf(SPGradient *gr, int width, int height);
Cairo::RefPtr<Cairo::ImageSurface> sp_gradient_to_surface(SPGradient* gr, int width, int height);
Cairo::RefPtr<Cairo::ImageSurface> sp_gradstop_to_surface(SPStop *stop, int width, int height);

#endif // SEEN_SP_GRADIENT_IMAGE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
