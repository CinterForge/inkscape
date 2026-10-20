// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "svg-path-parser.h"

#include <boost/spirit/home/x3.hpp>
#include <unordered_map>

namespace Inkscape {

/** @brief Reformat a path 'd' attibute for better readability. */
Glib::ustring SvgPathParser::prettify_svgd(Glib::ustring const &d)
{
    if (d.empty()) {
        return {};
    }

    // Reserve 25% extra space to avoid reallocations
    Glib::ustring out;
    out.reserve(d.size() * 5 / 4);

    int args_needed = 0;
    int args_seen = 0;
    bool first_line = true;

    // Actions to perform on encountering a path command
    auto on_command = [&](char cmd) {
        if (!first_line) {
            out.push_back('\n');
        }

        first_line = false;
        args_needed = get_command_arg_count(cmd);
        args_seen = 0;
        out.push_back(cmd);

        if (args_needed != 0) {
            out.push_back(' ');
        }
    };

    // Actions to perform on encountering a number (floating or integer)
    auto on_number = [&](Glib::ustring const& num) {
        if (args_seen > 0) {
            if (args_seen == args_needed) {
                out.append("\n  ");
                args_seen = 0;
            } else {
                out.push_back(' ');
            }
        }
        out.append(num);
        ++args_seen;
    };

    auto const command =
        boost::spirit::x3::char_("A-Za-z")[([&](auto& ctx) {
            on_command(_attr(ctx));
        })];

    auto const number =
        boost::spirit::x3::raw[boost::spirit::x3::double_][([&](auto& ctx) {
            on_number(Glib::ustring(_attr(ctx).begin(), _attr(ctx).end()));
        })];

    // Characters to skip
    constexpr auto skipper = boost::spirit::x3::space | boost::spirit::x3::char_(',');

    auto const grammar = *(command | number);

    boost::spirit::x3::phrase_parse(d.begin(), d.end(), grammar, skipper);

    return out;
}

int SvgPathParser::get_command_arg_count(char cmd)
{
    static const std::unordered_map<char, int> cmd_argument_count_map {
        {'Z',0},
        {'z',0},
        {'H',1},
        {'h',1},
        {'V',1},
        {'v',1},
        {'M',2},
        {'m',2},
        {'L',2},
        {'l',2},
        {'T',2},
        {'t',2},
        {'Q',4},
        {'q',4},
        {'S',4},
        {'s',4},
        {'C',6},
        {'c',6},
        {'A',7},
        {'a',7}
    };

    auto it = cmd_argument_count_map.find(cmd);
    return it != cmd_argument_count_map.end() ? it->second : 0;
}

} // Namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
