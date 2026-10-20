// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Shared test header for testing colors
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_TEST_UTILS_H
#define INKSCAPE_TEST_UTILS_H

#include <cmath>
#include <iomanip>
#include <gtest/gtest.h>

namespace {

/**
 * Allow the correct tracing of the file and line where data came from when using P_TESTs.
 */
struct traced_data
{
    const char *_file;
    const int _line;

    ::testing::ScopedTrace enable_scope() const { return ::testing::ScopedTrace(_file, _line, ""); }
};
// Macro for the above tracing in P Tests
#define _P(type, ...)                   \
    type                                \
    {                                   \
        __FILE__, __LINE__, __VA_ARGS__ \
    }

/**
 * Print a vector of doubles for debugging
 *
 * @arg v      - The values to be printed
 * @arg other  - When provided, pads the values to the same width between other and v
 * @arg failed - When provided, colors the failed entries for console output.
 */
template <typename T>
std::string print_values(T const &v, T const *other = nullptr, std::vector<bool> failed = {})
{
    auto as_string = [](double v, int precision) {
        std::ostringstream ch;
        ch << std::setprecision(precision) << v;
        return ch.str();
    };

    std::ostringstream oo; 
    oo << "{";
    for (auto i = 0; i < v.size(); i++) {
        int precision = (other ? std::min((*other)[i], v[i]) : v[i]) < 0 ? 4 : 3;
        int other_size = other ? as_string((*other)[i], precision).length() : 0;
        auto item = as_string(v[i], precision);

        if (failed.size() && failed[i]) {
            // Turn a failed text RED in a console
            oo << "\x1B[91m";
        }
        oo << std::setw(std::max((int)item.length(), other_size)) << item;
        if (failed.size() && failed[i]) {
            oo << "\033[0m";
        }

        if (i < v.size() - 1)
            oo << ", ";
    }
    oo << "}(" << v.size() << ")";
    return oo.str();
}

inline static ::testing::AssertionResult IsNear(double a, double b, double epsilon = 0.01)
{
    if (std::fabs(a - b) < epsilon) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure();
}

/**
 * Test each value in a values list is within a certain distance from each other.
 */
template <typename T>
inline static ::testing::AssertionResult VectorIsNear(T const &A, T const &B, double epsilon)
{
    bool same_size = A.size() == B.size();
    bool is_same = same_size;
    std::vector<bool> failed(A.size(), false);
    for (size_t i = 0; i < A.size() && i < B.size(); i++) {
        failed[i] = (std::fabs(A[i] - B[i]) >= epsilon);
        is_same = is_same and !failed[i];
    }
    if (!is_same) {
        return ::testing::AssertionFailure() << "\n"
                                             << print_values(A, same_size ? &B : nullptr, failed) << "\n != \n"
                                             << print_values(B, same_size ? &A : nullptr, failed);
    }
    return ::testing::AssertionSuccess();
}


/**
 * Generate a count of random doubles between 0 and 1.
 *
 * Randomly appends an extra value for optional opacity.
 */
inline static std::vector<double> random_values(unsigned ccount)
{
    std::vector<double> values;
    for (unsigned j = 0; j < ccount; j++) {
        values.emplace_back(static_cast<double>(std::rand()) / RAND_MAX);
    }
    // randomly add opacity
    if (std::rand() > (RAND_MAX / 2)) {
        values.emplace_back(static_cast<double>(std::rand()) / RAND_MAX);
    }
    return values;
}

/** Locale testing fixture.
 *
 * This fixture only creates a locale, it is test responsibility to use it.
 * \code{.cpp}
 * class FooTestFixture : public LocaleTestFixture {};
 * TEST_P(FooTestFixture, Test1)
 * {
 *    std::sstream stream("1.32");
 *    stream.imbue(locale);
 *    float v;
 *    stream >> v;
 * }
 * TEST_P(FooTestFixture, Test2) { .. }
 * INSTANTIATE_TEST_SUITE_P(TestSuite,
 *                           FooTestFixture,
 *                           testing::Values("C", "de_DE.UTF8"));
 * \endcode
 */
class LocaleTestFixture : public ::testing::TestWithParam<char const *>
{
protected:
    std::locale locale;

    void SetUp() override { locale = std::locale(GetParam()); }
};

/** Global locale testing fixture.
 *
 * Parametrized test fixtures which automatically sets up and restores global locale.
 * \code{.cpp}
 * class FooTestFixture : public GlobalLocaleTestFixture {};
 * TEST_P(FooTestFixture, Test1)
 * {
 *    test_logic
 * }
 * TEST_P(FooTestFixture, Test2) { .. }
 * INSTANTIATE_TEST_SUITE_P(TestSuite,
 *                           FooTestFixture,
 *                           testing::Values("C", "de_DE.UTF8"));
 * \endcode
 */
class GlobalLocaleTestFixture : public LocaleTestFixture
{
    std::locale backup;

protected:
    void SetUp() override
    {
        try {
            LocaleTestFixture::SetUp();
        } catch (std::exception const &e) {
            GTEST_SKIP() << "Skipping locale '" << GetParam() << "' not available\n";
        }
        backup = std::locale::global(locale);
    }

    void TearDown() override
    {
        LocaleTestFixture::TearDown();
        std::locale::global(backup);
    }
};

} // namespace

#endif // INKSCAPE_TEST_UTILS_H
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
