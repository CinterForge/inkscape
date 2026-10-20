// SPDX-License-Identifier: GPL-2.0-or-later
//========================================================================
//
// CairoFontEngine.cc
//
// Copied into Inkscape from poppler-22.09.0 2022
//   - poppler/CairoFontEngine.*
//   - goo/ft_utils.*
//
// Copyright 2003 Glyph & Cog, LLC
// Copyright 2004 Red Hat, Inc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005-2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005 Martin Kretzschmar <martink@gnome.org>
// Copyright (C) 2005, 2009, 2012, 2013, 2015, 2017-2019, 2021, 2022 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006, 2007, 2010, 2011 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2007 Koji Otani <sho@bbr.jp>
// Copyright (C) 2008, 2009 Chris Wilson <chris@chris-wilson.co.uk>
// Copyright (C) 2008, 2012, 2014, 2016, 2017, 2022 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2009 Darren Kenny <darren.kenny@sun.com>
// Copyright (C) 2010 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2010 Jan Kümmel <jan+freedesktop@snorc.org>
// Copyright (C) 2012 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2015, 2016 Jason Crain <jason@aquaticape.us>
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 Christian Persch <chpe@src.gnome.org>
// Copyright (C) 2020 Michal <sudolskym@gmail.com>
// Copyright (C) 2021, 2022 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2022 Marcel Fabian Krüger <tex@2krueger.de>
//
//========================================================================

#include "poppler-cairo-font-engine.h"

#include <config.h>
#include <cstring>
#include <fofi/FoFiTrueType.h>
#include <fofi/FoFiType1C.h>

#include <Error.h>
#include <Gfx.h>
#include <GlobalParams.h>
#include <Page.h>
#include <XRef.h>

//========================================================================
//
// ft_util.cc
//
// FreeType helper functions.
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2022 Adrian Johnson <ajohnson@redneon.com>
//
//========================================================================

#include <cstdio>

FT_Error ft_new_face_from_file(FT_Library library, const char *filename_utf8, FT_Long face_index, FT_Face *aface);

#ifdef _WIN32
static unsigned long ft_stream_read(FT_Stream stream, unsigned long offset, unsigned char *buffer, unsigned long count)
{
    FILE *file = (FILE *)stream->descriptor.pointer;
    fseek(file, offset, SEEK_SET);
    return fread(buffer, 1, count, file);
}

static void ft_stream_close(FT_Stream stream)
{
    FILE *file = (FILE *)stream->descriptor.pointer;
    fclose(file);
    delete stream;
}
#endif

// Same as FT_New_Face() but handles UTF-8 filenames on Windows
FT_Error ft_new_face_from_file(FT_Library library, const char *filename_utf8, FT_Long face_index, FT_Face *aface)
{
#ifdef _WIN32
    FILE *file;
    long size;

    if (!filename_utf8)
        return FT_Err_Invalid_Argument;

    file = openFile(filename_utf8, "rb");
    if (!file)
        return FT_Err_Cannot_Open_Resource;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    if (size <= 0)
        return FT_Err_Cannot_Open_Stream;

    FT_StreamRec *stream = new FT_StreamRec;
    *stream = {};
    stream->size = size;
    stream->read = ft_stream_read;
    stream->close = ft_stream_close;
    stream->descriptor.pointer = file;

    FT_Open_Args args = {};
    args.flags = FT_OPEN_STREAM;
    args.stream = stream;

    return FT_Open_Face(library, &args, face_index, aface);
#else
    // On POSIX, FT_New_Face mmaps font files. If not Windows, prefer FT_New_Face over our stdio.h based FT_Open_Face.
    return FT_New_Face(library, filename_utf8, face_index, aface);
#endif
}

//------------------------------------------------------------------------
// CairoFont
//------------------------------------------------------------------------

CairoFont::CairoFont(Ref refA, cairo_font_face_t *cairo_font_faceA, std::vector<int> &&codeToGIDA, bool substituteA,
                     bool printingA)
    : ref(refA)
    , cairo_font_face(cairo_font_faceA)
    , substitute(substituteA)
    , printing(printingA)
{
    codeToGID = std::move(codeToGIDA);
}

