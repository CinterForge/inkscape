// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Extract OpenType features from a font using HarfBuzz.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018, 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "OpenTypeUtil.h"


#include <iostream>  // For debugging
#include <memory>
#include <unordered_map>

// Harfbuzz
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#include <glibmm/regex.h>

// SVG in OpenType
#include "io/stream/gzipstream.h"
#include "io/stream/bufferstream.h"

#include "util/delete-with.h"

// Utilities used in this file

using HbSet = std::unique_ptr<hb_set_t, Inkscape::Util::Deleter<hb_set_destroy>>;

void dump_tag( guint32 *tag, Glib::ustring prefix = "", bool lf=true ) {
    std::cout << prefix
              << ((char)((*tag & 0xff000000)>>24))
              << ((char)((*tag & 0x00ff0000)>>16))
              << ((char)((*tag & 0x0000ff00)>> 8))
              << ((char)((*tag & 0x000000ff)    ));
    if( lf ) {
        std::cout << std::endl;
    }
}

Glib::ustring extract_tag( guint32 *tag ) {
    Glib::ustring tag_name;
    tag_name += ((char)((*tag & 0xff000000)>>24));
    tag_name += ((char)((*tag & 0x00ff0000)>>16));
    tag_name += ((char)((*tag & 0x0000ff00)>> 8));
    tag_name += ((char)((*tag & 0x000000ff)    ));
    return tag_name;
}

// Retrieve name from OpenType name table.
Glib::ustring get_name_string(hb_face_t* hb_face, hb_ot_name_id_t name_id) {

    unsigned int text_size = 1023;
    std::vector<char> buffer(text_size + 1); // Always returns null terminated string, not included in text_size.
    hb_ot_name_get_utf8(hb_face, name_id, HB_LANGUAGE_INVALID, &text_size, buffer.data()); // Defaults to "en".
    return Glib::ustring(buffer.data());
}


void readOpenTypeTableList(hb_font_t* hb_font, std::unordered_set<std::string>& list) {

    hb_face_t* hb_face = hb_font_get_face (hb_font);

    static const unsigned int MAX_TABLES = 100;
    unsigned int table_count = MAX_TABLES;
    hb_tag_t table_tags[MAX_TABLES];
    auto count = hb_face_get_table_tags(hb_face, 0, &table_count, table_tags);

    for (unsigned int i = 0; i < count; ++i) {
        char buf[5] = {}; // 4 characters plus null termination.
        hb_tag_to_string(table_tags[i], buf);
        list.emplace(buf);
    }
}

// Later (see get_glyphs) we need to lookup the Unicode codepoint for a glyph
// but there's no direct API for that. So, we need a way to iterate over all
// glyph mappings and build a reverse map.
// FIXME: we should handle UVS at some point... or better, work with glyphs directly

// Allows looking up the lowest Unicode codepoint mapped to a given glyph.
// To do so, it lazily builds a reverse map.
class GlyphToUnicodeMap {
protected:
    hb_font_t* font;
    HbSet codepointSet;

    std::unordered_map<hb_codepoint_t, hb_codepoint_t> mappings;
    bool more = true; // false if we have finished iterating the set
    hb_codepoint_t codepoint = HB_SET_VALUE_INVALID; // current iteration
public:
    GlyphToUnicodeMap(hb_font_t* font): font(font), codepointSet(hb_set_create()) {
        hb_face_collect_unicodes(hb_font_get_face(font), codepointSet.get());
    }

    hb_codepoint_t lookup(hb_codepoint_t glyph) {
        // first, try to find it in the mappings we've seen so far
        if (auto it = mappings.find(glyph); it != mappings.end())
            return it->second;

        // populate more mappings from the set
        while ((more = (more && hb_set_next(codepointSet.get(), &codepoint)))) {
            // get the glyph that this codepoint is associated with, if any
            hb_codepoint_t tGlyph;
            if (!hb_font_get_nominal_glyph(font, codepoint, &tGlyph)) continue;

            // save the mapping, and return if this is the one we were looking for
            mappings.emplace(tGlyph, codepoint);
            if (tGlyph == glyph) return codepoint;
        }
        return 0;
    }
};

void get_glyphs(GlyphToUnicodeMap& glyphMap, HbSet& set, Glib::ustring& characters) {
    hb_codepoint_t glyph = -1;
    while (hb_set_next(set.get(), &glyph)) {
        if (auto codepoint = glyphMap.lookup(glyph))
            characters += codepoint;
    }
}

