// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * LPE "Ellipse through 5 points" implementation
 */
/*
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpe-ellipse_5pts.h"

#include <vector>
#include <2geom/ellipse.h>
#include <2geom/path-sink.h>
#include <glibmm/i18n.h>

#include "desktop.h"
#include "inkscape.h"
#include "message-stack.h"

namespace Inkscape::LivePathEffect {

[[nodiscard]] static auto buildUnitCircle()
{
    Geom::PathBuilder builder;
    builder.moveTo({1, 0});
    builder.arcTo(1, 1, 0, true, true, {-1, 0});
    builder.arcTo(1, 1, 0, true, true, { 1, 0});
    builder.closePath();
    return builder.peek();
}

LPEEllipse5Pts::LPEEllipse5Pts(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , _unit_circle{buildUnitCircle()}
{}

/** Flash a warning message on the status bar. */
void LPEEllipse5Pts::_flashWarning(char const *message)
{
    auto &app = Inkscape::Application::instance();
    if (auto desktop = app.active_desktop()) {
        _clearWarning();
        _error = desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, message);
    }
}

/** Clear our warning from the status bar. */
void LPEEllipse5Pts::_clearWarning()
{
    if (!_error) {
        return;
    }

    auto &app = Inkscape::Application::instance();
    if (auto desktop = app.active_desktop()) {
        desktop->messageStack()->cancel(*_error);
        _error = std::nullopt;
    }
}

/** Fit an ellipse to the first 5 nodes in the given PathVector. */
Geom::PathVector LPEEllipse5Pts::doEffect_path(Geom::PathVector const &path_in)
{
    auto const &source = path_in[0];
    if (source.size() < 4 /* For 5 nodes, we need at least 4 segments. */) {
        _flashWarning(_("Five points required for constructing an ellipse"));
        return path_in;
    }

    std::vector<Geom::Point> source_points;
    source_points.reserve(5);
    for (int i = 0; i < 5; i++) {
        source_points.push_back(source.pointAt((Geom::Coord)i));
    }

    Geom::Ellipse ellipse;
    bool fitting_fail = false;
    try {
        ellipse.fit(source_points);
    } catch (Geom::RangeError &e) {
        fitting_fail = true;
    }
    if (fitting_fail || ellipse.ray(Geom::X) == 0 || ellipse.ray(Geom::Y) == 0) {
        _flashWarning(_("No unique ellipse passing through these points"));
        return path_in;
    }
    _clearWarning();

    // Transform the unit circle contour to the fitted ellipse.
    return _unit_circle * ellipse.unitCircleTransform();
}

} // namespace Inkscape::LivePathEffect

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
