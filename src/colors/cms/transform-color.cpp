// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Convert CMS Colors into different spaces (see Color::profileToProfile)
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <numeric>

#include "transform-color.h"
#include "profile.h"

#include "colors/color.h"

namespace Inkscape::Colors::CMS {

/**
 * Construct a transformation suitable for Space::CMS transformations using the given rendering intent
 *
 * @arg from - The CMS Profile the color data will start in
 * @arg to - The target CMS Profile the color data needs to end up in.
 * @arg intent - The rendering intent to use when changing the gamut and white balance.
 */
TransformColor::TransformColor(std::shared_ptr<Profile> const &from, std::shared_ptr<Profile> const &to,
                               RenderingIntent intent)
    : Transform(cmsCreateTransform(
        from->getHandle(),
        // Colors may have Alpha, but are NEVER premultiplied and we only convert one "pixel"
        // at a time so there's no need to specify the presence of the alpha which is optional
        lcms_color_format(from),
        to->getHandle(),
        lcms_color_format(to),
        lcms_intent(intent),
        lcms_bpc(intent)), true)
    , _channels_in(from->getSize())
    , _channels_out(to->getSize())
{
}

/**
 * Apply the CMS transform to a single Color object's data.
 *
 * @arg io - The input/output color as a vector of numbers between 0.0 and 1.0
 *
 * @returns the modified color in io
 */
bool TransformColor::do_transform(std::vector<double> &io) const
{
    bool alpha = (int)io.size() == _channels_in + 1;

    // Pad data for output channels
    while ((int)io.size() < _channels_out + alpha) {
        io.insert(io.begin() + _channels_in, 0.0);
    }

    cmsDoTransform(_handle, &io.front(), &io.front(), 1);

    // Trim data for output channels
    while ((int)io.size() > _channels_out + alpha) {
        io.erase(io.end() - 1 - alpha);
    }
    return true;
}

/**
 * Create a unique fingerprint context used for alarm codes in gamut checker.
 */
static cmsContext get_gamut_context()
{
    static cmsContext check_context = nullptr;
    if (!check_context) {
        // Create an lcms context just for checking out of gamut colors, this can live as long as inkscape.
        check_context = cmsCreateContext(nullptr, nullptr);
        cmsUInt16Number alarmCodes[cmsMAXCHANNELS] = {0, 0, 0, 0, 0};
        cmsSetAlarmCodesTHR(check_context, alarmCodes);
    }
    return check_context;
}

/**
 * Format gamut checker transform to 16bit ints as expected by alarmCodes
 */
static int lcms_gamut_format(std::shared_ptr<Profile> const &profile)
{
    return cmsFormatterForColorspaceOfProfile(profile->getHandle(), 2, false);
}

/**
 * Construct a transformation suitable for Space::CMS gamut checking
 *
 * @arg from - The CMS Profile the color data will start in
 * @arg to - The target CMS Profile the color data needs to end up in.
 */
GamutChecker::GamutChecker(std::shared_ptr<Profile> const &from,
                           std::shared_ptr<Profile> const &to)
    : Transform(cmsCreateProofingTransformTHR(
        get_gamut_context(),
        from->getHandle(),
        lcms_gamut_format(from),
        from->getHandle(),
        lcms_gamut_format(from),
        to->getHandle(),
        INTENT_RELATIVE_COLORIMETRIC,
        INTENT_RELATIVE_COLORIMETRIC,
        cmsFLAGS_GAMUTCHECK | cmsFLAGS_SOFTPROOFING), true)
{}

/**
 * Return true if the input color is outside of the gamut if it was transformed using
 * this cms transform.
 *
 * @arg input - The input color as a vector of numbers between 0.0 and 1.0
 */
bool GamutChecker::check_gamut(std::vector<double> const &input) const
{
    cmsUInt16Number in[cmsMAXCHANNELS];
    cmsUInt16Number out[cmsMAXCHANNELS];
    for (int i = 0; i < cmsMAXCHANNELS; i++) {
        in[i] = (int)input.size() > i ? input[i] * 65535 : 0;
        out[i] = 0;
    }
    cmsDoTransform(_handle, &in, &out, 1);
    // All channels are set to zero in the alarm context, so the result is zero if out of gamut.
    return std::accumulate(std::begin(out), std::end(out), 0, std::plus<int>()) == 0;
}

} // namespace Inkscape::Colors::CMS

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
