// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Test the ColorProfile object
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/color-profile.h"

#include <gtest/gtest.h>

#include "colors/cms/system.h"
#include "document.h"
#include "inkscape.h"
#include "object/uri.h"

static std::string icc_dir = INKSCAPE_TESTS_DIR "/data/colors/";
static std::string svg_objs_file = INKSCAPE_TESTS_DIR "/data/colors/cms-in-objs.svg";
static std::string cmyk_profile = INKSCAPE_TESTS_DIR "/data/colors/default_cmyk.icc";

using namespace Inkscape;
using namespace Inkscape::Colors;

namespace Inkscape {

bool operator==(URI const *a, URI const &b)
{
    return a != nullptr && a->str() == b.str();
}

void PrintTo(const URI &value, std::ostream *os)
{
    *os << "URI(\"" << value.str() << "\")";
}

void PrintTo(const URI *value, std::ostream *os)
{
    if (!value) {
        *os << "NULL";
        return;
    }
    PrintTo(*value, os);
}

} // namespace Inkscape

namespace {

class ObjectColorProfileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup inkscape dependency
        Inkscape::Application::create(false);

        // Allow lookup by ID and name with test icc profiles
        auto &cms = Inkscape::Colors::CMS::System::get();
        cms.clearDirectoryPaths();
        cms.addDirectoryPath(icc_dir, false);
        cms.refreshProfiles();

        // Load the test svg file with a bunch of icc profiles
        doc = SPDocument::createNewDoc(getDocFilename().c_str());
    }
    std::unique_ptr<SPDocument> doc;

    virtual std::string const &getDocFilename() const { return svg_objs_file; }
};

TEST(ObjectColorProfileTestCheck, sanityCheck)
{
    // Sanity check for the URI matcher
    auto uri = URI::from_native_filename("/tmp/file");
    EXPECT_EQ(uri.str(), "file:///tmp/file");
    EXPECT_EQ(&uri, URI::from_native_filename("/tmp/file"));
    EXPECT_NE(&uri, URI::from_native_filename("/tmp/file2"));
    EXPECT_NE(static_cast<URI const *>(nullptr), uri);
}

TEST_F(ObjectColorProfileTest, attributesRead)
{
    auto *cp1 = cast<ColorProfile>(doc->getObjectById("cp1"));
    ASSERT_NE(cp1, nullptr);
    EXPECT_EQ(cp1->getName(), "grb");
    EXPECT_EQ(cp1->getLocalProfileId(), "f9eda5a42a222a28f0adb82a938eeb0e");
    EXPECT_EQ(cp1->getUri(), nullptr);
    EXPECT_EQ(cp1->getRenderingIntent(), RenderingIntent::UNKNOWN);

    auto *cp2 = cast<ColorProfile>(doc->getObjectById("cp2"));
    ASSERT_NE(cp2, nullptr);
    EXPECT_EQ(cp2->getName(), "cmyk-rcm");
    EXPECT_EQ(cp2->getLocalProfileId(), "");
    EXPECT_EQ(cp2->getUri(), URI::from_native_filename(cmyk_profile.c_str()));
    EXPECT_EQ(cp2->getRenderingIntent(), RenderingIntent::RELATIVE_COLORIMETRIC);

    auto *cp3 = cast<ColorProfile>(doc->getObjectById("cp3"));
    ASSERT_NE(cp3, nullptr);
    EXPECT_EQ(cp3->getName(), "cmyk-acm");
    EXPECT_EQ(cp3->getLocalProfileId(), "");
    EXPECT_EQ(cp3->getUri(), URI::from_native_filename(cmyk_profile.c_str()));
    EXPECT_EQ(cp3->getRenderingIntent(), RenderingIntent::ABSOLUTE_COLORIMETRIC);
}

TEST_F(ObjectColorProfileTest, attributesWrite)
{
    auto *cp1 = cast<ColorProfile>(doc->getObjectById("cp1"));
    ASSERT_NE(cp1, nullptr);
    cp1->updateRepr(SP_OBJECT_WRITE_ALL);
    EXPECT_STREQ(cp1->getRepr()->attribute("name"), "grb");
    EXPECT_STREQ(cp1->getRepr()->attribute("local"), "f9eda5a42a222a28f0adb82a938eeb0e");
    EXPECT_STREQ(cp1->getRepr()->attribute("xlink:href"), nullptr);
    EXPECT_STREQ(cp1->getRepr()->attribute("rendering-intent"), nullptr);

    auto *cp2 = cast<ColorProfile>(doc->getObjectById("cp2"));
    ASSERT_NE(cp2, nullptr);
    cp2->updateRepr(SP_OBJECT_WRITE_ALL);
    EXPECT_STREQ(cp2->getRepr()->attribute("name"), "cmyk-rcm");
    EXPECT_STREQ(cp2->getRepr()->attribute("local"), nullptr);
    EXPECT_STREQ(cp2->getRepr()->attribute("xlink:href"), "default_cmyk.icc");
    EXPECT_STREQ(cp2->getRepr()->attribute("rendering-intent"), "relative-colorimetric");

    auto *cp3 = cast<ColorProfile>(doc->getObjectById("cp3"));
    ASSERT_NE(cp3, nullptr);
    cp3->updateRepr(SP_OBJECT_WRITE_ALL);
    EXPECT_STREQ(cp3->getRepr()->attribute("name"), "cmyk-acm");
    EXPECT_STREQ(cp3->getRepr()->attribute("local"), nullptr);
    EXPECT_STREQ(cp3->getRepr()->attribute("xlink:href"), "default_cmyk.icc");
    EXPECT_STREQ(cp3->getRepr()->attribute("rendering-intent"), "absolute-colorimetric");
}

TEST_F(ObjectColorProfileTest, createFromProfileAttributes)
{
    auto &cms = Inkscape::Colors::CMS::System::get();
    auto profile = cms.getProfile(cmyk_profile);
    ASSERT_NE(profile, nullptr);

    auto newCp = ColorProfile::createFromProfile(doc.get(), *profile, "new-cmyk", ColorProfileStorage::HREF_FILE,
                                                 RenderingIntent::AUTO);
    ASSERT_NE(newCp, nullptr);
    EXPECT_EQ(newCp->getName(), "new-cmyk");
    EXPECT_EQ(newCp->getLocalProfileId(), "");
    EXPECT_EQ(newCp->getUri(), URI::from_native_filename(cmyk_profile.c_str()));
    EXPECT_EQ(newCp->getRenderingIntent(), RenderingIntent::AUTO);

    EXPECT_STREQ(newCp->getRepr()->attribute("name"), "new-cmyk");
    EXPECT_STREQ(newCp->getRepr()->attribute("local"), nullptr);
    EXPECT_STREQ(newCp->getRepr()->attribute("xlink:href"), "default_cmyk.icc");
    EXPECT_STREQ(newCp->getRepr()->attribute("rendering-intent"), "auto");
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
