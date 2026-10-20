// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * @copyright
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <variant>

#include "attribute-rel-svg.h"
#include "css/syntactic-decomposition.h"

/// Mock Glib::ustring
namespace Glib {
ustring::ustring(char const *c_string)
    : string_{c_string}
{}
ustring::~ustring() noexcept = default;
} // namespace Glib

struct MockStatics
{
    MOCK_CONST_METHOD1(m_isSVGElement, bool(std::string const &));

    MockStatics() { instance = this; }
    ~MockStatics() { instance = nullptr; }

    inline static MockStatics *instance = nullptr;
};

bool SPAttributeRelSVG::isSVGElement(Glib::ustring const &element)
{
    return MockStatics::instance->m_isSVGElement(element.raw());
}

namespace Inkscape::CSS {

using SimpleOutput = std::string;
using SelectorAndRule = std::pair<std::string, std::string>;

struct ParseCSSTestCase
{
    using ExpectedRepresentation = std::variant<SimpleOutput, SelectorAndRule>;
    std::string input_css;
    std::vector<ExpectedRepresentation> expected_repr;
};

/// Helper functions to check if the string representation in a test case matches the structured output
void representation_check(ParseCSSTestCase::ExpectedRepresentation const &rep, RuleStatement const &rule)
{
    auto *rule_display = std::get_if<SelectorAndRule>(&rep);
    ASSERT_TRUE(rule_display);
    EXPECT_EQ(rule_display->first, rule.selectors);
    EXPECT_EQ(rule_display->second, rule.rules);
}

void representation_check(ParseCSSTestCase::ExpectedRepresentation const &rep, BlockAtStatement const &block_at)
{
    auto *at_display = std::get_if<SelectorAndRule>(&rep);
    ASSERT_TRUE(at_display);
    EXPECT_EQ(at_display->first, block_at.at_statement);
}

void representation_check(ParseCSSTestCase::ExpectedRepresentation const &rep, OtherStatement const &other)
{
    auto *output = std::get_if<SimpleOutput>(&rep);
    ASSERT_TRUE(output);
    EXPECT_EQ(*output, other);
}

void representation_check(ParseCSSTestCase::ExpectedRepresentation const &rep, SyntacticElement const &e)
{
    return std::visit([&rep](auto const &element) { return representation_check(rep, element); }, e);
}

ParseCSSTestCase const parse_test_cases[] = {
    // Basic rules
    {.input_css = "text { color: red; }", .expected_repr = {SelectorAndRule{"text", "color: red;"}}},
    {.input_css = "* { color: red; }", .expected_repr = {SelectorAndRule{"*", "color: red;"}}},
    // Rule with comma-separated selector
    {.input_css = "text, circle { color: red; }", .expected_repr = {SelectorAndRule{"text, circle", "color: red;"}}},
    // Check that composite selectors work; insert some whitespace
    {.input_css = ".myclass .myother.foo {\n\t cx: 5; \n}",
     .expected_repr = {SelectorAndRule{".myclass.myother.foo", "cx: 5;"}}},
    // Check that comments are stripped; TODO: maybe show comments in the Selectors & CSS dialog?
    {.input_css = R"EOD(
circle { stroke: none; }
/* This is a CSS comment */
rect { fill: none; }
)EOD",
     .expected_repr =
         {
             SelectorAndRule{"circle", "stroke: none;"},
             SelectorAndRule{"rect", "fill: none;"},
         }},
    // Check that @media rules are parsed (note: the entire content of the block following the media rule
    // will be shown as "ruleset" due to a limitation of the Selectors & CSS dialog). TODO: remove the limitation.
    {.input_css = "@media print { rect { fill: green; } }",
     .expected_repr = {SelectorAndRule{"@media print", "rect { fill: green; }"}}},
    // @media rule followed by another rule
    {.input_css = R"EOD(
    @media print {
        rect { fill: green; }
    }
    circle { stroke: none; opacity: 90% }
    )EOD",
     .expected_repr = {SelectorAndRule{"@media print", "rect { fill: green; }"},
                       SelectorAndRule{"circle", "stroke: none; opacity: 90%;"}}},
// Example from https://gitlab.com/inkscape/inkscape/-/issues/3003 - this is still not handled properly by Libcroco
#if 0
    {.input_css = R"EOD("
    @import url(https://fonts.googleapis.com/css?family=UnifrakturCook:700);
                text {
                    font-family: UnifrakturCook;
                }
    )EOD",
     .expected_repr = {SimpleOutput("@import url(https://fonts.googleapis.com/css?family=UnifrakturCook:700);"),
                       SelectorAndRule("text", "font-family: UnifrakturCook;")}},
#endif
    // Legacy behaviour: "fix" non-SVG element selectors by making them classes
    {.input_css = "div { fill: none; }", .expected_repr = {SelectorAndRule{".div", "fill: none;"}}},
    // Check that @charset works
    {.input_css = "@charset 'UTF-8';", .expected_repr = {SimpleOutput{R"(@charset "UTF-8";)"}}},
};

using namespace ::testing;

struct ParseCSSTest : TestWithParam<ParseCSSTestCase>
{
    ParseCSSTest()
    {
        EXPECT_CALL(mock, m_isSVGElement(AnyOf(StrEq("text"), StrEq("circle"), StrEq("rect"))))
            .WillRepeatedly(Return(true));
        EXPECT_CALL(mock, m_isSVGElement(StrEq("div"))).WillRepeatedly(Return(false));
    }

    MockStatics mock;
};

TEST_P(ParseCSSTest, ParseCSSForDialogDisplay)
{
    auto const &test_case = GetParam();
    SyntacticDecomposition const decomposition{test_case.input_css};
    decomposition.for_each([&expected = test_case.expected_repr, pos = 0](auto const &element) mutable {
        ASSERT_LT(pos, expected.size());
        representation_check(expected[pos++], element);
    });
}

INSTANTIATE_TEST_SUITE_P(ParseCSSForDialogTests, ParseCSSTest, ::testing::ValuesIn(parse_test_cases));
} // namespace Inkscape::CSS