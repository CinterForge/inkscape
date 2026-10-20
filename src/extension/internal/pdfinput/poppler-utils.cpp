// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * PDF parsing utilities for libpoppler.
 *//*
 * Authors:
 *    Martin Owens
 * 
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "poppler-utils.h"

#include <UTF.h>

#include <2geom/affine.h>
#include <GfxFont.h>
#include <GfxState.h>
#include <PDFDoc.h>
#include <PDFDocEncoding.h>
#include "libnrtype/font-factory.h"

/**
 * Get the default transformation state from the GfxState
 */
Geom::Affine stateToAffine(GfxState *state)
{
    return ctmToAffine(state->getCTM());
}

/**
 * Convert a transformation matrix to a lib2geom affine object.
 */
Geom::Affine ctmToAffine(const double *ctm)
{
    if (!ctm)
        return Geom::identity();
    return Geom::Affine(ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
}

Geom::Affine ctmToAffine(const std::array<double, 6>& ctm) {
    return ctmToAffine(ctm.data());
}

void ctmout(const char *label, const double *ctm)
{
    std::cout << "C:" << label << ":" << ctm[0] << "," << ctm[1] << "," << ctm[2] << "," << ctm[3] << "," << ctm[4]
              << "," << ctm[5] << "\n";
}

void affout(const char *label, Geom::Affine ctm)
{
    std::cout << "A:" << label << ":" << ctm[0] << "," << ctm[1] << "," << ctm[2] << "," << ctm[3] << "," << ctm[4]
              << "," << ctm[5] << "\n";
}

//------------------------------------------------------------------------
// GfxFontDict from GfxFont.cc in poppler 22.09
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// See poppler source code for full list of copyright holders.
//------------------------------------------------------------------------

InkFontDict::InkFontDict(XRef *xref, Ref *fontDictRef, Dict *fontDict)
{
    Ref r;

    fonts.resize(fontDict->getLength());
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        const Object &obj1 = fontDict->getValNF(i);
        Object obj2 = obj1.fetch(xref);
        if (obj2.isDict()) {
            if (obj1.isRef()) {
                r = obj1.getRef();
            } else if (fontDictRef) {
                // legal generation numbers are five digits, so we use a
                // 6-digit number here
                r.gen = 100000 + fontDictRef->num;
                r.num = i;
            } else {
                // no indirect reference for this font, or for the containing
                // font dict, so hash the font and use that
                r.gen = 100000;
                r.num = hashFontObject(&obj2);
            }
            // Newer poppler will require some reworking as it gives a shared ptr.
#if POPPLER_CHECK_VERSION(26,3,0)
            fonts[i] = GfxFont::makeFont(xref, fontDict->getKey(i), r, *obj2.getDict());
#else
            fonts[i] = GfxFont::makeFont(xref, fontDict->getKey(i), r, obj2.getDict());
#endif
            if (fonts[i] && !fonts[i]->isOk()) {
                fonts[i] = nullptr;
            }
        } else {
            error(errSyntaxError, -1, "font resource is not a dictionary");
            fonts[i] = nullptr;
        }
    }
}

FontPtr InkFontDict::lookup(const char *tag) const
{
    for (const auto &font : fonts) {
        if (font && font->matches(tag)) {
            return font;
        }
    }
    return nullptr;
}

// FNV-1a hash
class FNVHash
{
public:
    FNVHash() { h = 2166136261U; }

    void hash(char c)
    {
        h ^= c & 0xff;
        h *= 16777619;
    }

    void hash(const char *p, int n)
    {
        int i;
        for (i = 0; i < n; ++i) {
            hash(p[i]);
        }
    }

    int get31() { return (h ^ (h >> 31)) & 0x7fffffff; }

private:
    unsigned int h;
};

int InkFontDict::hashFontObject(Object *obj)
{
    FNVHash h;

    hashFontObject1(obj, &h);
    return h.get31();
}

