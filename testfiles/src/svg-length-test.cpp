// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Test for SVG colors
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "svg/svg-length.h"
#include "svg/svg.h"
#include "util/units.h"

#include <glib.h>
#include <gtest/gtest.h>

namespace {

struct test_t
{
    char const *str;
    SVGLength::Unit unit;
    double value;
    double computed;
};

static std::vector<test_t> absolute_tests = {
    // clang-format off
    {"0",            SVGLength::NONE,   0        ,   0},
    {"  1  ",        SVGLength::NONE,   1        ,   1},
    {"\t2\n",        SVGLength::NONE,   2        ,   2},
    {"\n3    ",      SVGLength::NONE,   3        ,   3},
    {"1",            SVGLength::NONE,   1        ,   1},
    {"1.00001",      SVGLength::NONE,   1.00001  ,   1.00001},
    {"1px",          SVGLength::PX  ,   1        ,   1},
    {".1",           SVGLength::NONE,   0.1      ,   0.1},
    {".1px",         SVGLength::PX  ,   0.1      ,   0.1},
    {"1.",           SVGLength::NONE,   1        ,   1},
    {"1.px",         SVGLength::PX  ,   1        ,   1},
    {"100pt",        SVGLength::PT  , 100        ,  133.33333333333331},
    {"1e2pt",        SVGLength::PT  , 100        ,  133.33333333333331},
    {"3pc",          SVGLength::PC  ,   3        ,  48},
    {"-3.5pc",       SVGLength::PC  ,  -3.5      ,  -3.5*16.0},
    {"1.2345678mm",  SVGLength::MM  ,   1.2345678,  4.6660830236220479},
    {"123.45678cm",  SVGLength::CM  , 123.45678  ,  4666.0830236220481},
    {"73.162987in",  SVGLength::INCH,  73.162987 ,  7023.6467520000006},
    {"1.2345678912", SVGLength::NONE, 1.2345678912, 1.2345678912} // Precision of prasing checking, double
};

static std::vector<test_t> relative_tests = {
    {"123em", SVGLength::EM,      123, 123. *  7.},
    {"123ex", SVGLength::EX,      123, 123. * 13.},
    {"123%",  SVGLength::PERCENT, 1.23, 1.23 * 19.}
    // clang-format on
};

static std::vector<const char *> fail_single_tests = {
    "123 px",
    "123e",
    "123e+m",
    "123ec",
    "123pxt",
    "--123",
    "",
    "px",
    "1,",
    "1.0,,,",
    "inf",
    "+inf",
    "-inf",
    "nan"
};

// One successful value, then one failure
const std::vector<const char *> fail_list_tests = {
    "1 2rm",
    "4 ,",
    "\n3\n,,, 2",
    "3 trees wave goodbye,"
};

struct eq_test_t
{
    char const *a;
    char const *b;
    bool equal;
};

static std::vector<eq_test_t> eq_tests = {
    {"", "", true},
    {"1", "1", true},
    {"10mm", "10mm", true},
    {"20mm", "10mm", false},
};

} // namespace

TEST(SvgLengthTest, testRead)
{
    for (auto &test : absolute_tests) {
        SVGLength len;
        ASSERT_TRUE( len.read(test.str)) << "'" << test.str << "'";
        EXPECT_EQ( len.unit, test.unit) << test.str;
        EXPECT_EQ( len.value, test.value) << test.str;
        EXPECT_EQ( len.computed, test.computed) << test.str;
    }
    for (auto &test : relative_tests) {
        SVGLength len;
        ASSERT_TRUE( len.read(test.str)) << test.str;
        len.update(7, 13, 19);
        EXPECT_EQ( len.unit, test.unit) << test.str;
        EXPECT_EQ( len.value, test.value) << test.str;
        EXPECT_EQ( len.computed, test.computed) << test.str;
    }
    for (auto test : fail_single_tests) {
        SVGLength len;
        ASSERT_TRUE( !len.read(test)) << test;
    }
}

TEST(SvgLengthTest, testReadOrUnset)
{
    for (auto &test : absolute_tests) {
        SVGLength len;
        len.readOrUnset(test.str);
        ASSERT_EQ( len.unit, test.unit) << test.str;
        ASSERT_EQ( len.value, test.value) << test.str;
        ASSERT_EQ( len.computed, test.computed) << test.str;
    }
    for (auto &test : relative_tests) {
        SVGLength len;
        len.readOrUnset(test.str);
        len.update(7, 13, 19);
        ASSERT_EQ( len.unit, test.unit) << test.str;
        ASSERT_EQ( len.value, test.value) << test.str;
        ASSERT_EQ( len.computed, test.computed) << test.str;
    }
    for (auto test : fail_single_tests) {
        SVGLength len;
        len.readOrUnset(test, SVGLength::INCH, 123, 456);
        ASSERT_EQ( len.unit, SVGLength::INCH) << test;
        ASSERT_EQ( len.value, 123) << test;
        ASSERT_EQ( len.computed, 456) << test;
    }
}

TEST(SvgLengthTest, testReadAbsolute)
{
    for (auto &test : absolute_tests) {
        SVGLength len;
        ASSERT_TRUE( len.readAbsolute(test.str)) << test.str;
        EXPECT_EQ( len.unit, test.unit) << test.str;
        EXPECT_EQ( len.value, test.value) << test.str;
        EXPECT_EQ( len.computed, test.computed) << test.str;
    }
    for (auto &test : relative_tests) {
        SVGLength len;
        EXPECT_TRUE( !len.readAbsolute(test.str)) << test.str;
    }
    for (auto test : fail_single_tests) {
        SVGLength len;
        ASSERT_TRUE( !len.readAbsolute(test)) << test;
    }
}

