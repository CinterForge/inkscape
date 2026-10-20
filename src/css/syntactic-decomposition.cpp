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

#include "syntactic-decomposition.h"

#include <glib.h>
#include <memory>
#include <optional>
#include <regex>

#include "3rdparty/libcroco/src/cr-declaration.h"
#include "3rdparty/libcroco/src/cr-om-parser.h"
#include "3rdparty/libcroco/src/cr-selector.h"
#include "3rdparty/libcroco/src/cr-statement.h"
#include "3rdparty/libcroco/src/cr-stylesheet.h"
#include "attribute-rel-svg.h"

namespace Inkscape::CSS {

namespace {

using GucharBuffer = std::unique_ptr<guchar[], decltype(&g_free)>;
using CrocoStyleSheetHandle = std::unique_ptr<CRStyleSheet, decltype(&cr_stylesheet_destroy)>;

/// Wrapper around libcroco to reliably produce a croco stylesheet from string.
CrocoStyleSheetHandle parse_css_from_string(std::string const &css)
{
    CRStyleSheet *stylesheet = nullptr;
    if (cr_om_parser_simply_parse_buf((guchar const *)(css.c_str()), css.size(), CR_UTF_8, &stylesheet) == CR_OK) {
        return {stylesheet, &cr_stylesheet_destroy};
    }

    if (stylesheet) {
        // Something went wrong and yet we have a stylesheet: destroy it as it might be broken
        cr_stylesheet_destroy(stylesheet);
    }
    return {nullptr, nullptr};
}

/// Join a list of CRString's on ", ".
std::string join_crstring_list_with_commas(GList const &GList_of_CRString)
{
    std::string result;

    for (auto const *elem = &GList_of_CRString; elem; elem = elem->next) {
        if (auto data = reinterpret_cast<CRString const *>(elem->data)) {
            if (data->stryng && data->stryng->str) {
                result += ' ' + std::string(data->stryng->str) + ',';
            }
        }
    }
    if (result.back() == ',') {
        result.pop_back();
    }

    return result;
}

std::optional<SyntacticElement> classify_and_convert_from_croco(CRStatement const &statement);

/// Convert a libcroco ruleset to a RuleStatement.
RuleStatement convert_from_croco(CRRuleSet const &ruleset)
{
    RuleStatement result;

    if (auto const *selectors = ruleset.sel_list) {
        result.selectors = selector_to_validated_string(*selectors);
    }
    if (auto const *declarations = ruleset.decl_list) {
        static std::regex const space_before_colon{" :"};
        static std::regex const semicolon_without_space{";([^\\s])"};

        GucharBuffer const as_guchar_string{cr_declaration_list_to_string(declarations, 0), &g_free};
        result.rules =
            std::regex_replace(reinterpret_cast<char const *>(as_guchar_string.get()), space_before_colon, ":");
        result.rules = std::regex_replace(result.rules, semicolon_without_space, "; $1");
    }
    return result;
}

/// Convert a libcroco @media rule to a BlockAtStatement.
BlockAtStatement convert_from_croco(CRAtMediaRule const &media)
{
    std::string header = "@media";
    if (media.media_list) {
        header += join_crstring_list_with_commas(*media.media_list);
    }

    // Process the block contents as a nested sub-stylesheet.
    std::vector<SyntacticElement> block_contents;
    for (auto ruleset = media.rulesets; ruleset; ruleset = ruleset->next) {
        if (auto &&element = classify_and_convert_from_croco(*ruleset)) {
            block_contents.emplace_back(std::move(*element));
        }
    }

    return BlockAtStatement{.at_statement = std::move(header),
                            .block_content = std::make_unique<SyntacticDecomposition>(std::move(block_contents))};
}

/// Convert a generic libcroco statement (of "other" type) to an OtherStatement.
OtherStatement convert_from_croco(CRStatement const &generic_statement)
{
    static std::regex const space_before_semicolon{" ;"};
    std::unique_ptr<gchar[], decltype(&g_free)> serialized{cr_statement_to_string(&generic_statement, 0), &g_free};
    return OtherStatement{std::regex_replace(serialized.get(), space_before_semicolon, ";")};
}

/// Query the type of a libcroco statement and return an appropriate conversion to a SyntacticElement.
std::optional<SyntacticElement> classify_and_convert_from_croco(CRStatement const &statement)
{
    switch (statement.type) {
        case AT_RULE_STMT:
            g_warning("Ignoring an unrecognized @-rule in CSS stylesheet, line %u col %u", statement.location.line,
                      statement.location.column);
            break;
        case RULESET_STMT:
            if (auto const *ruleset = statement.kind.ruleset) {
                return convert_from_croco(*ruleset);
            }
            break;
        case AT_MEDIA_RULE_STMT:
            if (auto const *media = statement.kind.media_rule) {
                return convert_from_croco(*media);
            }
            break;

        // Generic statements
        case AT_IMPORT_RULE_STMT:
            [[fallthrough]];
        case AT_PAGE_RULE_STMT:
            [[fallthrough]];
        case AT_CHARSET_RULE_STMT:
            [[fallthrough]];
        case AT_FONT_FACE_RULE_STMT:
            return convert_from_croco(statement);
    }
    return {};
}
} // namespace

SyntacticDecomposition::SyntacticDecomposition(std::string const &css)
{
    auto const stylesheet = parse_css_from_string(css);
    if (!stylesheet) {
        return;
    }

    for (auto const *statement = stylesheet->statements; statement; statement = statement->next) {
        if (auto &&element = classify_and_convert_from_croco(*statement)) {
            _elements.emplace_back(std::move(*element));
        }
    }
}

std::string selector_to_validated_string(CRSelector const &croco_selector)
{
    for (auto *selector = croco_selector.simple_sel; selector; selector = selector->next) {
        if ((selector->type_mask & TYPE_SELECTOR) && !(selector->type_mask & UNIVERSAL_SELECTOR)) {
            if (auto *selector_name = selector->name) {
                if (auto *name_string = selector_name->stryng) {
                    if (name_string->str && !SPAttributeRelSVG::isSVGElement(name_string->str)) {
                        return (croco_selector.next || selector->next) ? "" : std::string{"."} + name_string->str;
                    }
                }
            }
        }
    }
    // Otherwise, just serialize the selector to a string
    GucharBuffer serialized{cr_selector_to_string(&croco_selector), &g_free};
    return serialized ? std::string{reinterpret_cast<char const *>(serialized.get())} : "";
}
} // namespace Inkscape::CSS

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