void InkFontDict::hashFontObject1(const Object *obj, FNVHash *h)
{
    const char *p;
    double r;
    int n, i;

    switch (obj->getType()) {
        case objBool:
            h->hash('b');
            h->hash(obj->getBool() ? 1 : 0);
            break;
        case objInt:
            h->hash('i');
            n = obj->getInt();
            h->hash((char *)&n, sizeof(int));
            break;
        case objReal:
            h->hash('r');
            r = obj->getReal();
            h->hash((char *)&r, sizeof(double));
            break;
        case objString: {
            h->hash('s');
#if POPPLER_CHECK_VERSION(26, 4, 0)
            const auto &s = obj->getString();
            h->hash(s.c_str(), s.size());
#else
            const GooString* s = obj->getString();
            h->hash(s->c_str(), get_goostring_length(*s));
#endif
            } break;
        case objName:
            h->hash('n');
            p = obj->getName();
            h->hash(p, (int)strlen(p));
            break;
        case objNull:
            h->hash('z');
            break;
        case objArray: {
                h->hash('a');
                Array * objArray = obj->getArray();
                n = objArray->getLength();
                h->hash((char *)&n, sizeof(int));
                for (i = 0; i < n; ++i) {
                    const Object &obj2 = objArray->getNF(i);
                    hashFontObject1(&obj2, h);
                }
            } break;
        case objDict: {
                h->hash('d');
                auto objdict = obj->getDict();
                n = objdict->getLength();
                h->hash((char *)&n, sizeof(int));
                for (i = 0; i < n; ++i) {
                    auto p = std::string(objdict->getKey(i));
                    h->hash(p.c_str(), p.length());
                    const Object &obj2 = objdict->getValNF(i);
                    hashFontObject1(&obj2, h);
                }
            } break;
        case objStream:
            // this should never happen - streams must be indirect refs
            break;
        case objRef:
            h->hash('f');
            n = obj->getRefNum();
            h->hash((char *)&n, sizeof(int));
            n = obj->getRefGen();
            h->hash((char *)&n, sizeof(int));
            break;
        default:
            h->hash('u');
            break;
    }
}

std::string getNameWithoutSubsetTag(std::string name)
{
    // there should only be a single tag AAAAAA+FontName, but in edge cases there may
    // be multiple. This also relaxes the uppercase letter specification.
    auto start = name.begin();

    for (auto iter = name.begin(); iter != name.end(); ++iter) {
        int diff = iter - start;
        if (*iter == '+' && diff == 6) {
            // found the plus sign, shift the start to the next char
            start = iter + 1;
        } else if (iter - start > 6 || ((*iter < '0' || *iter > '9') && 
                                        (*iter < 'A' || *iter > 'Z') && 
                                        (*iter < 'a' || *iter > 'z'))) {
            break; // not a valid postScript Tag
        }
    }

    return std::string(start, name.end());
}

/**
 * Extract all the useful information from the GfxFont object
 */