SVGGlyphEntry::~SVGGlyphEntry() = default;

// Make a list of all tables found in the GSUB
// This list includes all tables regardless of script or language.
// Use Harfbuzz, Pango's equivalent calls are deprecated.
void readOpenTypeGsubTable (hb_font_t* hb_font,
                            std::map<Glib::ustring, OTSubstitution>& tables)
{
    hb_face_t* hb_face = hb_font_get_face (hb_font);

    tables.clear();

    // First time to get size of array
    auto script_count = hb_ot_layout_table_get_script_tags(hb_face, HB_OT_TAG_GSUB, 0, nullptr, nullptr);
    auto const hb_scripts = g_new(hb_tag_t, script_count + 1);

    // Second time to fill array (this two step process was not necessary with Pango).
    hb_ot_layout_table_get_script_tags(hb_face, HB_OT_TAG_GSUB, 0, &script_count, hb_scripts);

    for(unsigned int i = 0; i < script_count; ++i) {
        // std::cout << " Script: " << extract_tag(&hb_scripts[i]) << std::endl;
        auto language_count = hb_ot_layout_script_get_language_tags(hb_face, HB_OT_TAG_GSUB, i, 0, nullptr, nullptr);

        if(language_count > 0) {
            auto const hb_languages = g_new(hb_tag_t, language_count + 1);
            hb_ot_layout_script_get_language_tags(hb_face, HB_OT_TAG_GSUB, i, 0, &language_count, hb_languages);

            for(unsigned int j = 0; j < language_count; ++j) {
                // std::cout << "  Language: " << extract_tag(&hb_languages[j]) << std::endl;
                auto feature_count = hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i, j, 0, nullptr, nullptr);
                auto const hb_features = g_new(hb_tag_t, feature_count + 1);
                hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i, j, 0, &feature_count, hb_features);

                for(unsigned int k = 0; k < feature_count; ++k) {
                    // std::cout << "   Feature: " << extract_tag(&hb_features[k]) << std::endl;
                    tables[ extract_tag(&hb_features[k])];
                }

                g_free(hb_features);
            }

            g_free(hb_languages);

        } else {

            // Even if no languages are present there is still the default.
            // std::cout << "  Language: " << " (dflt)" << std::endl;
            auto feature_count = hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i,
                                                                        HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                                        0, nullptr, nullptr);
            auto const hb_features = g_new(hb_tag_t, feature_count + 1);
            hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, i,
                                                   HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                   0, &feature_count, hb_features);

            for(unsigned int k = 0; k < feature_count; ++k) {
                // std::cout << "   Feature: " << extract_tag(&hb_features[k]) << std::endl;
                tables[ extract_tag(&hb_features[k])];
            }

            g_free(hb_features);
        }
    }

    // Find glyphs in OpenType substitution tables ('gsub').
    // Note that pango's functions are just dummies. Must use harfbuzz.

    GlyphToUnicodeMap glyphMap (hb_font);

    // Loop over all tables
    for (auto table: tables) {

        // Only look at style substitution tables ('salt', 'ss01', etc. but not 'ssty').
        // Also look at character substitution tables ('cv01', etc.).
        bool style    =
            table.first == "case"  /* Case-Sensitive Forms   */                          ||
            table.first == "salt"  /* Stylistic Alternatives */                          ||
            table.first == "swsh"  /* Swash                  */                          ||
            table.first == "cwsh"  /* Contextual Swash       */                          ||
            table.first == "ornm"  /* Ornaments              */                          ||
            table.first == "nalt"  /* Alternative Annotation */                          ||
            table.first == "hist"  /* Historical Forms       */                          ||
            (table.first[0] == 's' && table.first[1] == 's' && !(table.first[2] == 't')) ||
            (table.first[0] == 'c' && table.first[1] == 'v');

        bool ligature = ( table.first == "liga" ||  // Standard ligatures
                          table.first == "clig" ||  // Common ligatures
                          table.first == "dlig" ||  // Discretionary ligatures
                          table.first == "hlig" ||  // Historical ligatures
                          table.first == "calt" );  // Contextual alternatives

        bool numeric  = ( table.first == "lnum" ||  // Lining numerals
                          table.first == "onum" ||  // Old style
                          table.first == "pnum" ||  // Proportional
                          table.first == "tnum" ||  // Tabular
                          table.first == "frac" ||  // Diagonal fractions
                          table.first == "afrc" ||  // Stacked fractions
                          table.first == "ordn" ||  // Ordinal fractions
                          table.first == "zero" );  // Slashed zero

        if (style || ligature || numeric) {

            unsigned int feature_index;
            if (  hb_ot_layout_language_find_feature (hb_face, HB_OT_TAG_GSUB,
                                                      0,  // Assume one script exists with index 0
                                                      HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                      HB_TAG(table.first[0],
                                                             table.first[1],
                                                             table.first[2],
                                                             table.first[3]),
                                                      &feature_index ) ) {

                // std::cout << "Table: " << table.first << std::endl;
                // std::cout << "  Found feature, number: " << feature_index << std::endl;

                unsigned start_offset = 0;

                while (true) {
                    unsigned lookup_indexes[32];
                    unsigned lookup_count = 32;
                    int count = hb_ot_layout_feature_get_lookups(hb_face, HB_OT_TAG_GSUB,
                                                                 feature_index,
                                                                 start_offset,
                                                                 &lookup_count,
                                                                 lookup_indexes);
                    // std::cout << "  Lookup count: " << lookup_count << " total: " << count << std::endl;

                    for (unsigned i = 0; i < lookup_count; i++) {
                        HbSet glyphs_before (hb_set_create());
                        HbSet glyphs_input  (hb_set_create());
                        HbSet glyphs_after  (hb_set_create());
                        HbSet glyphs_output (hb_set_create());

                        hb_ot_layout_lookup_collect_glyphs (hb_face, HB_OT_TAG_GSUB,
                                                            lookup_indexes[i],
                                                            glyphs_before.get(),
                                                            glyphs_input.get(),
                                                            glyphs_after.get(),
                                                            glyphs_output.get() );

                        // std::cout << "  Populations: "
                        //           << " " << hb_set_get_population (glyphs_before)
                        //           << " " << hb_set_get_population (glyphs_input)
                        //           << " " << hb_set_get_population (glyphs_after)
                        //           << " " << hb_set_get_population (glyphs_output)
                        //           << std::endl;

                        get_glyphs (glyphMap, glyphs_before, tables[table.first].before);
                        get_glyphs (glyphMap, glyphs_input,  tables[table.first].input );
                        get_glyphs (glyphMap, glyphs_after,  tables[table.first].after );
                        get_glyphs (glyphMap, glyphs_output, tables[table.first].output);

                        // std::cout << "  Before: " << tables[table.first].before.c_str() << std::endl;
                        // std::cout << "  Input:  " << tables[table.first].input.c_str() << std::endl;
                        // std::cout << "  After:  " << tables[table.first].after.c_str() << std::endl;
                        // std::cout << "  Output: " << tables[table.first].output.c_str() << std::endl;
                    }

                    start_offset += lookup_count;
                    if (start_offset >= count) {
                        break;
                    }
                }

            } else {
                // std::cout << "  Did not find '" << table.first << "'!" << std::endl;
            }
        }

    }

    g_free(hb_scripts);
}

