// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Parse a string containing number ranges.
 *
 *  Authors:
 *    Martin Owens
 *    PBS <pbs3141@gmail.com>
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parse-int-range.h"

#include <cassert>
#include <charconv>

#include <glibmm/regex.h>

namespace Inkscape {
namespace {

/// Extract the string view from a match. Should be in Glib::Regex.
std::string_view fetch(Glib::MatchInfo &m, int i)
{
    int a, b;
    if (!m.fetch_pos(i, a, b)) {
        return {}; // error
    }
    if (a == -1) {
        return {}; // no match
    }
    auto const str = g_match_info_get_string(m.gobj());
    return {str + a, static_cast<size_t>(b - a)};
};

/// Parse a string_view as a numeric type T. Should be in the standard library.
template <typename T>
T from_chars(std::string_view const &v)
{
    T result = 0;
    std::from_chars(v.begin(), v.end(), result);
    return result;
};

} // namespace

std::set<unsigned> parseIntRange(std::string const &input, unsigned start, unsigned end)
{
    // Special word based translations go here:
    if (input == "all") {
        return parseIntRange("-", start, end);
    }

    auto valid = [&] (unsigned val) {
        return start <= val && (!end || val <= end);
    };

    auto clamp_to_valid = [&] (unsigned val) {
        val = std::max(val, start);
        if (end) {
            val = std::min(val, end);
        }
        return val;
    };

    std::set<unsigned> out;

    auto add = [&] (unsigned val) {
        out.insert(val);
    };

    auto add_if_valid = [&] (unsigned val) {
        if (valid(val)) {
            add(val);
        }
    };

    static auto const regex = Glib::Regex::create("\\s*(((\\d*)\\s*-\\s*(\\d*))|(\\d+))\\s*,?", Glib::Regex::CompileFlags::OPTIMIZE | Glib::Regex::CompileFlags::ANCHORED);

    auto pos = input.data();
    Glib::MatchInfo m;

    while (regex->match(pos, m)) {
        if (auto const single = fetch(m, 5); !single.empty()) {
            add_if_valid(from_chars<unsigned>(single));
        } else if (auto const range = fetch(m, 2); !range.empty()) {
            auto const first = fetch(m, 3);
            auto const last = fetch(m, 4);
            auto const first_num = first.empty() ? start : from_chars<unsigned>(first);
            auto const last_num = last.empty() ? (end ? end : first_num) : from_chars<unsigned>(last);
            auto const min = clamp_to_valid(std::min(first_num, last_num));
            auto const max = clamp_to_valid(std::max(first_num, last_num));
            for (auto i = min; i <= max; i++) {
                add(i);
            }
        } else {
            assert(false); // both groups above cannot be empty
            break;
        }
        pos = fetch(m, 0).end();
    }

    return out;
}

} // namespace Inkscape

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