FontData::FontData(FontPtr font)
{
    // Level one parsing is taking the data from the PDF font, although this
    // information is almost always missing. Perhaps sometimes it's not.
    found = false;

    // Style: italic, oblique, normal
    style = font->isItalic() ? "italic" : "";

    // Weight: normal, bold, etc
    weight = "normal";
    switch (font->getWeight()) {
        case GfxFont::WeightNotDefined:
            break;
        case GfxFont::W400:
            weight = "normal";
            break;
        case GfxFont::W700:
            weight = "bold";
            break;
        default:
            weight = std::to_string(font->getWeight() * 100);
            break;
    }

    // Stretch: condensed or expanded
    stretch = "";
    switch (font->getStretch()) {
        case GfxFont::UltraCondensed:
            stretch = "ultra-condensed";
            break;
        case GfxFont::ExtraCondensed:
            stretch = "extra-condensed";
            break;
        case GfxFont::Condensed:
            stretch = "condensed";
            break;
        case GfxFont::SemiCondensed:
            stretch = "semi-condensed";
            break;
        case GfxFont::Normal:
            stretch = "normal";
            break;
        case GfxFont::SemiExpanded:
            stretch = "semi-expanded";
            break;
        case GfxFont::Expanded:
            stretch = "expanded";
            break;
        case GfxFont::ExtraExpanded:
            stretch = "extra-expanded";
            break;
        case GfxFont::UltraExpanded:
            stretch = "ultra-expanded";
            break;
    }

    if (font->getName()) {
        name = validateString(getNameWithoutSubsetTag(font->getName()->c_str()));
    } else {
        g_warning("Font has no name");
        name = "";
    }

    PangoFontDescription *desc = FontFactory::get().parsePostscriptName(name, false);

    if (!desc && font->getFamily()) {
        // Level two parsing, we break off the font description part of the name
        // which often contains font data and use it as a pango font description.
        std::string pdf_family = validateString(font->getFamily()->c_str());
        std::string desc_str = pdf_family;
        auto pos = name.find("-");
        if (pos != std::string::npos) {
            // Insert spaces where we see capital letters.
            std::stringstream ret;
            auto str = name.substr(pos + 1, name.size());
            for (char l : str) {
                if (l >= 'A' && l <= 'Z')
                    ret << " ";
                ret << l;
            }
            desc_str = desc_str + ret.str();
        }
        desc = pango_font_description_from_string(desc_str.c_str());
        if (!desc) {
            // Sometimes it's possible to match the description string directly.
            desc = pango_font_description_from_string(pdf_family.c_str());
        }
    }

    if (desc) {
        // Now we pull data out of the description.
        auto new_family = pango_font_description_get_family(desc);
        if (new_family && FontFactory::get().hasFontFamily(new_family)) {
            family = new_family;

            // Style from pango description
            switch (pango_font_description_get_style(desc)) {
                case PANGO_STYLE_ITALIC:
                    style = "italic";
                    break;
                case PANGO_STYLE_OBLIQUE:
                    style = "oblique";
                    break;
            }

            // Weight from pango description
            auto pw = pango_font_description_get_weight(desc);
            if (pw != PANGO_WEIGHT_NORMAL) {
                weight = std::to_string(pw); // Number 100-1000
            }

            // Stretch from pango description
            switch (pango_font_description_get_stretch(desc)) {
                case PANGO_STRETCH_ULTRA_CONDENSED:
                    stretch = "ultra-condensed";
                    break;
                case PANGO_STRETCH_EXTRA_CONDENSED:
                    stretch = "extra-condensed";
                    break;
                case PANGO_STRETCH_CONDENSED:
                    stretch = "condensed";
                    break;
                case PANGO_STRETCH_SEMI_CONDENSED:
                    stretch = "semi-condensed";
                    break;
                case PANGO_STRETCH_SEMI_EXPANDED:
                    stretch = "semi-expanded";
                    break;
                case PANGO_STRETCH_EXPANDED:
                    stretch = "expanded";
                    break;
                case PANGO_STRETCH_EXTRA_EXPANDED:
                    stretch = "extra-expanded";
                    break;
                case PANGO_STRETCH_ULTRA_EXPANDED:
                    stretch = "ultra-expanded";
                    break;
            }

            // variant = TODO Convert to variant pango_font_description_get_variant(desc)

            found = true;
            // All information has been processed, don't over-write with level three.
            return;
        }
    }

    // Level three parsing, we take our name and attempt to match known style names
    // Copy id-name stored in PDF and make it lower case and strip whitespaces
    std::string source = name;
    transform(source.begin(), source.end(), source.begin(), ::tolower);
    source.erase(std::remove_if(source.begin(), source.end(), ::isspace), source.end());
    auto contains = [=](const std::string &other) { return source.find(other) != std::string::npos; };

    if (contains("italic") || contains("slanted")) {
        style = "italic";
    } else if (contains("oblique")) {
        style = "oblique";
    }

    // Ordered by string matching pass through.
    static std::map<std::string, std::string> weights{
        // clang-format off
        {"bold",        "bold"},
        {"ultrabold",   "800"},
        {"extrabold",   "800"},
        {"demibold",    "600"},
        {"semibold",    "600"},
        {"thin",        "100"},
        {"ultralight",  "200"},
        {"extralight",  "200"},
        {"light",       "300"},
        {"black",       "900"},
        {"heavy",       "900"},
        {"medium",      "500"},
        {"book",        "normal"},
        {"regular",     "normal"},
        {"roman",       "normal"},
        {"normal",      "normal"},
        // clang-format on
    };
    // Apply the font weight translations
    for (auto w : weights) {
        if (contains(w.first))
            weight = w.second;
    }

    static std::map<std::string, std::string> stretches{
        // clang-format off
        {"ultracondensed", "ultra-condensed"},
        {"extracondensed", "extra-condensed"},
        {"semicondensed", "semi-condensed"},
        {"condensed", "condensed"},
        {"ultraexpanded", "ultra-expanded"},
        {"extraexpanded", "extra-expanded"},
        {"semiexpanded", "semi-expanded"},
        {"expanded", "expanded"},
        // clang-format on
    };
    // Apply the font weight translations
    for (auto s : stretches) {
        if (contains(s.first))
            stretch = s.second;
    }
}

