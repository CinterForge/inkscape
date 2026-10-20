// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Shared test header for testing color spaces
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cmath>
#include <gtest/gtest.h>

#include "../test-utils.h"

#include "colors/color.h"
#include "colors/manager.h"
#include "colors/spaces/base.h"
#include "colors/spaces/components.h"

using namespace Inkscape::Colors;

namespace {

/**
 * Test that a color space actually exists, to catch test writing mistakes instead of crashing.
 */
void testSpaceName(Space::Type const &type)
{
    auto &manager = Manager::get();
    ASSERT_TRUE(manager.find(type)) << "Unknown Color Space";
}

std::string getSpaceName(Space::Type const &type)
{
    return Manager::get().find(type)->getName();
}

// Allow numbers to be printed as hex in failures
// see https://github.com/google/googletest/issues/222
class Hex
{
public:
    explicit Hex(unsigned int n)
        : _number(n)
    {}
    operator unsigned int() { return _number; }
    bool operator==(Hex const &other) const { return other._number == _number; }
    bool operator!=(Hex const &other) const { return other._number != _number; }
    bool operator==(unsigned int const &other) const { return other == _number; }
    bool operator!=(unsigned int const &other) const { return other != _number; }
    unsigned int _number;
};
void PrintTo(const Hex &obj, std::ostream *oo)
{
    oo->imbue(std::locale("C"));
    *oo << "0x" << std::setfill('0') << std::setw(8) << std::hex << obj._number << "'";
}

/* ===== In test ===== */

struct in : traced_data
{
    const std::string val;
    const std::vector<double> out;
    const unsigned int rgba;
};
void PrintTo(const in &obj, std::ostream *oo)
{
    *oo << "'" << obj.val << "'";
}

class fromString : public testing::TestWithParam<in>
{
};

TEST_P(fromString, hasValues)
{
    in test = GetParam();
    auto color = Color::parse(test.val);
    ASSERT_TRUE(color);
    auto scope = test.enable_scope();
    EXPECT_TRUE(VectorIsNear(color->getValues(), test.out, 0.001));
}

TEST_P(fromString, hasRGBA)
{
    in test = GetParam();
    auto scope = test.enable_scope();
    auto color = Color::parse(test.val);
    ASSERT_TRUE(color);
    EXPECT_EQ(Hex(color->toRGBA(true)), Hex(test.rgba));
}

class badColorString : public testing::TestWithParam<std::string>
{
};

TEST_P(badColorString, returnsNone)
{
    EXPECT_FALSE(Color::parse(GetParam()));
}

/* ===== Out test ===== */

struct out : traced_data
{
    const Space::Type space;
    const std::vector<double> val;
    const std::string out;
    const bool opacity = true;
};
void PrintTo(const out &obj, std::ostream *oo)
{
    *oo << "'" << obj.out << "'";
}
class toString : public testing::TestWithParam<out>
{
};
TEST_P(toString, hasValue)
{
    out test = GetParam();
    auto scope = test.enable_scope();
    testSpaceName(test.space);
    EXPECT_EQ(Color(test.space, test.val).toString(test.opacity), test.out);
}

/* ====== Convert test ===== */

struct norm
{
    double count = 0.0;
    std::vector<double> min;
    std::vector<double> max;
};

struct inb : traced_data
{
    const Space::Type space_in;
    const std::vector<double> in;
    const Space::Type space_out;
    const std::vector<double> out;
    bool both_directions = true;

    Color do_conversion(bool inplace, norm *notnorm = nullptr) const
    {
        auto result = Color(space_in, in);
        if (inplace) {
            result.convert(space_out);
            count_notnorm(result, notnorm);
            return result;
        }
        if (auto color = result.converted(space_out)) {
            count_notnorm(*color, notnorm);
            return *color;
        }
        throw ColorError("Bad conversion in test");
    }

    void count_notnorm(Color const &c, norm *out = nullptr) const
    {
        if (!out)
            return;

        for (unsigned int i = 0; i < c.size(); i++) {
            double v = c[i];

            // Count the out of bounds results from conversions
            if (v < 0.0)
                out->count += std::abs(v);
            else if (v > 1.0)
                out->count += (v - 1.0);

            // Record the min and max ranges
            if (out->min.size() <= i)
                out->min.emplace_back(0.0);
            if (out->max.size() <= i)
                out->max.emplace_back(0.0);

            out->min[i] = std::min(out->min[i], v);
            out->max[i] = std::max(out->max[i], v);
        }
    }

    ::testing::AssertionResult forward_test(bool inplace, norm *notnorm = nullptr) const
    {
        auto result = do_conversion(inplace, notnorm);
        return VectorIsNear(result.getValues(), out, 0.005);
    }

    ::testing::AssertionResult backward_test(bool inplace, norm *notnorm = nullptr) const
    {
        return inb{_file, _line, space_out, out, space_in, in}.forward_test(inplace, notnorm);
    }

