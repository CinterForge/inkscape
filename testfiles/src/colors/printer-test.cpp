// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for printer base class
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/printer.h"

#include <gtest/gtest.h>

using namespace Inkscape::Colors;

namespace {

TEST(ColorsPrinter, PrinterBasics)
{
    std::stringstream number;
    try {
        number.imbue(std::locale("C"));
    } catch (std::runtime_error &e) {
        ASSERT_TRUE(false) << "Locale 'C' not available for testing";
    }

    number.precision(12);
    number << std::fixed << 3.1415;

    auto oo = CssPrinter(3, "prefix", "", " ");
    oo << (int)1 << (double)3.3 << (double)0.0;
    ASSERT_EQ(std::string(oo), "prefix(1 3.3 0)");
}

TEST(ColorsPrinter, PrinterLocale)
{
    std::locale was;
    try {
        was = std::locale::global(std::locale("de_DE.utf8"));
    } catch (std::runtime_error &e) {
        GTEST_SKIP() << "Skipping all locale test, locale not available";
    }

    auto oo = CssPrinter(3, "prefix", "", " ");
    oo << 1.2 << 3.3 << 0.0234;
    ASSERT_EQ(std::string(oo), "prefix(1.2 3.3 0.023)");
    std::locale::global(was);
}

TEST(ColorsPrinter, LegacyPrinter)
{
    auto oo = CssLegacyPrinter(3, "leg", false);
    oo << 1.0 << 3.3 << 0.0;
    ASSERT_EQ(std::string(oo), "leg(1, 3.3, 0)");

    oo = CssLegacyPrinter(3, "leg", true);
    oo << 1.2 << 3.3 << 0.0 << 0.5;
    ASSERT_EQ(std::string(oo), "lega(1.2, 3.3, 0, 0.5)");
}

TEST(ColorsPrinter, FuncPrinter)
{
    auto oo = CssFuncPrinter(4, "func");
    oo << 1.0 << 3.3 << 0.0 << 1.2;
    ASSERT_EQ(std::string(oo), "func(1 3.3 0 1.2)");

    oo = CssFuncPrinter(4, "func");
    oo << 1.0 << 3.3 << 0.0 << 1.2 << 0.5;
    ASSERT_EQ(std::string(oo), "func(1 3.3 0 1.2 / 50%)");
}

TEST(ColorsPrinter, ColorPrinter)
{
    auto oo = CssColorPrinter(3, "ident");
    oo << 1.0 << 3.3 << 0.0 << 0.5;
    ASSERT_EQ(std::string(oo), "color(ident 1 3.3 0 / 50%)");
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