/*
 * Scan the available fonts to find the font name that best match.
 *
 * If nothing can be matched, returns an empty string.
 */
std::string FontData::getSubstitute() const
{
    if (found)
        return "";

    if (auto desc = FontFactory::get().parsePostscriptName(name, true)) {
        auto new_family = pango_font_description_get_family(desc);
        if (FontFactory::get().hasFontFamily(new_family)) {
            return new_family;
        }
    }
    return "sans";
}

/*
 * Used to determine if any font property has changed by comparing
 * font specifications.
 */
std::string FontData::getSpecification() const
{
    if (auto desc = FontFactory::get().parsePostscriptName(name, false)) {
        char *copyAsString = pango_font_description_to_string(desc);
        std::string pangoString = copyAsString;
        g_free(copyAsString);
        return pangoString;
    } else {
        return
            family +
            (weight  == "normal" ? "" : " " + weight) +
            (style.empty()   ? "" : " " + style) +
            (stretch.empty() ? "" : " " + stretch) +
            (variation.empty() ? "" : " " + variation);
    }
}

//------------------------------------------------------------------------
// scanFonts from FontInfo.cc
//------------------------------------------------------------------------

void _getFontsRecursive(std::shared_ptr<PDFDoc> pdf_doc, Dict *resources, const FontList &fontsList,
                        std::set<int> &visitedObjects, int page)
{
    assert(resources);
    auto xref = pdf_doc->getXRef();

    InkFontDict *fontDict = nullptr;
    const Object &obj1 = resources->lookupNF("Font");
    if (obj1.isRef()) {
        Object obj2 = obj1.fetch(xref);
        if (obj2.isDict()) {
            auto r = obj1.getRef();
            fontDict = new InkFontDict(xref, &r, obj2.getDict());
        }
    } else if (obj1.isDict()) {
        fontDict = new InkFontDict(xref, nullptr, obj1.getDict());
    }

    if (fontDict) {
        for (int i = 0; i < fontDict->getNumFonts(); ++i) {
            auto font = fontDict->getFont(i);
            if (fontsList->find(font) == fontsList->end()) {
                // Create new font data
                fontsList->emplace(font, FontData(font));
            }
            fontsList->at(font).pages.insert(page);
        }
    }

    // recursively scan any resource dictionaries in objects in this resource dictionary
    const char *resTypes[] = {"XObject", "Pattern"};
    for (const char *resType : resTypes) {
        Object objDict = resources->lookup(resType);
        if (!objDict.isDict())
            continue;

        for (int i = 0; i < objDict.dictGetLength(); ++i) {
            Ref obj2Ref;
            const Object obj2 = objDict.getDict()->getVal(i, &obj2Ref);
            if (obj2Ref != Ref::INVALID() && !visitedObjects.insert(obj2Ref.num).second)
                continue;

            if (!obj2.isStream())
                continue;

            Ref resourcesRef;
            const Object resObj = obj2.getStream()->getDict()->lookup("Resources", &resourcesRef);
            if (resourcesRef != Ref::INVALID() && !visitedObjects.insert(resourcesRef.num).second)
                continue;

            if (resObj.isDict() && resObj.getDict() != resources) {
                _getFontsRecursive(pdf_doc, resObj.getDict(), fontsList, visitedObjects, page);
            }
        }
    }
}