// Make a vector of all Variation axes with ranges. This is used by the GUI.
// Variation axes tags are unique per OpenType specification. They are limited to ASCII.
void readOpenTypeFvarAxes(hb_font_t* hb_font,
                          std::vector<OTVarAxis>& axes) {

    hb_face_t* hb_face = hb_font_get_face(hb_font);

    if (!hb_ot_var_has_data(hb_face)) { // hb 1.4.2
        return;
    }

    unsigned int axis_count = hb_ot_var_get_axis_count(hb_face); // hb 1.4.2
    std::vector<hb_ot_var_axis_info_t> axes_raw(axis_count);
    hb_ot_var_get_axis_infos(hb_face, 0, &axis_count, axes_raw.data()); // hb 2.2.0

    auto axis_count_save = axis_count;
    auto axes_coords = hb_font_get_var_coords_design(hb_font, &axis_count); // hb 3.3.0
    if (axis_count_save != axis_count) {
        std::cerr << "readOpenTypeFvarAxes: number of design coordinates not equal to number of axes!" << std::endl;
    }

    for (unsigned int i = 0; i < axis_count; ++i) {
        auto axis = axes_raw[i];

        // Don't expose parametric axes internal to font, i.e. Roboto Flex has a number).
        if (axis.flags & HB_OT_VAR_AXIS_FLAG_HIDDEN) continue;

        char tag[5];
        hb_tag_to_string(axis.tag, tag);
        tag[4] = 0;

        auto name = get_name_string(hb_face, axis.name_id);

        axes.emplace_back(OTVarAxis(tag,
                                    name,
                                    axis.min_value,
                                    axis.default_value,
                                    axis.max_value,
                                    axes_coords[i]));
    }

    // std::cout << "readOpenTypeFvarAxes:" << std::endl;
    // for (auto axis: axes) {
    //     std::cout << " "         << std::setw(4)  << axis.tag
    //               << "  name:  " << std::setw(20) << axis.name
    //               << "  min:  "  << std::setw(4)  << axis.minimum
    //               << "  def:  "  << std::setw(4)  << axis.def
    //               << "  max:  "  << std::setw(4)  << axis.maximum
    //               << "  set:  "  << std::setw(4)  << axis.set_val << std::endl;
    // }
    // std::cout << std::endl;
}