    // Send the results back to be tested for a pass-through test
    ::testing::AssertionResult through_test(bool inplace, norm *notnorm = nullptr) const
    {
        auto result = do_conversion(inplace, notnorm);
        return inb{_file, _line, space_out, result.getValues(), space_in, in}.forward_test(inplace, notnorm);
    }
};
void PrintTo(const inb &obj, std::ostream *oo)
{
    *oo << (int)obj.space_in << print_values(obj.in);
    *oo << "<->";
    *oo << (int)obj.space_out << print_values(obj.out);
}

class convertColorSpace : public testing::TestWithParam<inb>
{
};
TEST_P(convertColorSpace, copy)
{
    auto test = GetParam();
    auto scope = test.enable_scope();
    testSpaceName(test.space_in);
    testSpaceName(test.space_out);
    auto in_name = getSpaceName(test.space_in);
    auto out_name = getSpaceName(test.space_out);
    EXPECT_TRUE(test.forward_test(false)) << " " << in_name << " copy to " << out_name;
    if (test.both_directions) {
        EXPECT_TRUE(test.backward_test(false)) << " " << in_name << " copy from " << out_name;
    }
}
TEST_P(convertColorSpace, inPlace)
{
    auto test = GetParam();
    auto scope = test.enable_scope();
    testSpaceName(test.space_in);
    testSpaceName(test.space_out);
    auto in_name = getSpaceName(test.space_in);
    auto out_name = getSpaceName(test.space_out);
    EXPECT_TRUE(test.forward_test(true)) << " in place " << in_name << " to " << out_name;
    if (test.both_directions) {
        EXPECT_TRUE(test.backward_test(true)) << " in place " << in_name << " from " << out_name;
    }
}

/**
 * Create many random tests of the conversion functions, outputs and fed to inputs
 * to guarentee stability in both directions.
 *
 * @arg from_func - A conversion function in one direction
 * @arg to_func - The reverse function
 * @arg count - The number of tests to create
 */
::testing::AssertionResult RandomPassFunc(std::function<void(double const *, double *)> from_func,
                                          std::function<void(double const *, double *)> to_func, unsigned count = 1000)
{
    (void)&RandomPassFunc; // Avoid compile warning
    std::srand(13375336);  // We always seed for tests' repeatability

    std::vector<double> range = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0};

    for (unsigned i = 0; i < count; i++) {
        auto values = random_values(3);
        auto expected = values;

        from_func(values.data(), values.data());
        for (int x = 0; x < 3; x++) {
            range[x + 0] = std::min(range[x + 0], values[x]);
            range[x + 3] = std::max(range[x + 3], values[x]);
        }

        to_func(values.data(), values.data());
        for (int x = 6; x < 9; x++) {
            range[x + 0] = std::min(range[x + 0], values[x - 6]);
            range[x + 3] = std::max(range[x + 3], values[x - 6]);
        }

        auto ret = VectorIsNear(values, expected, 0.005);
        if (!ret) {
            return ret;
        }
    }
    /*auto ret = VectorIsNear(range, {0,0,0,1,1,1,0,0,0,1,1,1}, 0.01);
    if (!ret) {
        return ret << " values ranges in random functions calls.";
    }*/
    return ::testing::AssertionSuccess();
}

/**
 * Create many random tests of the conversion stack, outputs and fed to inputs
 * to guarentee stability in both directions.
 *
 * @arg from - A color space to convert in one direction
 * @arg to_func - A color space to convert in the oposite direction
 * @arg count - The number of tests to create
 */
::testing::AssertionResult RandomPassthrough(
    Space::Type const &from,
    Space::Type const &to,
    unsigned count = 1000,
    bool normal_check = false
)
{
    (void)&RandomPassthrough; // Avoid compile warning
    std::srand(13375336);     // We always seed for tests' repeatability
    norm notnorm;     // Count the out of bounds

    testSpaceName(from);
    testSpaceName(to);

    auto space = Manager::get().find(from);
    if (!space)
        return ::testing::AssertionFailure() << "can't find space " << (int)from;

    auto ccount = space->getComponentCount();
    for (unsigned i = 0; i < count; i++) {
        auto ret = inb{"", 0, from, random_values(ccount), to, {}}.through_test(true, normal_check ? &notnorm : nullptr);
        if (!ret) {
            return ret << " | " << (int)from << "->" << (int)to;
        }
    }
    if (normal_check && notnorm.count > 1.0) {
        return ::testing::AssertionFailure() << " values went above or below the normal expected range of 0.0 and 1.0 by " << notnorm.count << " in " << count << " conversions\n"
            << " - Minimal ranges: " << print_values(notnorm.min) << "\n"
            << " + Maximal ranges: " << print_values(notnorm.max) << "\n";
    }
    return ::testing::AssertionSuccess();
}

/* ===== Normalization tests ===== */

class normalize : public testing::TestWithParam<inb>
{
};

/**
 * Test that the normalization functions as expected for this color space.
 */
TEST_P(normalize, values)
{
    inb test = GetParam();
    testSpaceName(test.space_in);
    auto color = Color(test.space_in, test.in);
    color.normalize();
    auto scope = test.enable_scope();
    EXPECT_TRUE(VectorIsNear(color.getValues(), test.out, 0.001));
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