FontList getPdfFonts(std::shared_ptr<PDFDoc> pdf_doc)
{
    auto fontsList = std::make_shared<std::map<FontPtr, FontData>>();
    auto count = pdf_doc->getCatalog()->getNumPages();
    std::set<int> visitedObjects;

    for (auto page_num = 1; page_num <= count; page_num++) {
        auto page = pdf_doc->getCatalog()->getPage(page_num);
        auto resources = page->getResourceDict();

        if (resources) {
            _getFontsRecursive(pdf_doc, resources, fontsList, visitedObjects, page_num);
        }
    }
    return fontsList;
}

/**
 * Convert arbitrary string (e.g. group name in PDF) to a valid SVG ID.
 *
 * This function guarantees the following:
 * - The result is a valid SVG ID.
 * - Two different inputs can never lead to the same output, so ID collisions are avoided.
 *   (Mathematically, the function is invertible.)
 * - Valid SVG IDs are preserved if they only use the characters a-z A-Z 0-9.
 *
 * It does *not* guarantee that all other valid input SVG IDs are preserved.
 * (This would be impossible together with the above guarantees.)
 *
 * See also sanitize_id() in id-clash.cpp for a less aggressive version that is, however, not collision-free.
 */
std::string sanitizeId(std::string const &in)
{
    // XML allows IDs of the form [a-zA-Z_:][a-zA-Z0-9\-_\.:]* plus some UTF8 characters.
    // Here we restrict us to the subset [a-zA-Z_][a-zA-Z0-9_]*,
    // where "_" is used as escape character.
    // https://www.w3.org/TR/2008/REC-xml-20081126/#id
    // https://stackoverflow.com/questions/1077084/what-characters-are-allowed-in-dom-ids#1077111

    if (in.empty()) {
        return "_";
    }
    if (isalpha(in[0]) && std::find_if_not(in.begin(), in.end(), isalnum) == in.end()) {
        [[likely]];
        // Fast path:
        // Input is of the form [a-zA-Z][a-zA-Z0-9]*
        // --> Return unchanged.
        return in;
    }

    // Slow path: Escape anything non-alphanumeric,
    //  e.g., "a bc" as a_20bc
    std::ostringstream outStream;
    for (char chr : in) {
        if (isalnum(chr)) {
            outStream.put(chr);
        } else {
            outStream.put('_');
            outStream << std::hex << ((unsigned int)chr & 0xff);
        }
    }
    return outStream.str();
}

/**
 * Ensure string is valid UTF8.
 * If not, return an empty string.
 * (This could be changed in the future to only remove the problematic characters.)
 */
std::string validateString(std::string const &in)
{
    if (g_utf8_validate(in.c_str(), -1, nullptr)) {
        return in;
    }
    g_warning("Couldn't parse strings in the PDF, there may be errors.");
    return "";
}

/**
 * Get a string from a dictionary. If the string doesn't exist, return empty string.
 */
std::string getDictString(Dict *dict, const char *key)
{
    Object obj = dict->lookup(key);

    if (!obj.isString()) {
        return "";
    }
    return getString(obj.getString());
}

std::string getString(const std::optional<std::string> &value)
{
    return value ? getString(*value) : "";
}

std::string getString(const std::unique_ptr<GooString> &value)
{
    return getString(value.get());
}

std::string getString(const GooString *value)
{
    if (value) {
        return getString(value->toStr());
    }
    return "";
}