CairoFont::~CairoFont()
{
    cairo_font_face_destroy(cairo_font_face);
}

bool CairoFont::matches(Ref &other, bool printingA)
{
    return (other == ref);
}

cairo_font_face_t *CairoFont::getFontFace()
{
    return cairo_font_face;
}

unsigned long CairoFont::getGlyph(CharCode code, const Unicode *u, int uLen)
{
    FT_UInt gid;

    if (code < codeToGID.size()) {
        gid = (FT_UInt)codeToGID[code];
    } else {
        gid = (FT_UInt)code;
    }
    return gid;
}

#if POPPLER_CHECK_VERSION(22, 4, 0)
double CairoFont::getSubstitutionCorrection(const std::shared_ptr<GfxFont> &gfxFont)
#else
double CairoFont::getSubstitutionCorrection(GfxFont *gfxFont)
#endif
{
    double w1, w2, w3;
    CharCode code;
    const char *name;

#if POPPLER_CHECK_VERSION(22, 4, 0)
    auto gfx8bit = std::static_pointer_cast<Gfx8BitFont>(gfxFont);
#else
    auto gfx8bit = dynamic_cast<Gfx8BitFont *>(gfxFont);
#endif

    // for substituted fonts: adjust the font matrix -- compare the
    // width of 'm' in the original font and the substituted font
    if (isSubstitute() && !gfxFont->isCIDFont()) {
        for (code = 0; code < 256; ++code) {
            if ((name = gfx8bit->getCharName(code)) && name[0] == 'm' && name[1] == '\0') {
                break;
            }
        }
        if (code < 256) {
            w1 = gfx8bit->getWidth(code);
            {
                cairo_matrix_t m;
                cairo_matrix_init_identity(&m);
                cairo_font_options_t *options = cairo_font_options_create();
                cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
                cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
                cairo_scaled_font_t *scaled_font = cairo_scaled_font_create(cairo_font_face, &m, &m, options);

                cairo_text_extents_t extents;
                cairo_scaled_font_text_extents(scaled_font, "m", &extents);

                cairo_scaled_font_destroy(scaled_font);
                cairo_font_options_destroy(options);
                w2 = extents.x_advance;
            }
            w3 = gfx8bit->getWidth(0);
            if (!gfxFont->isSymbolic() && w2 > 0 && w1 > w3) {
                // if real font is substantially narrower than substituted
                // font, reduce the font size accordingly
                if (w1 > 0.01 && w1 < 0.9 * w2) {
                    w1 /= w2;
                    return w1;
                }
            }
        }
    }
    return 1.0;
}

//------------------------------------------------------------------------
// CairoFreeTypeFont
//------------------------------------------------------------------------

static cairo_user_data_key_t ft_cairo_key;

// Font resources to be freed when cairo_font_face_t is destroyed
struct FreeTypeFontResource
{
    FT_Face face;
    std::vector<unsigned char> font_data;
};

// cairo callback for when cairo_font_face_t is destroyed
static void _ft_done_face(void *closure)
{
    FreeTypeFontResource *resource = (FreeTypeFontResource *)closure;

    FT_Done_Face(resource->face);
    delete resource;
}

CairoFreeTypeFont::CairoFreeTypeFont(Ref refA, cairo_font_face_t *cairo_font_faceA, std::vector<int> &&codeToGIDA,
                                     bool substituteA)
    : CairoFont(refA, cairo_font_faceA, std::move(codeToGIDA), substituteA, true)
{}

CairoFreeTypeFont::~CairoFreeTypeFont() {}

