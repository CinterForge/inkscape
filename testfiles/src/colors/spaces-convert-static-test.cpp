// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for converting color data using static functions.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include "colors/color.h"
#include "colors/manager.h"
#include "colors/spaces/convert-static.h"

#include "../test-utils.h"

using namespace Inkscape::Colors;

namespace {

TEST(ColorSpaceStaticConvertTest, spaceToProfile)
{
    auto from_space = Manager::get().find(Space::Type::HSL);
    std::array<double, 3> hsl = {0.182, 0.870, 0.172};
    std::array<double, 3> rgb = {0.295, 0.322, 0.022};
    {
        std::array<double, 3> out;
        spaceToProfile<double, false>(std::dynamic_pointer_cast<Space::ProfileSpace<false>>(from_space))(hsl.data(), out.data());
        ASSERT_TRUE(VectorIsNear(out, rgb, 0.01));
    }
}

TEST(ColorSpaceStaticConvertTest, profileToSpace)
{
    auto from_space = Manager::get().find(Space::Type::HSL);
    std::array<double, 3> hsl = {0.182, 0.870, 0.172};
    std::array<double, 3> rgb = {0.295, 0.322, 0.022};
    {
        std::array<double, 3> out;
        profileToSpace<double, false>(std::dynamic_pointer_cast<Space::ProfileSpace<false>>(from_space))(rgb.data(), out.data());
        ASSERT_TRUE(VectorIsNear(out, hsl, 0.01));
    }
}

TEST(ColorSpaceStaticConvertTest, premultipliedAlpha)
{
    for (std::shared_ptr<Space::AnySpace> space : Manager::get().spaces(Space::Traits::Internal)) {
        auto alpha_in = space->getComponentCount();
        auto alpha_out = space->getProfile()->getSize();
        std::vector<double> in = {0.5, 0.5, 0.5, 0.5, 0.5};
        std::vector<double> out_a = {0.0, 0.0, 0.0, 0.0, 0.0};
        std::vector<double> out_b = {0.0, 0.0, 0.0, 0.0, 0.0};

        if (auto convertable = std::dynamic_pointer_cast<Space::ProfileSpace<false>>(space)) {
            // Get a value for non-premultiplied convertion
            Space::spaceToProfile<double, false>(convertable)(in.data(), out_a.data());
            // Alpha channel wasn't copied over
            EXPECT_EQ(out_a[alpha_out], 0.0) << space->getName();
            // Premultiply the alpha
            for (unsigned i = 0; i < alpha_in; i++) {
                in[i] *= in[alpha_in];
            }
            // Now ask for the same values as before
            Space::spaceToProfile<double, true>(convertable)(in.data(), out_b.data());
            // Alpha channel WAS copied over
            EXPECT_EQ(out_b[alpha_out], 0.5) << space->getName();
            out_a[alpha_out] = out_b[alpha_out]; // Harmonise for testing
            // They should be the same output as the premultiplication was undone
            EXPECT_TRUE(VectorIsNear(out_a, out_b, 0.01)) << space->getName();
        }
    }
}

TEST(ColorSpaceStaticConvertTest, correctConversion)
{
    for (std::shared_ptr<Space::AnySpace> space : Manager::get().spaces(Space::Traits::Internal)) {
        // Find direct profile with the same lcms2 color profile
        for (std::shared_ptr<Space::AnySpace> profile : Manager::get().spaces(Space::Traits::Internal)) {
            auto convertable = std::dynamic_pointer_cast<Space::ProfileSpace<false>>(space);
            auto direct = std::dynamic_pointer_cast<Space::ProfileSpace<true>>(profile);

            // These have the same profile space so converting between them yields the same results
            // as just running the spaceToProfile and profileToSpace static functions.
            if (convertable && direct && *convertable->getProfile() == *direct->getProfile()) {
                {
                    std::vector<double> in, out_b;
                    for (unsigned i = 0; i <= profile->getComponentCount(); i++) {
                        in.emplace_back(0.5);
                    }
                    for (unsigned i = 0; i <= space->getComponentCount(); i++) {
                        out_b.emplace_back(0.5);
                    }
                    auto out_a = Inkscape::Colors::Color(profile, in).converted(space)->getValues();
                    Space::profileToSpace<double, false>(convertable)(in.data(), out_b.data());
                    EXPECT_TRUE(VectorIsNear(out_a, out_b, 0.01)) << profile->getName() << " to " << space->getName();
                }

                {
                    std::vector<double> in, out_b;
                    for (unsigned i = 0; i <= space->getComponentCount(); i++) {
                        in.emplace_back(0.5);
                    }
                    for (unsigned i = 0; i <= profile->getComponentCount(); i++) {
                        out_b.emplace_back(0.5);
                    }
                    auto out_a = Inkscape::Colors::Color(space, in).converted(profile)->getValues();
                    Space::spaceToProfile<double, false>(convertable)(in.data(), out_b.data());
                    EXPECT_TRUE(VectorIsNear(out_a, out_b, 0.01)) << space->getName() << " to " << profile->getName();
                }
            }
        }
    }
}

TEST(ColorSpaceStaticConvertTest, premultiplyAlpha)
{
    for (std::shared_ptr<Space::AnySpace> space : Manager::get().spaces(Space::Traits::Internal)) {
        auto alpha_out = space->getComponentCount();
        std::vector<double> in = {0.5, 0.5, 0.5, 0.5, 0.5};
        std::vector<double> out_a = {0.0, 0.0, 0.0, 0.0, 0.0};
        std::vector<double> out_b = {0.0, 0.0, 0.0, 0.0, 0.0};

        if (auto convertable = std::dynamic_pointer_cast<Space::ProfileSpace<false>>(space)) {
            Space::profileToSpace<double, false>(convertable)(in.data(), out_a.data());
            Space::profileToSpace<double, true>(convertable)(in.data(), out_b.data());
            EXPECT_EQ(out_a[alpha_out], 0.0) << space->getName();
            EXPECT_EQ(out_b[alpha_out], 0.5) << space->getName();
            out_a[alpha_out] = out_b[alpha_out];
            // Unpremultiply the alpha
            for (unsigned i = 0; i < alpha_out; i++) {
                out_b[i] /= in[alpha_out];
            }
            // They should be the same output as the premultiplication was undone
            EXPECT_TRUE(VectorIsNear(out_a, out_b, 0.01)) << space->getName();
        }
    }
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
