// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Utility structures and functions for pdf parsing.
 *//*
 * 
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "pdf-utils.h"

#include <utility>
#include <glib.h>
#include <2geom/path-sink.h>

#include "path/path-boolop.h"
#include "poppler-utils.h"

// Map PDF clip types to fill rules
const std::map<GfxClipType, FillRule> ClipFillMap = {
    {clipNone, FillRule::fill_justDont},
    {clipNormal, FillRule::fill_nonZero},
    {clipEO, FillRule::fill_oddEven}
};

//------------------------------------------------------------------------
// ClipHistoryEntry
//------------------------------------------------------------------------

ClipHistoryEntry::ClipHistoryEntry(Geom::PathVector clipPathA, GfxClipType clipTypeA)
    : saved(nullptr)
    , clipPath(std::move(clipPathA))
    , fillRule(ClipFillMap.at(clipTypeA))
{}

ClipHistoryEntry::~ClipHistoryEntry()
{
    if (saved) {
        delete saved;
        saved = nullptr;
    }
}

/**
 * Set the clipping path of the current entry (does not add to stack). This is mostly
 * exposed in public for ease of testing, but also handy for bbox.
 */
void ClipHistoryEntry::setClip(Geom::PathVector const &newPath, FillRule newFill) {
    if (copied) {
        // overwrite the new path info
        clipPath = newPath;
    } else {
        // Destructively compose the new clipping path by intersecting with the current
        clipPath = maybeIntersect(clipPath, newPath, fillRule, newFill);
    } 
    
    // either way, set the new fill rule. This assumes that the new fill rule is the one that should apply
    // to the output of the intersection operation, but it may not matter due to intersection normalization
    fillRule = newFill;
    copied = false;
}

/**
 * Set the clip path based on the poppler GfxState, baking in the affine transform.
 */
void ClipHistoryEntry::setClip(GfxState *state, GfxClipType clipType)
{
    auto newPath = getPathV(state->getPath()) * stateToAffine(state);
    setClip(newPath, ClipFillMap.at(clipType));
}

/**
 * Create a new clip-history, appending it to the stack.
 */
ClipHistoryEntry *ClipHistoryEntry::save()
{
    ClipHistoryEntry *newEntry = new ClipHistoryEntry(this);
    newEntry->saved = this;
    return newEntry;
}

ClipHistoryEntry *ClipHistoryEntry::restore()
{
    ClipHistoryEntry *oldEntry;

    if (saved) {
        oldEntry = saved;
        saved = nullptr;
        delete this; // TODO really should avoid deleting from inside.
    } else {
        oldEntry = this;
    }

    return oldEntry;
}

ClipHistoryEntry::ClipHistoryEntry(ClipHistoryEntry *other)
{
    this->clipPath = other->clipPath;
    this->fillRule = other->fillRule;
    this->copied = true;
    saved = nullptr;
}

/**
 * Computes the intersection of all clipping paths in the stack.
 */

Geom::PathVector ClipHistoryEntry::getFlattenedClipPath() 
{
    if (saved) {
        return maybeIntersect(clipPath, saved->getFlattenedClipPath(), fillRule, saved->fillRule);
    } else {
        return clipPath;
    }
}

/*************** Helper functions *****************/

Geom::Rect getRect(_POPPLER_CONST PDFRectangle *box)
{
    return Geom::Rect(box->x1, box->y1, box->x2, box->y2);
}

Geom::Rect getRect(_POPPLER_CONST PDFRectangle &box)
{
    return Geom::Rect(box.x1, box.y1, box.x2, box.y2);
}

Geom::PathVector getPathV(_POPPLER_CONST GfxPath *path) 
{
    if (!path) {
        // empty path
        return Geom::PathVector();
    }

    // copied from svgInterpretPath, but with a PathBuilder instead of string
    Geom::PathBuilder res;
    for (int i = 0 ; i < path->getNumSubpaths() ; ++i ) {
        _POPPLER_CONST_83 GfxSubpath *subpath = path->getSubpath(i);
        if (subpath->getNumPoints() > 0) {
            res.moveTo(Geom::Point(subpath->getX(0), subpath->getY(0)));
            int j = 1;
            while (j < subpath->getNumPoints()) {
                if (subpath->getCurve(j)) {
                    res.curveTo(Geom::Point(subpath->getX(j), subpath->getY(j)),
                                Geom::Point(subpath->getX(j+1), subpath->getY(j+1)),
                                Geom::Point(subpath->getX(j+2), subpath->getY(j+2)));

                    j += 3;
                } else {
                    res.lineTo(Geom::Point(subpath->getX(j), subpath->getY(j)));
                    ++j;
                }
            }
            if (subpath->isClosed()) {
                res.closePath();
            }
        }
    }

    res.flush();
    return res.peek();
}

/**
 * Computes the intersection between paths v1 and v2. If one of the paths is empty, return the other.
 */
Geom::PathVector maybeIntersect(Geom::PathVector const &v1, Geom::PathVector const &v2, FillRule fill1, FillRule fill2) {
    if (v1.empty()) {
        // okay if both are empty (just the same)
        return v2;
    }
    if (v2.empty()) {
        return v1;
    }

    return sp_pathvector_boolop(v1, v2, BooleanOp::bool_op_inters, fill1, fill2);
}