// Create a cairo_font_face_t for the given font filename OR font data.
static std::optional<FreeTypeFontFace> createFreeTypeFontFace(FT_Library lib, const std::string &filename,
                                                              std::vector<unsigned char> &&font_data)
{
    FreeTypeFontResource *resource = new FreeTypeFontResource;
    FreeTypeFontFace font_face;

    if (font_data.empty()) {
        FT_Error err = ft_new_face_from_file(lib, filename.c_str(), 0, &resource->face);
        if (err) {
            delete resource;
            return {};
        }
    } else {
        resource->font_data = std::move(font_data);
        FT_Error err = FT_New_Memory_Face(lib, (FT_Byte *)resource->font_data.data(), resource->font_data.size(), 0,
                                          &resource->face);
        if (err) {
            delete resource;
            return {};
        }
    }

    font_face.cairo_font_face =
        cairo_ft_font_face_create_for_ft_face(resource->face, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    if (cairo_font_face_set_user_data(font_face.cairo_font_face, &ft_cairo_key, resource, _ft_done_face)) {
        cairo_font_face_destroy(font_face.cairo_font_face);
        _ft_done_face(resource);
        return {};
    }

    font_face.face = resource->face;
    return font_face;
}

// Create a cairo_font_face_t for the given font filename OR font data. First checks if external font
// is in the cache.
std::optional<FreeTypeFontFace> CairoFreeTypeFont::getFreeTypeFontFace(CairoFontEngine *fontEngine, FT_Library lib,
                                                                       const std::string &filename,
                                                                       std::vector<unsigned char> &&font_data)
{
    if (font_data.empty()) {
        return fontEngine->getExternalFontFace(lib, filename);
    }

    return createFreeTypeFontFace(lib, filename, std::move(font_data));
}

#if POPPLER_CHECK_VERSION(22, 4, 0)
CairoFreeTypeFont *CairoFreeTypeFont::create(const std::shared_ptr<GfxFont> &gfxFont, XRef *xref, FT_Library lib,
                                             CairoFontEngine *fontEngine, bool useCIDs)
#else
CairoFreeTypeFont *CairoFreeTypeFont::create(GfxFont *gfxFont, XRef *xref, FT_Library lib, CairoFontEngine *fontEngine,
                                             bool useCIDs)
#endif
{
    std::string fileName;
    std::vector<unsigned char> font_data;
    int i, n;
#if POPPLER_CHECK_VERSION(22, 2, 0)
    std::optional<GfxFontLoc> fontLoc;
#else
    GfxFontLoc *fontLoc;
#endif
#if POPPLER_CHECK_VERSION(26, 5, 0)
    const char * const *enc;
#else
    char **enc;
#endif
    const char *name;
#if POPPLER_CHECK_VERSION(25, 7, 0)
    std::unique_ptr<FoFiType1C> ff1c;
#else
    FoFiType1C *ff1c;
#endif
    std::optional<FreeTypeFontFace> font_face;
    std::vector<int> codeToGID;
    bool substitute = false;

#if POPPLER_CHECK_VERSION(22, 4, 0)
    auto gfxcid = std::static_pointer_cast<GfxCIDFont>(gfxFont);
    auto gfx8bit = std::static_pointer_cast<Gfx8BitFont>(gfxFont);
    typedef unsigned char *fontchar;
#else
    auto gfxcid = dynamic_cast<GfxCIDFont *>(gfxFont);
    auto gfx8bit = dynamic_cast<Gfx8BitFont *>(gfxFont);
    typedef char *fontchar;
#endif

    Ref ref = *gfxFont->getID();
    Ref embFontID = Ref::INVALID();
    gfxFont->getEmbeddedFontID(&embFontID);
    GfxFontType fontType = gfxFont->getType();

    if (!(fontLoc = gfxFont->locateFont(xref, nullptr))) {
        error(errSyntaxError, -1, "Couldn't find a font for '{0:s}'",
              gfxFont->getName() ? gfxFont->getName()->c_str() : "(unnamed)");
        goto err2;
    }

    // embedded font
    if (fontLoc->locType == gfxFontLocEmbedded) {
#if POPPLER_CHECK_VERSION(22, 4, 0)
        auto fd = gfxFont->readEmbFontFile(xref);
        if (!fd || fd->empty()) {
            goto err2;
        }
        font_data = std::move(fd.value());
#else
        int nSize = 0;
        char *chars = gfxFont->readEmbFontFile(xref, &nSize);
        if (!nSize || !chars) {
            goto err2;
        }
        font_data = {chars, chars + nSize};
        gfree(chars);
#endif

        // external font
    } else { // gfxFontLocExternal

#if POPPLER_CHECK_VERSION(22, 1, 0)
        fileName = fontLoc->path;
#else
        fileName = fontLoc->path->toNonConstStr();
#endif
        fontType = fontLoc->fontType;
        substitute = true;
    }

    switch (fontType) {
        case fontType1:
        case fontType1C:
        case fontType1COT:
            font_face = getFreeTypeFontFace(fontEngine, lib, fileName, std::move(font_data));
            if (!font_face) {
                error(errSyntaxError, -1, "could not create type1 face");
                goto err2;
            }

#if POPPLER_CHECK_VERSION(26, 5, 0)
            enc = gfx8bit->getEncoding().data();
#else
            enc = gfx8bit->getEncoding();
#endif

            codeToGID.resize(256);
            for (i = 0; i < 256; ++i) {
                codeToGID[i] = 0;
                if ((name = enc[i])) {
                    codeToGID[i] = FT_Get_Name_Index(font_face->face, (char *)name);
                    if (codeToGID[i] == 0) {
                        Unicode u;
                        u = globalParams->mapNameToUnicodeText(name);
                        codeToGID[i] = FT_Get_Char_Index(font_face->face, u);
                    }
                    if (codeToGID[i] == 0) {
                        name = GfxFont::getAlternateName(name);
                        if (name) {
                            codeToGID[i] = FT_Get_Name_Index(font_face->face, (char *)name);
                        }
                    }
                }
            }
            break;
        case fontCIDType2:
        case fontCIDType2OT:
#if POPPLER_CHECK_VERSION(25,2,0)
            if (!gfxcid->getCIDToGID().empty()) {
                const auto src = gfxcid->getCIDToGID();
                codeToGID = std::move(src);
            }
#else
            if (gfxcid->getCIDToGID()) {
                n = gfxcid->getCIDToGIDLen();
                if (n) {
                    const auto src = gfxcid->getCIDToGID();
                    codeToGID.reserve(n);
                    codeToGID.insert(codeToGID.begin(), src, src + n);
                }
            }
#endif
            else {
#if POPPLER_CHECK_VERSION(22, 1, 0)
                std::unique_ptr<FoFiTrueType> ff;
#else
                FoFiTrueType *ff;
#endif
                if (!font_data.empty()) {
                    ff = _POPPLER_FOFI_TRUETYPE_MAKE(font_data, 0);
                } else {
                    ff = FoFiTrueType::load(fileName.c_str(), 0);
                }
                if (!ff) {
                    goto err2;
                }
#if POPPLER_CHECK_VERSION(22, 1, 0)
                auto src = gfxcid->_POPPLER_GET_CODE_TO_GID_MAP(ff.get(), &n);
#else
                auto src = gfxcid->_POPPLER_GET_CODE_TO_GID_MAP(ff, &n);
#endif

#if POPPLER_CHECK_VERSION(25,2,0)
                codeToGID = std::move(src);
#else
                codeToGID.reserve(n);
                codeToGID.insert(codeToGID.begin(), src, src + n);
                gfree(src);
#endif
            }
            /* Fall through */
        case fontTrueType:
        case fontTrueTypeOT: {
#if POPPLER_CHECK_VERSION(22, 1, 0)
            std::unique_ptr<FoFiTrueType> ff;
#else
            FoFiTrueType *ff;
#endif
            if (!font_data.empty()) {
                ff = _POPPLER_FOFI_TRUETYPE_MAKE(font_data, 0);
            } else {
                ff = FoFiTrueType::load(fileName.c_str(), 0);
            }
            if (!ff) {
                error(errSyntaxError, -1, "failed to load truetype font\n");
                goto err2;
            }
            /* This might be set already for the CIDType2 case */
            if (fontType == fontTrueType || fontType == fontTrueTypeOT) {
#if POPPLER_CHECK_VERSION(22, 1, 0)
                auto src = gfx8bit->getCodeToGIDMap(ff.get());
#else
                auto src = gfx8bit->getCodeToGIDMap(ff);
#endif
#if POPPLER_CHECK_VERSION(25,2,0)
                codeToGID = std::move(src);
#else
                codeToGID.reserve(256);
                codeToGID.insert(codeToGID.begin(), src, src + 256);
                gfree(src);
#endif
            }
            font_face = getFreeTypeFontFace(fontEngine, lib, fileName, std::move(font_data));
            if (!font_face) {
                error(errSyntaxError, -1, "could not create truetype face\n");
                goto err2;
            }
            break;
        }
        case fontCIDType0:
        case fontCIDType0C:
            if (!useCIDs) {
                if (!font_data.empty()) {
                    ff1c = _POPPLER_FOFI_TYPE1C_MAKE(font_data);
                } else {
                    ff1c = FoFiType1C::load(fileName.c_str());
                }
                if (ff1c) {
                    auto src = ff1c->_POPPLER_GET_CID_TO_GID_MAP(&n);
#if POPPLER_CHECK_VERSION(25,2,0)
                    codeToGID = std::move(src);
#else
                    codeToGID.reserve(n);
                    codeToGID.insert(codeToGID.begin(), src, src + n);
                    gfree(src);
#endif
#if !(POPPLER_CHECK_VERSION(25, 7, 0))
                    delete ff1c;
#endif
                }
            }

            font_face = getFreeTypeFontFace(fontEngine, lib, fileName, std::move(font_data));
            if (!font_face) {
                error(errSyntaxError, -1, "could not create cid face\n");
                goto err2;
            }
            break;

        case fontCIDType0COT:
#if POPPLER_CHECK_VERSION(25,2,0)
            if (!gfxcid->getCIDToGID().empty()) {
                const auto src = gfxcid->getCIDToGID();
                codeToGID = std::move(src);
            }
#else
            if (gfxcid->getCIDToGID()) {
                n = gfxcid->getCIDToGIDLen();
                if (n) {
                    const auto src = gfxcid->getCIDToGID();
                    codeToGID.reserve(n);
                    codeToGID.insert(codeToGID.begin(), src, src + n);
                }
            }
#endif

            if (codeToGID.empty()) {
                if (!useCIDs) {
#if POPPLER_CHECK_VERSION(22, 1, 0)
                    std::unique_ptr<FoFiTrueType> ff;
#else
                    FoFiTrueType *ff;
#endif
                    if (!font_data.empty()) {
                        ff = _POPPLER_FOFI_TRUETYPE_MAKE(font_data, 0);
                    } else {
                        ff = FoFiTrueType::load(fileName.c_str(), 0);
                    }
                    if (ff) {
                        if (ff->isOpenTypeCFF()) {
                            auto src = ff->_POPPLER_GET_CID_TO_GID_MAP(&n);
#if POPPLER_CHECK_VERSION(25,2,0)
                            codeToGID = std::move(src);
#else
                            codeToGID.reserve(n);
                            codeToGID.insert(codeToGID.begin(), src, src + n);
                            gfree(src);
#endif
                        }
                    }
                }
            }
            font_face = getFreeTypeFontFace(fontEngine, lib, fileName, std::move(font_data));
            if (!font_face) {
                error(errSyntaxError, -1, "could not create cid (OT) face\n");
                goto err2;
            }
            break;

        default:
            fprintf(stderr, "font type %d not handled\n", (int)fontType);
            goto err2;
            break;
    }

    return new CairoFreeTypeFont(ref, font_face->cairo_font_face, std::move(codeToGID), substitute);

err2:
    fprintf(stderr, "some font thing failed\n");
    return nullptr;
}

//------------------------------------------------------------------------
// CairoType3Font
//------------------------------------------------------------------------

static const cairo_user_data_key_t type3_font_key = {0};

typedef struct _type3_font_info
{
#if POPPLER_CHECK_VERSION(22, 4, 0)
    _type3_font_info(const std::shared_ptr<GfxFont> &fontA, PDFDoc *docA, CairoFontEngine *fontEngineA, bool printingA,
                     XRef *xrefA)
        : font(fontA)
        , doc(docA)
        , fontEngine(fontEngineA)
        , printing(printingA)
        , xref(xrefA)
    {}

    std::shared_ptr<GfxFont> font;
#else
    _type3_font_info(GfxFont *fontA, PDFDoc *docA, CairoFontEngine *fontEngineA, bool printingA, XRef *xrefA)
        : font(fontA)
        , doc(docA)
        , fontEngine(fontEngineA)
        , printing(printingA)
        , xref(xrefA)
    {}

    GfxFont *font;
#endif

    PDFDoc *doc;
    CairoFontEngine *fontEngine;
    bool printing;
    XRef *xref;
} type3_font_info_t;

static void _free_type3_font_info(void *closure)
{
    type3_font_info_t *info = (type3_font_info_t *)closure;
    delete info;
}

static cairo_status_t _init_type3_glyph(cairo_scaled_font_t *scaled_font, cairo_t *cr, cairo_font_extents_t *extents)
{
    type3_font_info_t *info;

    info = (type3_font_info_t *)cairo_font_face_get_user_data(cairo_scaled_font_get_font_face(scaled_font),
                                                              &type3_font_key);
    const auto& mat = info->font->getFontBBox();
    extents->ascent = mat[3];   /* y2 */
    extents->descent = -mat[3]; /* -y1 */
    extents->height = extents->ascent + extents->descent;
    extents->max_x_advance = mat[2] - mat[1]; /* x2 - x1 */
    extents->max_y_advance = 0;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t _render_type3_glyph(cairo_scaled_font_t *scaled_font, unsigned long glyph, cairo_t *cr,
                                          cairo_text_extents_t *metrics, bool color)
{
    // We have stripped out the type3 glyph support here, because it calls back
    // into CairoOutputDev which is private and would pull in the entire poppler codebase.
    return CAIRO_STATUS_USER_FONT_ERROR;
}

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 6)
static cairo_status_t _render_type3_color_glyph(cairo_scaled_font_t *scaled_font, unsigned long glyph, cairo_t *cr,
                                                cairo_text_extents_t *metrics)
{
    return _render_type3_glyph(scaled_font, glyph, cr, metrics, true);
}
#endif

static cairo_status_t _render_type3_noncolor_glyph(cairo_scaled_font_t *scaled_font, unsigned long glyph, cairo_t *cr,
                                                   cairo_text_extents_t *metrics)
{
    return _render_type3_glyph(scaled_font, glyph, cr, metrics, false);
}

#if POPPLER_CHECK_VERSION(22, 4, 0)
CairoType3Font *CairoType3Font::create(const std::shared_ptr<GfxFont> &gfxFont, PDFDoc *doc,
                                       CairoFontEngine *fontEngine, bool printing, XRef *xref)
{
    auto gfx8bit = std::static_pointer_cast<Gfx8BitFont>(gfxFont);
#else
CairoType3Font *CairoType3Font::create(GfxFont *gfxFont, PDFDoc *doc, CairoFontEngine *fontEngine, bool printing,
                                       XRef *xref)
{
    auto gfx8bit = dynamic_cast<Gfx8BitFont *>(gfxFont);
#endif

    std::vector<int> codeToGID;
    const char *name;

    Dict *charProcs = gfx8bit->getCharProcs();
    Ref ref = *gfxFont->getID();
    cairo_font_face_t *font_face = cairo_user_font_face_create();
    cairo_user_font_face_set_init_func(font_face, _init_type3_glyph);
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 6)
    // When both callbacks are set, Cairo will call the color glyph
    // callback first.  If that returns NOT_IMPLEMENTED, Cairo will
    // then call the non-color glyph callback.
    cairo_user_font_face_set_render_color_glyph_func(font_face, _render_type3_color_glyph);
#endif
    cairo_user_font_face_set_render_glyph_func(font_face, _render_type3_noncolor_glyph);
    type3_font_info_t *info = new type3_font_info_t(gfxFont, doc, fontEngine, printing, xref);

    cairo_font_face_set_user_data(font_face, &type3_font_key, (void *)info, _free_type3_font_info);

#if POPPLER_CHECK_VERSION(26, 5, 0)
    const char * const *enc = gfx8bit->getEncoding().data();
#else
    char **enc = gfx8bit->getEncoding();
#endif
    codeToGID.resize(256);
    for (int i = 0; i < 256; ++i) {
        codeToGID[i] = 0;
        if (charProcs && (name = enc[i])) {
            for (int j = 0; j < charProcs->getLength(); j++) {
                if (std::string(charProcs->getKey(j)).compare(name) == 0) {
                    codeToGID[i] = j;
                }
            }
        }
    }

    return new CairoType3Font(ref, font_face, std::move(codeToGID), printing, xref);
}

CairoType3Font::CairoType3Font(Ref refA, cairo_font_face_t *cairo_font_faceA, std::vector<int> &&codeToGIDA,
                               bool printingA, XRef *xref)
    : CairoFont(refA, cairo_font_faceA, std::move(codeToGIDA), false, printingA)
{}

CairoType3Font::~CairoType3Font() {}

bool CairoType3Font::matches(Ref &other, bool printingA)
{
    return (other == ref && printing == printingA);
}

//------------------------------------------------------------------------
// CairoFontEngine
//------------------------------------------------------------------------

std::unordered_map<std::string, FreeTypeFontFace> CairoFontEngine::fontFileCache;
std::recursive_mutex CairoFontEngine::fontFileCacheMutex;

CairoFontEngine::CairoFontEngine(FT_Library libA)
{
    lib = libA;
    fontCache.reserve(cairoFontCacheSize);

    FT_Int major, minor, patch;
    // as of FT 2.1.8, CID fonts are indexed by CID instead of GID
    FT_Library_Version(lib, &major, &minor, &patch);
    useCIDs = major > 2 || (major == 2 && (minor > 1 || (minor == 1 && patch > 7)));
}

CairoFontEngine::~CairoFontEngine() {}

#if POPPLER_CHECK_VERSION(22, 4, 0)
std::shared_ptr<CairoFont> CairoFontEngine::getFont(const std::shared_ptr<GfxFont> &gfxFont, PDFDoc *doc, bool printing,
                                                    XRef *xref)
#else
std::shared_ptr<CairoFont> CairoFontEngine::getFont(GfxFont *gfxFont, PDFDoc *doc, bool printing, XRef *xref)
#endif
{
    std::scoped_lock lock(mutex);
    Ref ref = *gfxFont->getID();
    std::shared_ptr<CairoFont> font;

    // Check if font is in the MRU cache, and move it to the end if it is.
    for (auto it = fontCache.rbegin(); it != fontCache.rend(); ++it) {
        if ((*it)->matches(ref, printing)) {
            font = *it;
            // move it to the end
            if (it != fontCache.rbegin()) {
                // https://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                fontCache.erase(std::next(it).base());
                fontCache.push_back(font);
            }
            return font;
        }
    }

    GfxFontType fontType = gfxFont->getType();
    if (fontType == fontType3) {
        font = std::shared_ptr<CairoFont>(CairoType3Font::create(gfxFont, doc, this, printing, xref));
    } else {
        font = std::shared_ptr<CairoFont>(CairoFreeTypeFont::create(gfxFont, xref, lib, this, useCIDs));
    }

    if (font) {
        if (fontCache.size() == cairoFontCacheSize) {
            fontCache.erase(fontCache.begin());
        }
        fontCache.push_back(font);
    }
    return font;
}

std::optional<FreeTypeFontFace> CairoFontEngine::getExternalFontFace(FT_Library ftlib, const std::string &filename)
{
    std::scoped_lock lock(fontFileCacheMutex);

    auto it = fontFileCache.find(filename);
    if (it != fontFileCache.end()) {
        FreeTypeFontFace font = it->second;
        cairo_font_face_reference(font.cairo_font_face);
        return font;
    }

    std::optional<FreeTypeFontFace> font_face = createFreeTypeFontFace(ftlib, filename, {});
    if (font_face) {
        cairo_font_face_reference(font_face->cairo_font_face);
        fontFileCache[filename] = *font_face;
    }

    it = fontFileCache.begin();
    while (it != fontFileCache.end()) {
        if (cairo_font_face_get_reference_count(it->second.cairo_font_face) == 1) {
            cairo_font_face_destroy(it->second.cairo_font_face);
            it = fontFileCache.erase(it);
        } else {
            ++it;
        }
    }

    return font_face;
}
