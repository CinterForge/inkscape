// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "colors/cms/profile.h"
#include "colors/cms/transform-color.h"

static std::string grb_profile = INKSCAPE_TESTS_DIR "/data/colors/SwappedRedAndGreen.icc";
static std::string cmyk_profile = INKSCAPE_TESTS_DIR "/data/colors/default_cmyk.icc";

using namespace Inkscape::Colors;

namespace {

TEST(ColorCmsTransformColor, applyTransformColor)
{
    auto srgb = CMS::Profile::create_srgb();
    auto profile = CMS::Profile::create_from_uri(grb_profile);
    auto tr = CMS::TransformColor(srgb, profile, RenderingIntent::RELATIVE_COLORIMETRIC);

    std::vector<double> output = {0.1, 0.2, 0.3, 1.0};
    tr.do_transform(output);
    ASSERT_NEAR(output[0], 0.2, 0.01);
    ASSERT_NEAR(output[1], 0.1, 0.01);
    ASSERT_NEAR(output[2], 0.3, 0.01);
    ASSERT_EQ(output[3], 1.0);
}

TEST(ColorCmsGamutChecker, gamutCheckColor)
{
    auto srgb = CMS::Profile::create_srgb();
    auto profile = CMS::Profile::create_from_uri(cmyk_profile);
    ASSERT_TRUE(srgb);
    ASSERT_TRUE(profile);

    auto tr1 = CMS::GamutChecker(srgb, profile);

    // An RGB color which is within the cmyk color profile gamut
    EXPECT_FALSE(tr1.check_gamut({0.83, 0.19, 0.49}));

    // An RGB color (magenta) which is outside the cmyk color profile
    EXPECT_TRUE(tr1.check_gamut({1.0, 0.0, 1.0}));
}

} // namespace

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