std::string getString(const GooString &value)
{
    return getString(value.toStr());
}

/**
 * Convert PDF strings, which can be formatted as UTF8, UTF16BE or UTF16LE into
 * a predictable UTF8 string consistant with svg requirements.
 */
std::string getString(const std::string &value)
{
    char *str = nullptr;
#if !POPPLER_CHECK_VERSION(25,02,0)
    int stringLength;
#endif

    if (_POPPLER_HAS_UNICODE_BOM(value)) {
        str = g_convert(value.c_str() + 2, value.size() - 2,
                        "UTF-8", "UTF-16BE", NULL, NULL, NULL);
    } else if (_POPPLER_HAS_UNICODE_BOMLE(value)) {
        str = g_convert(value.c_str() + 2, value.size() - 2,
                        "UTF-8", "UTF-16LE", NULL, NULL, NULL);
    }
#if POPPLER_CHECK_VERSION(25,02,0)
    else if (auto utf16 = pdfDocEncodingToUTF16(value); !utf16.empty())  {
        str = g_convert(utf16.c_str(), utf16.length(), "UTF-8", "UTF-16", NULL, NULL, NULL);
    }
#else
    else if (auto utf16 = pdfDocEncodingToUTF16(value, &stringLength))  {
        str = g_convert(utf16, stringLength, "UTF-8", "UTF-16", NULL, NULL, NULL);
        delete[] utf16;
    }
#endif
    if (str) {
        std::string copy = str;
        g_free(str);
        return copy;
    }
    g_warning("Couldn't parse text in PDF from UTF16.");
    return "";
}

void pdf_debug_array(const Array *array, int depth, XRef *xref)
{
    if (depth > 20) {
        std::cout << "[ ... ]";
        return;
    }
    std::cout << "[\n";
    for (int i = 0; i < array->getLength(); ++i) {
        for (int x = depth; x > -1; x--)
            std::cout << " ";
        std::cout << i << ": ";
        Object obj = array->get(i);
        pdf_debug_object(&obj, depth + 1, xref);
        std::cout << ",\n";
    }
    for (int x = depth; x > 0; x--)
        std::cout << " ";
    std::cout << "]";
}

void pdf_debug_dict(const Dict *dict, int depth, XRef *xref)
{
    if (depth > 20) {
        std::cout << "{ ... }";
        return;
    }
    std::cout << "{\n";
    for (auto j = 0; j < dict->getLength(); j++) {
        auto key = dict->getKey(j);
        auto val = dict->getVal(j);
        for (int x = depth; x > -1; x--)
            std::cout << " ";
        std::cout << key << ": ";
        pdf_debug_object(&val, depth + 1, xref);
        std::cout << ",\n";
    }
    for (int x = depth; x > 0; x--)
        std::cout << " ";
    std::cout << "}";
}

void pdf_debug_object(const Object *obj, int depth, XRef *xref)
{
    if (obj->isRef()) {
        std::cout << " > REF(" << obj->getRef().num << "):";
        if (xref) {
            auto ref = obj->fetch(xref);
            pdf_debug_object(&ref, depth + 1, xref);
        }
    } else if (obj->isDict()) {
        pdf_debug_dict(obj->getDict(), depth, xref);
    } else if (obj->isArray()) {
        pdf_debug_array(obj->getArray(), depth, xref);
    } else if (obj->isString()) {
#if POPPLER_CHECK_VERSION(26, 4, 0)
        std::cout << " STR '" << obj->getString().c_str() << "'";
#else
        std::cout << " STR '" << obj->getString()->getCString() << "'";
#endif
    } else if (obj->isName()) {
        std::cout << " NAME '" << obj->getName() << "'";
    } else if (obj->isBool()) {
        std::cout << " BOOL " << (obj->getBool() ? "true" : "false");
    } else if (obj->isNum()) {
        std::cout << " NUM " << obj->getNum();
    } else {
        std::cout << " > ? " << obj->getType() << "";
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
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