// Construct a map of variable font named instances with names as key and corresponding Pango string as data.
// String is of form: AXIS1=VALUE,AXIS2=VALUE... where:
//    AXIS is a 4 character tag.
//    VALUE is a float (FIXME: currently saved as an integer).
// This is the same format as passed to "font-variation-settings".
// Axes with default values are not included.
void readOpenTypeFvarNamedInstances(hb_font_t* hb_font, std::map<Glib::ustring, Glib::ustring>& named_instance) {

    hb_face_t* hb_face = hb_font_get_face(hb_font);

    unsigned int names_count = hb_ot_var_get_named_instance_count(hb_face);

    // Get tags and default values.
    std::vector<Glib::ustring> tags; // Order is important.
    std::vector<float> defaults;
    unsigned int axis_count = hb_ot_var_get_axis_count(hb_face); // hb 1.4.2
    std::vector<hb_ot_var_axis_info_t> axes_raw(axis_count);
    hb_ot_var_get_axis_infos(hb_face, 0, &axis_count, axes_raw.data()); // hb 2.2.0
    for (unsigned int i = 0; i < axis_count; ++i) {
        auto axis = axes_raw[i];
        char tag[5];
        hb_tag_to_string(axis.tag, tag);
        tag[4] = 0;
        tags.emplace_back(tag);
        defaults.push_back(axis.default_value);
    }

    for (unsigned int i = 0; i < names_count; ++i) {
        auto name = get_name_string(hb_face, hb_ot_var_named_instance_get_subfamily_name_id (hb_face, i));

        std::vector<float> coords(axis_count);
        std::map<std::string, float> axes;
        hb_ot_var_named_instance_get_design_coords(hb_face, i, &axis_count, coords.data());
        for (unsigned int j = 0; j < axis_count; ++j) {
            if (defaults[j] != coords[j]) {
                axes[tags[j]] = coords[j]; // Sorts alphabetically.
            }
        }

        Glib::ustring pango_string;
        for (auto [tag, value] : axes) {
            pango_string += tag + "=" + std::to_string(value);
            // Remove trailing zeros and decimal point
            pango_string = pango_string.substr(0, pango_string.find_last_not_of('0') + 1);
            if (pango_string.find('.') == pango_string.size() - 1) {
                pango_string = pango_string.substr(0, pango_string.size() - 1);
            }
            pango_string += ",";
        }

        // Remove trailing comma.
        if (!pango_string.empty()) {
            pango_string.erase (pango_string.size() - 1);
        }

        named_instance[name] = pango_string;
    }

    // std::cout << "readOpenTypeFvarNames: "
    //           << "Family: "    << std::setw(30) << std::left << get_name_string(hb_face, 1) << " "
    //           << "Subfamily: " << std::setw(30) << std::left << get_name_string(hb_face, 2) << " "
    //           << "count: " << names_count << std::endl;
    // std::cout << "            Preferred: "
    //           << "Family: "    << std::setw(30) << std::left << get_name_string(hb_face, 16) << " "
    //           << "Subfamily: " << std::setw(30) << std::left << get_name_string(hb_face, 17) << " "
    //           << std::endl;
    // for (auto i : named_instance) {
    //     std::cout << "  " << std::setw(20) << i.first << ": " << i.second << std::endl;
    // }
    // std::cout << "readOpenTypeFvarNames: Exit" << std::endl;
}

#define HB_OT_TAG_SVG HB_TAG('S','V','G',' ')

