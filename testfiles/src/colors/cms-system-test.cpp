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
#include "colors/cms/system.h"
#include "preferences.h"

static std::string icc_dir = INKSCAPE_TESTS_DIR "/data/colors/";
static std::string grb_profile = INKSCAPE_TESTS_DIR "/data/colors/SwappedRedAndGreen.icc";
static std::string display_profile = INKSCAPE_TESTS_DIR "/data/colors/display.icc";

using namespace Inkscape::Colors;
using namespace Inkscape::Colors::CMS;

namespace {

// ================= CMS::System ================= //

class ColorCmsSystem : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cms = &CMS::System::get();
        cms->clearDirectoryPaths();
        cms->addDirectoryPath(icc_dir, false);
        cms->refreshProfiles();

        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setString("/options/displayprofile/uri", display_profile);
        prefs->setBool("/options/displayprofile/enabled", true);
    }
    void TearDown() override {}
    Inkscape::Colors::CMS::System *cms = nullptr;
};

TEST_F(ColorCmsSystem, getDirectoryPaths)
{
    ASSERT_EQ(cms->getDirectoryPaths().size(), 1);
    ASSERT_EQ(cms->getDirectoryPaths()[0].first, icc_dir);
}

TEST_F(ColorCmsSystem, addDirectoryPath)
{
    cms->clearDirectoryPaths();
    cms->addDirectoryPath("nope", false);
    cms->addDirectoryPath("yep", true);
    ASSERT_EQ(cms->getDirectoryPaths().size(), 2);
    ASSERT_EQ(cms->getDirectoryPaths()[0].first, "nope");
    ASSERT_EQ(cms->getDirectoryPaths()[1].first, "yep");
}

TEST_F(ColorCmsSystem, clearDirectoryPaths)
{
    cms->clearDirectoryPaths();
    ASSERT_GE(cms->getDirectoryPaths().size(), 2);
}

TEST_F(ColorCmsSystem, getProfiles)
{
    auto profiles = cms->getProfiles();
    ASSERT_EQ(profiles.size(), 3);

    ASSERT_EQ(profiles[0]->getName(), "Artifex CMYK SWOP Profile");
    ASSERT_EQ(profiles[1]->getName(), "C.icc");
    ASSERT_EQ(profiles[2]->getName(), "Swapped Red and Green");
}

TEST_F(ColorCmsSystem, getProfileByName)
{
    auto profile = cms->getProfile("Swapped Red and Green");
    ASSERT_TRUE(profile);
    ASSERT_EQ(profile->getPath(), grb_profile);
}

TEST_F(ColorCmsSystem, getProfileByID)
{
    auto profile = cms->getProfile("f9eda5a42a222a28f0adb82a938eeb0e");
    ASSERT_TRUE(profile);
    ASSERT_EQ(profile->getName(), "Swapped Red and Green");
}

TEST_F(ColorCmsSystem, getProfileByPath)
{
    auto profile = cms->getProfile(grb_profile);
    ASSERT_TRUE(profile);
    ASSERT_EQ(profile->getId(), "f9eda5a42a222a28f0adb82a938eeb0e");
}

TEST_F(ColorCmsSystem, getDisplayProfiles)
{
    auto profiles = cms->getDisplayProfiles();
    ASSERT_EQ(profiles.size(), 1);
    ASSERT_EQ(profiles[0]->getName(), "C.icc");
}

TEST_F(ColorCmsSystem, getDisplayProfile)
{
    bool updated = false;
    auto profile = cms->getDisplayProfile(updated);
    ASSERT_TRUE(updated);
    ASSERT_TRUE(profile);
    ASSERT_EQ(profile->getName(), "C.icc");
}

TEST_F(ColorCmsSystem, getOutputProfiles)
{
    auto profiles = cms->getOutputProfiles();
    ASSERT_EQ(profiles.size(), 1);
    ASSERT_EQ(profiles[0]->getName(), "Artifex CMYK SWOP Profile");
}

TEST_F(ColorCmsSystem, getLinearRgbProfile)
{
    auto linear = CMS::Profile::create_linearrgb();
    EXPECT_EQ(linear->getName(), "linearRGB identity with D65");
}

TEST_F(ColorCmsSystem, refreshProfiles)
{
    ASSERT_EQ(cms->getDirectoryPaths().size(), 1);
    cms->clearDirectoryPaths();
    cms->refreshProfiles();
    ASSERT_GE(cms->getDirectoryPaths().size(), 5);
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
