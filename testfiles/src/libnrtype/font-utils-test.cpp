// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit test for the font utils
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "libnrtype/font-utils.h"
#include <gtest/gtest.h>
#include "../test-utils.h"

namespace {

struct data : traced_data
{
    const Glib::ustring in;
    const Glib::ustring out;
};

void PrintTo(const data &obj, std::ostream *oo)
{
    *oo << "'" << obj.in << "'";
};

class CanonizeFontspec : public testing::TestWithParam<data>
{
};

TEST_P(CanonizeFontspec, canonize_fontspec)
{
    data test = GetParam();
    EXPECT_EQ(Inkscape::canonize_fontspec(test.in), test.out);
};

// clang-format off
INSTANTIATE_TEST_SUITE_P(FontUtilsTest, CanonizeFontspec, testing::Values(
    _P(data, "Thin @wght=100",               "Thin"),
    _P(data, "Ultra-Light @wght=200",        "Ultra-Light"),
    _P(data, "Light @wght=300",              "Light"),
    _P(data, "Medium @wght=500",             "Medium"),
    _P(data, "Semi-Bold @wght=600",          "Semi-Bold"),
    _P(data, "Bold @wght=700",               "Bold"),
    _P(data, "Ultra-Bold @wght=800",         "Ultra-Bold"),
    _P(data, "Heavy @wght=900",              "Heavy"),
    _P(data, "Thin Italic @wght=100",        "Thin Italic"),
    _P(data, "Ultra-Light Italic @wght=200", "Ultra-Light Italic"),
    _P(data, "Light Italic @wght=300",       "Light Italic"),
    _P(data, "Medium Italic @wght=500",      "Medium Italic"),
    _P(data, "Semi-Bold Italic @wght=600",   "Semi-Bold Italic"),
    _P(data, "Bold Italic @wght=700",        "Bold Italic"),
    _P(data, "Ultra-Bold Italic @wght=800",  "Ultra-Bold Italic"),
    _P(data, "Heavy Italic @wght=900",       "Heavy Italic"),

    _P(data, "Cascadia Code, Ultra-Light @wght=200",        "Cascadia Code Ultra-Light"),
    _P(data, "Cascadia Code, Ultra-Light Italic @wght=200", "Cascadia Code Ultra-Light Italic"),
    _P(data, "Cascadia Code, Light @wght=300",              "Cascadia Code Light"),
    _P(data, "Cascadia Code, Light Italic @wght=300",       "Cascadia Code Light Italic"),
    _P(data, "Cascadia Code, Normal",                       "Cascadia Code"),
    _P(data, "Cascadia Code, Italic",                       "Cascadia Code Italic"),
    _P(data, "Cascadia Code, Semi-Bold @wght=600",          "Cascadia Code Semi-Bold"),
    _P(data, "Cascadia Code, Semi-Bold Italic @wght=600",   "Cascadia Code Semi-Bold Italic"),
    _P(data, "Cascadia Code, Bold @wght=700",               "Cascadia Code Bold"),
    _P(data, "Cascadia Code, Bold Italic @wght=700",        "Cascadia Code Bold Italic"),
    _P(data, "Cascadia Code, Normal @wght=450",             "Cascadia Code weight=450"),
    _P(data, "Cascadia Code, Italic @wght=450",             "Cascadia Code weight=450 Italic"),

    _P(data, "Decovar Alpha, Normal  @WMX2=1000",           "Decovar Alpha @WMX2=1000"),
    _P(data, "Decovar Alpha, Normal @WMX2=1000",            "Decovar Alpha @WMX2=1000"),
    _P(data, "Decovar Alpha, Normal  @TRMG=1000",           "Decovar Alpha @TRMG=1000"),
    _P(data, "Decovar Alpha, Normal  @TRMG=1000,WMX2=1000", "Decovar Alpha @TRMG=1000,WMX2=1000"),
    _P(data, "Decovar Alpha, Normal  @WMX2=1000,TRMG=1000", "Decovar Alpha @TRMG=1000,WMX2=1000"),

    _P(data, "Nabla, Normal @EDPT=70,EHLT=20",              "Nabla @EDPT=70,EHLT=20"),
    _P(data, "Nabla, Normal @EHLT=20,EDPT=70",              "Nabla @EDPT=70,EHLT=20"),

    _P(data, "Red Hat Display, weight=596",                 "Red Hat Display weight=596"),
    _P(data, "Red Hat Display, weight=596 Italic",          "Red Hat Display weight=596 Italic"),

    _P(data, "Rocher Color, Normal",                        "Rocher Color"),
    _P(data, "Rocher Color, Normal  @BVEL=0",               "Rocher Color @BVEL=0"),
    _P(data, "Rocher Color, Normal  @SHDW=0",               "Rocher Color @SHDW=0"),
    _P(data, "Rocher Color, Normal  @SHDW=10,BVEL=20",      "Rocher Color @BVEL=20,SHDW=10"),

    _P(data, "Vazirmatn, Normal @wght=450",                 "Vazirmatn weight=450")
));
// clang-format on

struct data2 : traced_data
{
    const char* in;
    std::map<std::string, std::string> out;
};

void PrintTo(const data2 &obj, std::ostream *oo)
{
    *oo << "'" << obj.in << "'";
};

class ParseVariations : public testing::TestWithParam<data2>
{
};

TEST_P(ParseVariations, parse_variations)
{
    data2 test = GetParam();
    EXPECT_EQ(Inkscape::parse_variations(test.in), test.out);
};

INSTANTIATE_TEST_SUITE_P(FontUtilsTest, ParseVariations, testing::Values(
    _P(data2, "@aaaa=100,bbbb=200,cccc=1.5", {{"aaaa", "100"}, {"bbbb", "200"}, {"cccc", "1.5"}})
));

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
