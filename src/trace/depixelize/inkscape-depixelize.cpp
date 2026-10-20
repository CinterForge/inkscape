// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This is the C++ glue between Inkscape and libdepixelize
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *   Stéphane Gimenez <dev@gim.name>
 *
 * Copyright (C) 2004-2006 Authors
 */

#include "inkscape-depixelize.h"

#include <glibmm/i18n.h>
#include <depixelize/depixelize.h>

#include "colors/utils.h"
#include "async/progress.h"
#include "svg/css-ostringstream.h"

namespace Inkscape::Trace::Depixelize {

DepixelizeTracingEngine::DepixelizeTracingEngine(TraceType traceType, double curves, int islands, int sparsePixels, double sparseMultiplier, bool optimize)
    : traceType(traceType)
{
    params = std::make_unique<::Depixelize::Options>();
    params->curves_multiplier = curves;
    params->islands_weight = islands;
    params->sparse_pixels_radius = sparsePixels;
    params->sparse_pixels_multiplier = sparseMultiplier;
    params->optimize = optimize;
}

DepixelizeTracingEngine::~DepixelizeTracingEngine() = default;

TraceResult DepixelizeTracingEngine::trace(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf, Async::Progress<double> &progress)
{
    TraceResult res;

    auto const img = ::Depixelize::Image{
        .pixels = std::as_const(*pixbuf).get_pixels(), // as_const necessary to opt into non-COW overload
        .n_channels = pixbuf->get_n_channels(),
        .width = pixbuf->get_width(),
        .height = pixbuf->get_height(),
        .rowstride = pixbuf->get_rowstride()
    };

    ::Depixelize::Splines splines;

    if (traceType == TraceType::VORONOI) {
        splines = ::Depixelize::to_voronoi(img, *params);
    } else {
        splines = ::Depixelize::to_splines(img, *params);
    }

    progress.report_or_throw(0.5);

    auto subprogress = Async::SubProgress(progress, 0.5, 0.5);
    auto throttled = Async::ProgressStepThrottler(subprogress, 0.02);

    int num_splines = std::distance(splines.begin(), splines.end());
    int i = 0;

    for (auto &it : splines) {
        throttled.report_or_throw((double)i / num_splines);
        i++;

        auto hex = Inkscape::Colors::rgba_to_hex(
                           SP_RGBA32_U_COMPOSE(unsigned(it.rgba[0]),
                                               unsigned(it.rgba[1]),
                                               unsigned(it.rgba[2]),
                                               unsigned(it.rgba[3])));
        Inkscape::CSSOStringStream ss;
        ss << "fill:" << hex.c_str() << ";fill-opacity:" << (it.rgba[3] / 255.0f) << ";";
        res.emplace_back(ss.str(), std::move(it.pathVector));
    }

    return res;
}

Glib::RefPtr<Gdk::Pixbuf> DepixelizeTracingEngine::preview(Glib::RefPtr<Gdk::Pixbuf> const &pixbuf)
{
    return pixbuf;
}

bool DepixelizeTracingEngine::check_image_size(Geom::IntPoint const &size) const
{
    return size.x() > 256 || size.y() > 256;
}

} // namespace Inkscape::Trace::Depixelize

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
