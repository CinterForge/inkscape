// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * LPE "Ellipse through 5 points" implementation.
 */
/*
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <optional>
#include <2geom/pathvector.h>

#include "live_effects/effect.h"
#include "message.h"

#ifndef SEEN_LPE_ELLIPSE_5PTS_H
#define SEEN_LPE_ELLIPSE_5PTS_H

namespace Inkscape::LivePathEffect {

class LPEEllipse5Pts final : public Effect
{
public:
    LPEEllipse5Pts(LivePathEffectObject *lpeobject);
    ~LPEEllipse5Pts() final { _clearWarning(); }

    LPEEllipse5Pts(LPEEllipse5Pts const &) = delete;
    LPEEllipse5Pts& operator=(LPEEllipse5Pts const &) = delete;

    Geom::PathVector doEffect_path(Geom::PathVector const &path_in) final;

private:
    void _flashWarning(char const *message);
    void _clearWarning();

    std::optional<MessageId> _error = std::nullopt;
    Geom::PathVector const _unit_circle;
};

} // namespace Inkscape::LivePathEffect

#endif // SEEN_LPE_ELLIPSE_5PTS_H

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
