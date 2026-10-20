// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_BOUNDING_BOX_H
#define INKSCAPE_LPE_BOUNDING_BOX_H

/*
 * Inkscape::LPEFillBetweenStrokes
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/originalsatellite.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEBoundingBox : public Effect {
public:
    LPEBoundingBox(LivePathEffectObject *lpeobject);
    ~LPEBoundingBox() override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    bool doOnOpen(SPLPEItem const *lpeitem) override;
    void doEffect(Geom::PathVector &curve) override;
    void doOnApply(SPLPEItem const *lpeitem) override;

private:
    OriginalSatelliteParam  linked_path;
    BoolParam visual_bounds;

private:
    LPEBoundingBox(const LPEBoundingBox&) = delete;
    LPEBoundingBox& operator=(const LPEBoundingBox&) = delete;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