// Get SVG glyphs out of an OpenType font.
void readOpenTypeSVGTable(hb_font_t* hb_font,
                          std::map<unsigned int, SVGGlyphEntry>& glyphs,
                          std::map<int, std::string>& svgs) {

    hb_face_t* hb_face = hb_font_get_face (hb_font);

    // Harfbuzz has some support for SVG fonts but it is not exposed until version 2.1 (Oct 30, 2018).
    // We do it the hard way!
    hb_blob_t *hb_blob = hb_face_reference_table (hb_face, HB_OT_TAG_SVG);

    if (!hb_blob) {
        // No SVG table in font!
        return;
    }

    unsigned int svg_length = hb_blob_get_length (hb_blob);
    if (svg_length == 0) {
        // No SVG glyphs in table!
        return;
    }

    const char* data = hb_blob_get_data(hb_blob, &svg_length);
    if (!data) {
        std::cerr << "readOpenTypeSVGTable: Failed to get data! " << std::endl;
        return;
    }

    // OpenType fonts use Big Endian
    uint32_t offset  = ((data[2] & 0xff) << 24) + ((data[3] & 0xff) << 16) + ((data[4] & 0xff) << 8) + (data[5] & 0xff);

    // std::cout << "Offset: "  << offset << std::endl;
    // Bytes 6-9 are reserved.

    uint16_t entries = ((data[offset] & 0xff) << 8) + (data[offset+1] & 0xff);
    // std::cout << "Number of entries: " << entries << std::endl;

    for (int entry = 0; entry < entries; ++entry) {
        uint32_t base = offset + 2 + entry * 12;

        uint16_t startGlyphID = ((data[base  ] & 0xff) <<  8) + (data[base+1] & 0xff);
        uint16_t endGlyphID   = ((data[base+2] & 0xff) <<  8) + (data[base+3] & 0xff);
        uint32_t offsetGlyph  = ((data[base+4] & 0xff) << 24) + ((data[base+5] & 0xff) << 16) +((data[base+6]  & 0xff) << 8) + (data[base+7]  & 0xff);
        uint32_t lengthGlyph  = ((data[base+8] & 0xff) << 24) + ((data[base+9] & 0xff) << 16) +((data[base+10] & 0xff) << 8) + (data[base+11] & 0xff);

        // std::cout << "Entry " << entry << ": Start: " << startGlyphID << "  End: " << endGlyphID
        //           << "  Offset: " << offsetGlyph << " Length: " << lengthGlyph << std::endl;

        std::string svg;

        // static cast is needed as hb_blob_get_length returns char but we are comparing to a value greater than allowed by char.
        if (lengthGlyph > 1 && //
            static_cast<unsigned char>(data[offset + offsetGlyph + 0]) == 0x1f &&
            static_cast<unsigned char>(data[offset + offsetGlyph + 1]) == 0x8b) {
            // Glyph is gzipped

            std::vector<unsigned char> buffer;
            for (unsigned int c = offsetGlyph; c < offsetGlyph + lengthGlyph; ++c) {
                buffer.push_back(data[offset + c]);
            }

            Inkscape::IO::BufferInputStream zipped(buffer);
            Inkscape::IO::GzipInputStream gzin(zipped);
            for (int character = gzin.get(); character != -1; character = gzin.get()) {
               svg+= (char)character;
            }

        } else {
            // Glyph is not compressed

            for (unsigned int c = offsetGlyph; c < offsetGlyph + lengthGlyph; ++c) {
                svg += (unsigned char) data[offset + c];
            }
        }

        // Make all glyphs hidden (for SVG files with multiple glyphs, we'll need to pickout just one).
        static auto regex = Glib::Regex::create("(id=\"\\s*glyph\\d+\\s*\")", Glib::Regex::CompileFlags::OPTIMIZE);
        svg = regex->replace(Glib::UStringView(svg), 0, "\\1 visibility=\"hidden\"", static_cast<Glib::Regex::MatchFlags>(0));

        svgs[entry] = svg;

        for (unsigned int i = startGlyphID; i < endGlyphID+1; ++i) {
            glyphs[i].entry_index = entry;
        }

        // for (auto const& glyph : glyphs) {
        //     std::cout << "Glyph: " << glyph.first << std::endl;
        //     auto length = svgs[glyph.second.entry_index].length();
        //     if (length < 1000) {
        //         std::cout << svgs[glyph.second.entry_index] << std::endl;
        //     } else {
        //         std::cout << "glyph svg string length: " << length << std::endl;
        //     }
        // }
    }
}

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
