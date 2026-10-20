// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   See Git history
 *
 * Copyright (C) 2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "font-utils.h"

#include <iostream> // TEMP TEMP TEMP
#include <glibmm/regex.h>
#include <pango/pango-font.h>

namespace Inkscape {

// Pass fontspec to and back from Pango to get a the fontspec in canonical form. TEST
Glib::ustring canonize_fontspec(Glib::ustring const &fontspec)
{
    PangoFontDescription *descr = pango_font_description_from_string(fontspec.c_str());

    // CSS Font Module Level 4 dictates that "font-weight", "font-width' ("font-stretch"),
    // "font-style", and "font-optical-sizing" shoud be used rather than the
    // "font-variation-settings" axes 'wght', 'wdth', 'slnt'/'ital', or 'opsz'.
    // At the moment (May 2026), Pango only provides adequate support for doing so for
    // font weight.
    //
    // Pango font description does not order the axes. This can cause problems when
    // trying to match named instances. Reorder alphabetically here.
    std::string variations;

    const char* str = pango_font_description_get_variations(descr);
    if (str) {
        auto variations_map = parse_variations(str);
        for (auto [tag, value] : variations_map) {
            if (tag == "wght") {
                auto weight = std::stoi(value);
                pango_font_description_set_weight(descr, (PangoWeight)weight);
            } else if (tag == "ital" && value == "1") {
                pango_font_description_set_style(descr, PANGO_STYLE_ITALIC);
            } else {
                variations += tag;
                variations += "=";
                variations += value;
                variations += ",";
            }
        }

        if (variations.length() >= 1) { // Remove last comma and save
            variations.pop_back();
        }

        pango_font_description_set_variations(descr, variations.c_str());
    }

    gchar *canonized = pango_font_description_to_string(descr);
    Glib::ustring Canonized = canonized;
    g_free(canonized);
    pango_font_description_free(descr);

    // Pango canonized strings remove space after comma between family names. Put it back.
    // But don't add a space inside a 'font-variation-settings' declaration (this breaks Pango).
    size_t i = 0;
    while ((i = Canonized.find_first_of(",@", i)) != std::string::npos ) {
        if (Canonized[i] == '@') // Found start of 'font-variation-settings'.
            break;
        Canonized.replace(i, 1, ", ");
        i += 2;
    }

    return Canonized;
}


// Returns a map of 'tag' => 'value' from a variations string. TEST
std::map<std::string, std::string> parse_variations(const char* variations)
{
    std::map<std::string, std::string> variations_map;

    auto regex = Glib::Regex::create("(\\w{4})=([-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?)");
    Glib::MatchInfo matchInfo;

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(",", variations);
    for (auto const &token : tokens) {
        regex->match(token, matchInfo);
        if (matchInfo.matches()) {
            auto tag   = matchInfo.fetch(1).raw();
            auto value = matchInfo.fetch(2).raw();
            variations_map[tag] = value; // This will alphabetize axes based on tag.
        }
    }

    return variations_map;
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
