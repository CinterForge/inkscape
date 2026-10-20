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

#ifndef POPPLER_UTILS_H
#define POPPLER_UTILS_H

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include <goo/GooString.h>

#include "poppler-transition-api.h"

namespace Geom {
class Affine;
}
class Array;
class Dict;
class FNVHash;
class GfxFont;
class GfxState;
class GooString;
class Object;
class PDFDoc;
class Ref;
class XRef;

Geom::Affine stateToAffine(GfxState *state);
// this function is for Poppler older than v25.09.0
Geom::Affine ctmToAffine(const double *ctm);
// this flavor is for Poppler v25.09.0 and above
Geom::Affine ctmToAffine(const std::array<double, 6>& ctm);

void ctmout(const char *label, const double *ctm);
void affout(const char *label, Geom::Affine affine);

void pdf_debug_array(const Array *array, int depth = 0, XRef *xref = nullptr);
void pdf_debug_dict(const Dict *dict, int depth = 0, XRef *xref = nullptr);
void pdf_debug_object(const Object *obj, int depth = 0, XRef *xref = nullptr);

#if POPPLER_CHECK_VERSION(22, 4, 0)
typedef std::shared_ptr<GfxFont> FontPtr;
#else
typedef GfxFont *FontPtr;
#endif

class FontData
{
public:
    FontData(FontPtr font);
    std::string getSubstitute() const;
    std::string getSpecification() const;

    bool found = false;

    std::unordered_set<int> pages;
    std::string name;
    std::string family;

    std::string style;
    std::string weight;
    std::string stretch;
    std::string variation;

private:
    void _parseStyle();
};

typedef std::shared_ptr<std::map<FontPtr, FontData>> FontList;

FontList getPdfFonts(std::shared_ptr<PDFDoc> pdf_doc);
std::string getNameWithoutSubsetTag(std::string name);
std::string getDictString(Dict *dict, const char *key);
std::string getString(const std::string &value);
std::string getString(const std::optional<std::string> &value);
std::string getString(const std::unique_ptr<GooString> &value);
std::string getString(const GooString *value);
std::string getString(const GooString &value);
std::string validateString(std::string const &in);
std::string sanitizeId(std::string const &in);

// Replacate poppler FontDict
class InkFontDict
{
public:
    // Build the font dictionary, given the PDF font dictionary.
    InkFontDict(XRef *xref, Ref *fontDictRef, Dict *fontDict);

    // Iterative access.
    int getNumFonts() const { return fonts.size(); }

    // Get the specified font.
    FontPtr lookup(const char *tag) const;
    FontPtr getFont(int i) const { return fonts[i]; }
    std::vector<FontPtr> fonts;

private:
    int hashFontObject(Object *obj);
    void hashFontObject1(const Object *obj, FNVHash *h);
};

inline size_t get_goostring_length(const GooString& str) {
#if POPPLER_CHECK_VERSION(25, 10, 0)
    return str.size();
#else
    return str.getLength();
#endif
}

#endif /* POPPLER_UTILS_H */
