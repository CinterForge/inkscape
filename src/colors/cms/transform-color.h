// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors: see git history
 *
 * Copyright (C) 2024-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_CMS_TRANSFORM_COLOR_H
#define SEEN_COLORS_CMS_TRANSFORM_COLOR_H

#include <memory>
#include <vector>

#include "transform.h"

#include "colors/spaces/enum.h"

namespace Inkscape::Colors::CMS {

class Profile;
class TransformColor : public Transform
{
public:
    TransformColor(std::shared_ptr<Profile> const &from,
                   std::shared_ptr<Profile> const &to, RenderingIntent intent);

    bool do_transform(std::vector<double> &io) const;

    template <typename In, typename Out>
    bool do_transform(In const &i, Out &o)
    {
        assert(i.size() >= _channels_in && o.size() >= _channels_out);
        cmsDoTransform(_handle, &i.front(), &o.front(), 1);
        return true;
    }

private:

    int _channels_in;
    int _channels_out;
};

class GamutChecker : public Transform
{
public:
    GamutChecker(std::shared_ptr<Profile> const &from,
                 std::shared_ptr<Profile> const &to);
    bool check_gamut(std::vector<double> const &input) const;
};

} // namespace Inkscape::Colors::CMS

#endif // SEEN_COLORS_CMS_TRANSFORM_COLOR_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