TEST(SvgLengthTest, testReadLocale)
{
    std::locale was;
    try {
        was = std::locale::global(std::locale("de_DE.UTF8"));
    } catch (std::runtime_error &e) {
        GTEST_SKIP() << "Skipping all locale test, locale not available";
    }
    for (auto &test : absolute_tests) {
        SVGLength len;
        ASSERT_TRUE( len.read(test.str)) << test.str;
        EXPECT_EQ( len.unit, test.unit) << test.str;
        EXPECT_EQ( len.value, test.value) << test.str;
        EXPECT_EQ( len.computed, test.computed) << test.str;
    }
    std::locale::global(was);
}

TEST(SvgLengthTest, testToFromString)
{
    double scale = 96.0 / 25.4;
    SVGLength len;
    ASSERT_TRUE(len.fromString("10", "mm", scale));
    EXPECT_EQ(len.unit, SVGLength::NONE);
    EXPECT_EQ(len.write(), "10");
    EXPECT_EQ(len.toString("mm", scale), "10mm");
    EXPECT_EQ(len.toString("in", scale), "0.3937007874in");
    EXPECT_EQ(len.toString("", scale), "37.79527559");
}

TEST(SvgLengthTest, testEquality)
{
    for (auto &test : eq_tests) {
        SVGLength len_a;
        SVGLength len_b;
        len_a.read(test.a);
        len_b.read(test.b);
        if (test.equal) {
            ASSERT_TRUE(len_a == len_b) << test.a << " == " << test.b;
        } else {
            ASSERT_TRUE(len_a != len_b) << test.a << " != " << test.b;
        }
    }
}

TEST(SvgLengthTest, testStringsAreValidSVG)
{
    static auto const &unit_table = Inkscape::Util::UnitTable::get();
    gchar const *valid[] = {"", "em", "ex", "px", "pt", "pc", "cm", "mm", "in", "%"};
    std::set<std::string> validStrings(valid, valid + G_N_ELEMENTS(valid));
    for (int i = (static_cast<int>(SVGLength::NONE) + 1); i <= static_cast<int>(SVGLength::LAST_UNIT); i++) {
        Inkscape::Util::Unit const *unit = unit_table.getUnit(static_cast<SVGLength::Unit>(i));
        ASSERT_TRUE( validStrings.find(unit->abbr) != validStrings.end()) << i;
    }
}

TEST(SvgLengthTest, testValidSVGStringsSupported)
{
    static auto const &unit_table = Inkscape::Util::UnitTable::get();
    // Note that "px" is omitted from the list, as it will be assumed to be so if not explicitly set.
    gchar const *valid[] = {"em", "ex", "pt", "pc", "cm", "mm", "in", "%"};
    std::set<std::string> validStrings(valid, valid + G_N_ELEMENTS(valid));
    for (int i = (static_cast<int>(SVGLength::NONE) + 1); i <= static_cast<int>(SVGLength::LAST_UNIT); i++) {
        Inkscape::Util::Unit const *unit = unit_table.getUnit(static_cast<SVGLength::Unit>(i));
        std::set<std::string>::iterator iter = validStrings.find(unit->abbr);
        if (iter != validStrings.end()) {
            validStrings.erase(iter);
        }
    }
    ASSERT_EQ(validStrings.size(), 0u) << validStrings.size();
}

TEST(SvgLengthTest, testPlaces)
{
    struct testd_t
    {
        char const *str;
        double val;
        int prec;
        int minexp;
    };

    testd_t const precTests[] = {
        {"760", 761.92918978947023, 2, -8},
        {"761.9", 761.92918978947023, 4, -8},
    };

    for (size_t i = 0; i < G_N_ELEMENTS(precTests); i++) {
        std::string buf;
        buf.append(sp_svg_number_write_de(precTests[i].val, precTests[i].prec, precTests[i].minexp));
        unsigned int retval = buf.length();
        ASSERT_EQ( retval, strlen(precTests[i].str)) << "Number of chars written";
        ASSERT_EQ( std::string(buf), std::string(precTests[i].str)) << "Numeric string written";
    }
}

TEST(SvgLengthTest, testList)
{
    auto items = sp_svg_length_list_read("56px \t-4in, 99.73738 9% 34.0em\n2e+2pt \n  3e-4px ");
    ASSERT_EQ(items.size(), 7);
    EXPECT_TRUE(items[0]);
    EXPECT_EQ(items[0].value, 56);
    EXPECT_EQ(items[0].unit, SVGLength::PX);
    EXPECT_EQ(items[1].value, -4);
    EXPECT_EQ(items[1].unit, SVGLength::INCH);
    EXPECT_EQ(items[2].value, 99.73738);
    EXPECT_EQ(items[2].unit, SVGLength::NONE);
    EXPECT_EQ(items[3].value, 0.09);
    EXPECT_EQ(items[3].unit, SVGLength::PERCENT);
    EXPECT_EQ(items[4].value, 34);
    EXPECT_EQ(items[4].unit, SVGLength::EM);
    EXPECT_EQ(items[5].value, 200);
    EXPECT_EQ(items[5].unit, SVGLength::PT);
    EXPECT_EQ(items[6].value, 0.0003);
    EXPECT_EQ(items[6].unit, SVGLength::PX);
}

TEST(SvgLengthTest, testListFailures)
{
    for (auto &test : fail_list_tests) {
        auto items = sp_svg_length_list_read(test);
        std::string debug;
        for (auto &item : items) {
            debug += "\n  * " + item.toString("px", 1.0);
        }
        EXPECT_EQ(items.size(), 1) << test << debug.c_str();
    }
}

// TODO: More tests

// vim: filetype=cpp:expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :
