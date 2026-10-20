// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the C++ glue between Inkscape and Potrace
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *   Stéphane Gimenez <dev@gim.name>
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 *
 * Potrace, the wonderful tracer located at http://potrace.sourceforge.net,
 * is provided by the generosity of Peter Selinger, to whom we are grateful.
 *
 */
#ifndef INKSCAPE_TRACE_POTRACE_H
#define INKSCAPE_TRACE_POTRACE_H

#include <optional>
#include <unordered_set>
#include <boost/functional/hash.hpp>
#include <2geom/point.h>
#include <2geom/path-sink.h>
#include "trace/trace.h"
#include "trace/imagemap.h"
using potrace_param_t = struct potrace_param_s;
using potrace_path_t  = struct potrace_path_s;

namespace Inkscape {
namespace Trace {
namespace Potrace {

enum class TraceType
{
    BRIGHTNESS,
    BRIGHTNESS_MULTI,
    CANNY,
    QUANT,
    QUANT_COLOR,
    QUANT_MONO,
    // Used in tracedialog.cpp
    AUTOTRACE_SINGLE,
    AUTOTRACE_MULTI,
    AUTOTRACE_CENTERLINE
};

class PotraceTracingEngine final
    : public TracingEngine
{
public:
    PotraceTracingEngine();
    PotraceTracingEngine(TraceType traceType,
                         bool invert,
                         int quantizationNrColors,
                         double brightnessThreshold,
                         double brightnessFloor,
                         double cannyHighThreshold,
                         int multiScanNrColors,
                         bool multiScanStack,
                         bool multiScanSmooth ,
                         bool multiScanRemoveBackground);
    ~PotraceTracingEngine() override;

    TraceResult trace(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress) override;
    Glib::RefPtr<Gdk::Pixbuf> preview(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf) override;

    TraceResult traceGrayMap(GrayMap const &grayMap, Async::Progress<double> &progress);

    void setOptiCurve(int);
    void setOptTolerance(double);
    void setAlphaMax(double);
    void setTurdSize(int);

private:
    potrace_param_t *potraceParams;

    TraceType traceType = TraceType::BRIGHTNESS;

    // Whether the image should be inverted at the end.
    bool invert = false;

    // Color -> b&w quantization
    int quantizationNrColors = 8;

    // Brightness items
    double brightnessThreshold = 0.45;
    double brightnessFloor = 0.0;

    // Canny items
    double cannyHighThreshold = 0.65;

    // Color -> multiscan quantization
    int multiScanNrColors = 8;
    bool multiScanStack = true; // do we tile or stack?
    bool multiScanSmooth = false; // do we use gaussian filter?
    bool multiScanRemoveBackground = false; // do we remove the bottom trace?

    void common_init();

    TraceResult traceQuant          (Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress);
    TraceResult traceBrightnessMulti(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress);
    TraceResult traceSingle         (Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress);

    IndexedMap filterIndexed(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf) const;
    std::optional<GrayMap> filter(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf) const;

    Geom::PathVector grayMapToPath(GrayMap const &gm, Async::Progress<double> &progress);

    void writePaths(potrace_path_t *paths, Geom::PathBuilder &builder, std::unordered_set<Geom::Point> &points, Async::Progress<double> &progress) const;
};

} // namespace Potrace
} // namespace Trace
} // namespace Inkscape

#endif // INKSCAPE_TRACE_POTRACE_H

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
