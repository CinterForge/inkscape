// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef LIBNRTYPE_FONT_FACTORY_H
#define LIBNRTYPE_FONT_FACTORY_H

#include <cstddef>
#include <functional>
#include <algorithm>
#include <glibmm/refptr.h>
#include <pangomm/fontfamily.h>
#include <pangomm/fontmap.h>
#include <utility>
#include <memory>
#include <map>

#include <pango/pango.h>
#include <pango/pangoft2.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "util/statics.h"
#include "util/cached_map.h"

class FontInstance;

// Wraps calls to pango_font_description_get_family with some name substitution
char const *sp_font_description_get_family(PangoFontDescription const *fontDescr);

// Map a non-existent font name to an existing one.
std::string getSubstituteFontName(std::string const &font);

// Class for style strings: both CSS and as suggested by font.
struct StyleNames
{
    StyleNames() = default;
    StyleNames(Glib::ustring name) : StyleNames{name, std::move(name)} {}
    StyleNames(Glib::ustring cssname, Glib::ustring displayname)
        : css_name{std::move(cssname)}
        , display_name{std::move(displayname)}
    {}

    Glib::ustring css_name;     // Style as Pango/CSS would write it.
    Glib::ustring display_name; // Style as Font designer named it.
};

class FontFactory
    /*
     * Using statics.h to ensure destruction before main() exits, otherwise Harfbuzz's internal
     * FreeType instance will come before us in the static destruction order and our destructor will crash.
     * Related - https://gitlab.com/inkscape/inkscape/-/issues/3765.
     */
    : public Inkscape::Util::EnableSingleton<FontFactory>
{
public:
    // Refresh pango font configuration
    void refreshConfig();

    ///< The fontsize used as workaround for hinting.
    static constexpr double fontSize = 512;

    std::vector<std::string> GetAllFontNames();

    /// Returns strings to be used in the UI for family and face (or "style" as the column is labeled)
    Glib::ustring GetUIFamilyString(PangoFontDescription const *fontDescr);
    Glib::ustring GetUIStyleString(PangoFontDescription const *fontDescr);
    bool hasFontFamily(std::string const &family);

    // Helpfully returns all font families in a map.
    std::map<std::string, Glib::RefPtr<Pango::FontFamily>> GetUIFamilies();
    // Retrieves style information about a font family.
    std::vector<StyleNames> GetUIStyles(const Glib::RefPtr<Pango::FontFamily> &in);

    std::vector<Glib::RefPtr<Pango::FontFamily>> get_font_families();

    // Various functions to get a FontInstance from different descriptions.
    std::shared_ptr<FontInstance> FaceFromDescr(char const *family, char const *style);
    std::shared_ptr<FontInstance> FaceFromPangoString(char const *pangoString);
    std::shared_ptr<FontInstance> FaceFromFontSpecification(char const *fontSpecification);
    std::shared_ptr<FontInstance> Face(PangoFontDescription *descr, bool canFail = true);

    std::unique_ptr<FontInstance> create_face(PangoFontDescription* descr);

# ifdef _WIN32
    void AddFontFilesWin32(char const *directory_path);
# endif

    /// Add a directory from which to include additional fonts
    void AddFontsDir(char const *utf8dir);

    /// Add an additional font.
    void AddFontFile(char const *utf8file);

    /// Add a new font config file
    void AddFontConfig(char const *utf8file);

    PangoContext *get_font_context() const { return fontContext; }
    PangoFontDescription *parsePostscriptName(std::string const &name, bool substitute);

private:
    FontFactory();
    ~FontFactory();
    friend class EnableSingleton;

    // Pango data. Backend-specific structures are cast to these opaque types.
    PangoFontMap *fontServer;
    PangoContext *fontContext;
    FcConfig *fontConfig;
    Glib::RefPtr<Pango::FontMap> _font_map;

    // A hashmap of all the loaded font instances, indexed by their PangoFontDescription.
    // Note: Since pango already does that, using the PangoFont could work too.
    struct Hash
    {
        size_t operator()(PangoFontDescription const *x) const;
    };
    struct Compare
    {
        bool operator()(PangoFontDescription const *a, PangoFontDescription const *b) const;
    };
    Inkscape::Util::cached_map<PangoFontDescription*, FontInstance, Hash, Compare> loaded;

    // The following two commented out maps were an attempt to allow Inkscape to use font faces
    // that could not be distinguished by CSS values alone. In practice, they never were that
    // useful as PangoFontDescription, which is used throughout our code, cannot distinguish
    // between faces anymore than raw CSS values (with the exception of two additional weight
    // values).
    //
    // During various works, for example to handle font-family lists and fonts that are not
    // installed on the system, the code has become less reliant on these maps. And in the work to
    // cache style information to speed up start up times, the maps were not being filled.
    // I've removed all code that used these maps as of Oct 2014 in the experimental branch.
    // The commented out maps are left here as a reminder of the path that was attempted.
    //
    // One possible method to keep track of font faces would be to use the 'display name', keeping
    // pointers to the appropriate PangoFontFace. The FontFactory loadedFaces map indexing would
    // have to be changed to incorporate 'display name' (InkscapeFontDescription?).


    // These two maps are used for translating between what's in the UI and a pango
    // font description. This is necessary because Pango cannot always
    // reproduce these structures from the names it gave us in the first place.

    // Key: A string produced by FontFactory::ConstructFontSpecification
    // Value: The associated PangoFontDescription
    // typedef std::map<Glib::ustring, PangoFontDescription *> PangoStringToDescrMap;
    // PangoStringToDescrMap fontInstanceMap;

    // Key: Family name in UI + Style name in UI
    // Value: The associated string that should be produced with FontFactory::ConstructFontSpecification
    // typedef std::map<Glib::ustring, Glib::ustring> UIStringToPangoStringMap;
    // UIStringToPangoStringMap fontStringMap;
};

#endif // LIBNRTYPE_FONT_FACTORY_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
