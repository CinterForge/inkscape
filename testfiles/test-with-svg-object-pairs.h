// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * @brief Test fixture which loads an SVG with test cases for a particular test suite.
 *
 * The SVG should contain test objects (test input data) and corresponding reference objects.
 * They should be arranged in such a way that the user can open the SVG and visually inspect
 * it, in order to confirm that the tested behavior is correct.
 *
 * Each test objects and the corresponding reference object have IDs containing the same
 * integer. The fixture automatically loads the document and exposes the test objects and the
 * corresponding reference objects, using the numbering to establish the correspondence.
 */
/*
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>
#include <span>
#include <gtest/gtest.h>
#include <string>

#include "document.h"

class SPObject;

namespace Inkscape {

/// A pair of pointers to SPObjects participating in a test case.
struct SvgObjectTestCase
{
    SPObject const* test_object = nullptr;
    SPObject const* reference_object = nullptr;
};

/**
 * @brief A test fixture exposing pairs of objects from an SPDocument.
 *
 * In order to use this fixture, you must wrap its constructor providing:
 * - the path to the SVG test file, relative to the Inkscape test directory
 *   (the 'testfiles' directory in the source tree).
 * - the number of test objects in the file.
 *
 * For example, your test fixture could be declared as follows:
 * \code
class MyTest : public Inkscape::TestWithSvgObjectPairs
{
public:
    MyTest()
        : TestWithSvgObjectPairs("path/to/my.svg", 3) {}
    // your code
};
\endcode
 *
 * This base class will load the SVG file and look for objects with IDs consisting of the
 * prefixes "test-object-" and "reference-object-" followed by consecutive integers starting
 * from 0, until the specified number of test objects is found. In the example above, the
 * SVG file is expected to contain objects with the following IDs:
 * "test-object-0", "reference-object-0",
 * "test-object-1", "reference-object-1",
 * "test-object-2", "reference-object-2".
 *
 * Each integer occurring in these IDs establishes a link between the test input object
 * and its reference object (which could, for example, represent the expected output of
 * an operation under test).
 *
 * Use the method \c getTestCases() to access a list of SPObject pointer pairs.
 */
class TestWithSvgObjectPairs : public ::testing::Test
{
public:
    /**
     * @brief Initialize a test based on an SVG file containing object pairs.
     *
     * @param svg_path The path to the SVG file relative to INKSCAPE_TESTS_DIR.
     * @param num_tests The number of object pairs with specially constructed IDs.
     */
    TestWithSvgObjectPairs(char const *const svg_path, size_t num_tests);

    /**
     * @brief Obtain a view of test cases (pointers to SPObjects).
     * @return A span of SvgObjectPair for use in the test.
     */
    std::span<const SvgObjectTestCase> getTestCases() const { return _test_cases; }

    void SetUp() override;
    void TearDown() override;

private:
    std::string _filename;
    size_t _num_tests;
    std::unique_ptr<SPDocument> _doc;
    std::vector<SvgObjectTestCase> _test_cases;
};

} // namespace Inkscape
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
