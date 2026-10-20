// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_OPENTYPEUTIL_H
#define SEEN_OPENTYPEUTIL_H

#include <string>
#ifndef USE_PANGO_WIN32

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

#include <glibmm/ustring.h>

/*
 * A set of utilities to extract data from OpenType fonts.
 *
 * Isolates dependencies on FreeType, Harfbuzz, and Pango.
 * All three provide variable amounts of access to data.
 */

struct hb_font_t;

// OpenType substitution
struct OTSubstitution
{
    Glib::ustring before;
    Glib::ustring input;
    Glib::ustring after;
    Glib::ustring output;
};

// An OpenType fvar axis.
struct OTVarAxis
{
    bool operator==(OTVarAxis const &other) const = default;

    // compare axis definition, ignore set value
    bool same_definition(const OTVarAxis& other) const {
        return
            tag     == other.tag     &&
            name    == other.name    &&
            minimum == other.minimum &&
            def     == other.def     &&
            maximum == other.maximum;
    }

    std::string tag;
    Glib::ustring name;
    double minimum = 0;
    double def = 500; // Default
    double maximum = 1000;
    double set_val = 500;
};

struct SVGGlyphEntry
{
    unsigned entry_index;
    ~SVGGlyphEntry();
};

void readOpenTypeTableList (hb_font_t* hb_font,
                            std::unordered_set<std::string>& list);

// This would be better if one had std::vector<OTSubstitution> instead of OTSubstitution where each
// entry corresponded to one substitution (e.g. ff -> ﬀ) but Harfbuzz at the moment cannot return
// individual substitutions. See Harfbuzz issue #673.
void readOpenTypeGsubTable (hb_font_t* hb_font,
                            std::map<Glib::ustring, OTSubstitution >& tables);

// Read the variation font axes parameters including set values. Axis order is defined by font.
void readOpenTypeFvarAxes  (hb_font_t* hb_font,
                            std::vector<OTVarAxis>& axes);

// Construct a map of variable font named instances with names as key and corresponding Pango string as data.
void readOpenTypeFvarNamedInstances (hb_font_t* hb_font, std::map<Glib::ustring, Glib::ustring>& named_instances);

void readOpenTypeSVGTable  (hb_font_t* hb_font,
                            std::map<unsigned int, SVGGlyphEntry>& glyphs,
                            std::map<int, std::string>& svgs);

#endif /* !USE_PANGO_WIND32    */
#endif /* !SEEN_OPENTYPEUTIL_H */

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
