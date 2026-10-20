// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Parsing utils capable of producing a rudimentary syntactic decomposition of a CSS stylesheet.
 */
/*
 * Authors: Rafał Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_CSS_SYNTACTIC_DECOMPOSITION_H
#define INKSCAPE_CSS_SYNTACTIC_DECOMPOSITION_H

#include <algorithm>
#include <memory>
#include <string>
#include <variant>
#include <vector>

using CRSelector = struct _CRSelector;

namespace Inkscape::CSS {

class SyntacticDecomposition;

/** A decomposed CSS rule statement, consisting of a selector (which can be complex),
 *  and the associated set of rules.
 *
 *  For example, the CSS statement
 *
 *  rect, .myClass1 { fill: yellow; stroke: none; }
 *
 *  has selectors == "rect, .myClass1" and rules == "fill: yellow; stroke: none".
 */
struct RuleStatement
{
    std::string selectors; ///< Selectors for a rule set statement
    std::string rules;     ///< Semicolon-separated rules
};

/** A decomposed block @-statement, consisting of the statement and the contents of the
 *  associated block.
 *
 *  For example, the CSS statement
 *
 *  @media print {
 *     circle { fill: none; }
 *  }
 *
 *  has at_statement == "@media print" and *block_content containing RuleStatement("circle", "fill: none").
 */
struct BlockAtStatement
{
    std::string at_statement;
    std::unique_ptr<SyntacticDecomposition> block_content;
};

/** Another CSS statement, currently not handled in a special way; for example @charset.
 *  \todo Add support for comments and @font-face statements.
 */
using OtherStatement = std::string;

/// A syntactic element of a CSS stylesheet is either a rule set statement, a block @-statement, or some
/// other, "generic" statement.
using SyntacticElement = std::variant<RuleStatement, BlockAtStatement, OtherStatement>;

/// A SyntacticElementHandler is a callable which can accept all possible variant types of a syntactic element,
/// assuming that they are passed by const &.
template <class F>
concept SyntacticElementHandler =
    requires(RuleStatement const &rule, BlockAtStatement const &block, OtherStatement const &other, F f) {
        f(rule);
        f(block);
        f(other);
    };

/**
 * A partial syntactic decomposition of a CSS stylesheet into syntactic elements.
 */
class SyntacticDecomposition
{
public:
    /// Build a syntactic decomposition from a CSS string.
    explicit SyntacticDecomposition(std::string const &css);

    /// Construct from a collection of syntactic elements.
    explicit SyntacticDecomposition(std::vector<SyntacticElement> elements)
        : _elements{std::move(elements)}
    {}

    /// Returns true when there are no elements.
    [[nodiscard]] bool empty() const { return _elements.empty(); }

    /// Execute an operation for each syntactic element, in the order of their occurrence.
    void for_each(SyntacticElementHandler auto &&handler) const
    {
        std::for_each(_elements.begin(), _elements.end(),
                      [&handler, this](auto const &element) { std::visit(handler, element); });
    }

private:
    std::vector<SyntacticElement> _elements;
};

/**
 * Convert a CSS selector to a string, performing a fix-up if needed.
 *
 * Fixup: If there is only a single, simple type-like selector which doesn't correspond to an SVG element,
 * the function fixes it up by converting to a class selector. For example the CSS " p { color: red; } " has
 * a selector "p" which is not a valid SVG element, so we convert it to a class and return ".p".
 */
std::string selector_to_validated_string(CRSelector const &croco_selector);

} // namespace Inkscape::CSS

#endif // INKSCAPE_CSS_SYNTACTIC_DECOMPOSITION_H
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
