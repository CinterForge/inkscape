// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "libnrtype/font-instance.h"
#include "libnrtype/font-factory.h"

#include <iostream>
#include <glibmm/init.h>
#include <gtest/gtest.h>

namespace {

class FontInstanceTest : public ::testing::Test
{
protected:
    FontInstanceTest()
    {
        Glib::init();
    }

    void SetUp() override {
        font = FontFactory::get().FaceFromDescr("FreeSans", "Normal");
    }   
    void TearDown() override {
        font = {};
    }

    Inkscape::Util::Statics statics;
    std::shared_ptr<FontInstance> font;
};

TEST_F(FontInstanceTest, MapUnicodeChar)
{
    ASSERT_EQ(font->MapUnicodeChar('i'), 76);
}

TEST_F(FontInstanceTest, PathVector)
{
    Geom::PathVector vec = *font->PathVector(76);
    std::ostringstream o;
    for (unsigned i = 0; i < vec.size(); i++) {
        o << (o.str().empty() ? "" : " ") << "M ";
        for (auto &p : vec[i]) {
            o << (int)(p.initialPoint()[Geom::X] * 100) << ","
              << (int)(p.initialPoint()[Geom::Y] * 100) << " ";
        }
        o << "Z";
    }
    // This is an 'i' from the FreeSans font, upside down.
    ASSERT_EQ(o.str(), "M 15,52 15,0 6,0 6,52 Z M 15,72 15,62 6,62 6,72 Z");
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
