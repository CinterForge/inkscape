// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit test for Image Surface conversions
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "../test-utils.h"
#include "colors/cms/profile.h"
#include "colors/cms/transform-surface.h"
#include "colors/cms/transform.h"
#include "colors/spaces/enum.h"

using namespace Inkscape::Colors;
using namespace Inkscape::Colors::CMS;

static auto rgb = Profile::create_srgb();
static auto grb = Profile::create_from_uri(INKSCAPE_TESTS_DIR "/data/colors/SwappedRedAndGreen.icc");
static auto cmyk = Profile::create_from_uri(INKSCAPE_TESTS_DIR "/data/colors/default_cmyk.icc");

namespace {

// clang-format off
TEST(ColorsCmsTransformSurface, TransformFloatTypeIn)
{
    static const std::vector<float> img = {
        0.2, 0.1, 0.3, 1.0,   0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,   1.0, 1.0, 1.0, 0.2,
    };

    // Test FloatOut
    {
        auto tr1 = TransformSurface::create<float>(rgb, grb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false);

        std::vector<float> out(img.size(), 0.0);
        tr1.do_transform(2, 2, img.data(), out.data());

        EXPECT_TRUE(VectorIsNear(out, {
            0.1, 0.2, 0.3, 1.0,  0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.5,  1.0, 1.0, 1.0, 0.2,
        }, 0.001));
    }

    // Test IntOut
    {
        auto tr2 = TransformSurface::create<float, uint16_t>(rgb, grb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false, false);

        std::vector<uint16_t> out(img.size(), 0.0);
        tr2.do_transform(2, 2, img.data(), out.data());

        std::vector<uint16_t> ret = {
            6553, 13109, 19661, 65535,  0,     0,     0,     0,
            0,    0,     0,     32768,  65534, 65535, 65535, 13107
        };
        ASSERT_TRUE(VectorIsNear(out, ret, 2));
    }

}

TEST(ColorsCmsTransformSurface, TransformIntTypeIn)
{
    static const std::vector<uint16_t> img = {
        6553, 13109, 19661, 65535,  0,     0,     0,     0,
        0,    0,     0,     32768,  65534, 65535, 65535, 13107
    };

    // Test FloatOut
    {
        auto tr1 = TransformSurface::create<uint16_t, float>(rgb, grb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false, false);

        std::vector<float> out(img.size(), 0.0);
        tr1.do_transform(2, 2, img.data(), out.data());

        EXPECT_TRUE(VectorIsNear(out, {
            0.2, 0.1, 0.3, 1.0,   0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.5,   1.0, 1.0, 1.0, 0.2,
        }, 0.001));
    }

    // Test IntOut
    {
        auto tr2 = TransformSurface::create<uint16_t>(rgb, grb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false, false);

        std::vector<uint16_t> out(img.size(), 0.0);
        tr2.do_transform(2, 2, img.data(), out.data());

        std::vector<uint16_t> ret = {
            13108, 6549, 19661, 65535,  0,     0,     0,     0,
            0,     0,    0,     32768,  65534, 65534, 65535, 13107
        };
        ASSERT_EQ(out, ret);
    }
}

TEST(ColorsCmsTransformSurface, TransformPremultiplied)
{
    static const std::vector<float> img = {
        0.2, 0.1, 0.3, 0.5,   0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,   0.2, 0.2, 0.2, 0.2,
    };

    auto tr = TransformSurface::create<float>(rgb, grb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, true);

    std::vector<float> out(img.size(), 0.0);
    tr.do_transform(2, 2, img.data(), out.data());

    EXPECT_TRUE(VectorIsNear(out, {
        0.2, 0.4, 0.6, 0.5,  0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,  1.0, 1.0, 1.0, 0.2,
    }, 0.001));
}

TEST(ColorsCmsTreansformSurface, TransformNoAlphaButMultiplied)
{
    static std::vector<float> img = {
        0.2, 0.1, 0.3, 0.5,   0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,   0.2, 0.2, 0.2, 0.2,
    };
    std::vector<float> out(img.size() - 4, 0.0);

    auto tr = TransformSurface::create<float>(rgb, rgb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, true, false, true, false);
    tr.do_transform(2, 2, img.data(), out.data());

    EXPECT_TRUE(VectorIsNear(out, {
        0.4, 0.2, 0.6,  0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,  1.0, 1.0, 1.0,
    }, 0.001));
}

TEST(ColorsCmsTreansformSurface, TransformNoAlphaNotMultiplied)
{
    static std::vector<float> img = {
        0.2, 0.1, 0.3, 0.5,   0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.5,   0.2, 0.2, 0.2, 0.2,
    };
    std::vector<float> out(img.size() - 4, 0.0);

    auto tr = TransformSurface::create<float>(rgb, rgb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false, false, true, false);
    tr.do_transform(2, 2, img.data(), out.data());

    EXPECT_TRUE(VectorIsNear(out, {
        0.2, 0.1, 0.3,   0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,   0.2, 0.2, 0.2,
    }, 0.001));
}

TEST(ColorsCmsTransformSurface, TransformCMYKToRGB)
{
    static const std::vector<float> img = {
        1.0, 0.1, 0.3, 0.2, 0.5,   0.0, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 0.4, 0.5,   0.2, 0.2, 0.2, 0.2, 0.2,
    };

    auto tr = TransformSurface::create<float>(cmyk, rgb, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false);

    std::vector<float> out(img.size() - 4, 0.0);
    tr.do_transform(2, 2, img.data(), out.data());

    EXPECT_TRUE(VectorIsNear(out, {
       -1.053, 0.529, 0.6,   0.5,   0.172, 0.16,  0.163, 0,
        0.659, 0.667, 0.677, 0.5,   0.667, 0.644, 0.639, 0.2
    }, 0.001));

}

TEST(ColorsCmsTransformSurface, TransformRGBToCMYK)
{
    static const std::vector<float> img = {
        0,     0.529, 0.6,   0.5,   0.172, 0.16,  0.163, 0,
        0.659, 0.667, 0.677, 0.5,   0.667, 0.644, 0.639, 0.2
    };

    auto tr = TransformSurface::create<float>(rgb, cmyk, RenderingIntent::PERCEPTUAL, {}, RenderingIntent::AUTO, false);

    std::vector<float> out(img.size() + 4, 0.0);
    tr.do_transform(2, 2, img.data(), out.data());

    EXPECT_TRUE(VectorIsNear(out, {
        0.892, 0.329, 0.363, 0.037, 0.5,   0.686, 0.693, 0.653, 0.867, 0,
        0.365, 0.293, 0.287, 0.0,   0.5,   0.361, 0.329, 0.329, 0.003, 0.2
    }, 0.001));

}

TEST(ColorsCmsTransformSurface, TransformForProof)
{
    static const std::vector<float> img = {
        1.0, 0.1, 0.3, 0.5,   0.0, 0.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 0.5,   0.2, 0.2, 0.2, 0.2,
    };
    std::vector<float> out(img.size(), 0.0);

    {
        auto tr = TransformSurface::create<float>(rgb, rgb, RenderingIntent::ABSOLUTE_COLORIMETRIC, cmyk, RenderingIntent::ABSOLUTE_COLORIMETRIC, false);

        tr.do_transform(2, 2, img.data(), out.data());

        EXPECT_TRUE(VectorIsNear(out, {
           0.815,  0.176, 0.319, 0.5,   0.136, 0.134,  0.13, 0,
           0.813,  0.172, 0.176, 0.5,   0.204, 0.199, 0.197, 0.2
        }, 0.001));
    }
    {
        auto tr = TransformSurface::create<float>(rgb, rgb, RenderingIntent::RELATIVE_COLORIMETRIC, cmyk, RenderingIntent::RELATIVE_COLORIMETRIC, false);

        tr.do_transform(2, 2, img.data(), out.data());

        EXPECT_TRUE(VectorIsNear(out, {
           0.934,  0.226, 0.351, 0.5,   0.168, 0.165, 0.164, 0,
           0.932,  0.203, 0.219, 0.5,   0.264, 0.258, 0.255, 0.2
        }, 0.001));
    }
}

TEST(ColorsCmsTransformSurface, TransformWithGamutWarning)
{
    static const std::vector<uint16_t> img = {
        65535, 0,     65535, 65535,  0,     0,     0,     65535,
        0,     65535, 65535, 32768,  65534, 65535, 65535, 13107
    };

    auto tr = TransformSurface::create<uint16_t>(rgb, rgb, RenderingIntent::PERCEPTUAL, cmyk, RenderingIntent::PERCEPTUAL, false, true);
    tr.set_gamut_warn_color({1.0, 0.0, 0.0, 0.5});

    std::vector<uint16_t> out(img.size(), 0.0);
    tr.do_transform(2, 2, img.data(), out.data());

    std::vector<uint16_t> ret = {
        65535, 0,    0,     65535,  65535, 0,     0,     65535,
        65535, 0,    0,     32768,  65535, 65535, 65535, 13107
    };
    EXPECT_EQ(out, ret);
}
// clang-format on

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
