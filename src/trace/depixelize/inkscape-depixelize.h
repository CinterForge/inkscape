// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the C++ glue between Inkscape and Potrace
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Potrace, the wonderful tracer located at http://potrace.sourceforge.net,
 * is provided by the generosity of Peter Selinger, to whom we are grateful.
 *
 */
#ifndef INKSCAPE_TRACE_DEPIXELIZE_H
#define INKSCAPE_TRACE_DEPIXELIZE_H

#include "trace/trace.h"

namespace Depixelize { class Options; }

namespace Inkscape::Trace::Depixelize {

enum class TraceType
{
    VORONOI,
    BSPLINES
};

class DepixelizeTracingEngine final
    : public TracingEngine
{
public:
    DepixelizeTracingEngine() = default;
    DepixelizeTracingEngine(TraceType traceType, double curves, int islands, int sparsePixels, double sparseMultiplier, bool optimize);
    ~DepixelizeTracingEngine() override;

    TraceResult trace(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress) override;
    Glib::RefPtr<Gdk::Pixbuf> preview(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf) override;
    bool check_image_size(Geom::IntPoint const &size) const override;

private:
    std::unique_ptr<::Depixelize::Options> params;
    TraceType traceType = TraceType::VORONOI;
};

} // Inkscape::Trace::Depixelize

#endif // INKSCAPE_TRACE_DEPIXELIZE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
