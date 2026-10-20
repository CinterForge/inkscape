// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Implementation of class TestWithSvgObjectPairs.
 */
/*
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "test-with-svg-object-pairs.h"

#include <exception>
#include <vector>
#include <gtest/gtest.h>
#include <string>

#include "inkscape.h"
#include "document.h"

class SVGTestError : public std::exception
{
public:
    SVGTestError(const std::string message) : _error_message{std::move(message)} {}
    const char *what() const noexcept override { return _error_message.c_str(); }
private:
    std::string _error_message;
};

const char *TEST_OBJECT_PREFIX      = "test-object-";
const char *REFERENCE_OBJECT_PREFIX = "reference-object-";

namespace Inkscape {

TestWithSvgObjectPairs::TestWithSvgObjectPairs(char const *const svg_path, size_t num_tests)
    : _num_tests{num_tests}
{
    if (!Inkscape::Application::exists()) {
        Inkscape::Application::create(false);
    }
    _filename = std::string(INKSCAPE_TESTS_DIR) + '/' + std::string(svg_path);
}

void TestWithSvgObjectPairs::SetUp()
{
    _doc = SPDocument::createNewDoc(_filename.c_str());
    if (!_doc) {
        throw SVGTestError(std::string("Could not open test file \"") + _filename + "\"!");
    }
    _doc->ensureUpToDate();
    for (unsigned case_number = 0; case_number < _num_tests; ++case_number) {
        auto const case_string = std::to_string(case_number);
        auto const test_object_id = std::string(TEST_OBJECT_PREFIX) + case_string;
        auto const reference_object_id = std::string(REFERENCE_OBJECT_PREFIX) + case_string;
        auto const *test_object = _doc->getObjectById(test_object_id.c_str());
        auto const *reference_object = _doc->getObjectById(reference_object_id.c_str());

        bool const found = test_object && reference_object;
        if (!found) {
            throw SVGTestError(std::string("Could not find objects with ids '") + test_object_id + "', '"
                               + reference_object_id + "' in the file '" + _filename + "'!");
        }
        _test_cases.push_back({
            .test_object = test_object,
            .reference_object = reference_object
        });
    }
    // Check that there is no forgotten test object with a higher index
    const std::string forgotten_test_object_id = std::string(TEST_OBJECT_PREFIX) + std::to_string(_num_tests);
    const std::string forgotten_ref_object_id = std::string(REFERENCE_OBJECT_PREFIX) + std::to_string(_num_tests);
    for (auto id : {forgotten_test_object_id.c_str(), forgotten_ref_object_id.c_str()}) {
        if (_doc->getObjectById(id)) {
            FAIL() << "Found forgotten test object with id='" << id << "' not included in iteration!";
        }
    }
}

void TestWithSvgObjectPairs::TearDown() { _test_cases.clear(); }

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
