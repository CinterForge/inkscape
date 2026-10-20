// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * PDF Parsing utility functions and classes.
 *//*
 * 
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef PDF_UTILS_H
#define PDF_UTILS_H

#include <Gfx.h>
#include <GfxState.h>
#include <Page.h>
#include <2geom/affine.h>
#include <2geom/pathvector.h>
#include <2geom/rect.h>

#include "livarot/LivarotDefs.h"
#include "poppler-transition-api.h"

class ClipHistoryEntry
{
public:
    ClipHistoryEntry(Geom::PathVector clipPath = Geom::PathVector(), GfxClipType clipType = clipNormal);
    virtual ~ClipHistoryEntry();

    // Manipulate clip path stack
    ClipHistoryEntry *save();
    ClipHistoryEntry *restore();
    Geom::PathVector getFlattenedClipPath();

    bool hasSaves() { return saved != nullptr; }
    bool hasClipPath() { return !clipPath.empty(); }
    bool isCopied() { return copied; }
    void setClip(GfxState *state, GfxClipType clipType = clipNormal);
    void setClip(Geom::PathVector const &newClip, FillRule newFill);
    Geom::PathVector const &getClipPath() const { return clipPath; }
    FillRule getFillRule() { return fillRule; }
    void clear() { clipPath.clear(); }

private:
    ClipHistoryEntry *saved; // next clip path on stack

    Geom::PathVector clipPath;
    FillRule fillRule;
    bool copied = false;

    ClipHistoryEntry(ClipHistoryEntry *other);
};

Geom::Rect getRect(_POPPLER_CONST PDFRectangle *box);
Geom::Rect getRect(_POPPLER_CONST PDFRectangle &box);
Geom::PathVector getPathV(_POPPLER_CONST GfxPath *gPath);
Geom::PathVector maybeIntersect(Geom::PathVector const &v1, Geom::PathVector const &v2,
                                FillRule fill1 = FillRule::fill_nonZero, FillRule fill2 = FillRule::fill_nonZero);

#endif /* PDF_UTILS_H */
