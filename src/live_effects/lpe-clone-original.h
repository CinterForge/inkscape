// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_CLONE_ORIGINAL_H
#define INKSCAPE_LPE_CLONE_ORIGINAL_H

/*
 * Inkscape::LPECloneOriginal
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/effect.h"
#include "live_effects/lpegroupbbox.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/originalsatellite.h"
#include "live_effects/parameter/text.h"

namespace Inkscape {
namespace LivePathEffect {

enum Clonelpemethod { CLM_NONE, CLM_D, CLM_ORIGINALD, CLM_BSPLINESPIRO, CLM_END };

class LPECloneOriginal : public Effect, GroupBBoxEffect {
public:
    LPECloneOriginal(LivePathEffectObject *lpeobject);
    ~LPECloneOriginal() override;
    void doEffect(Geom::PathVector &curve) override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    bool doOnOpen(SPLPEItem const *lpeitem) override;
    void doOnRemove(SPLPEItem const * /*lpeitem*/) override;
    bool getHolderRemove() override;
    Gtk::Widget *newWidget() override;
    OriginalSatelliteParam linkeditem;

private:
    EnumParam<Clonelpemethod> method;
    TextParam attributes;
    TextParam css_properties;
    BoolParam allow_transforms;
    Glib::ustring old_attributes;
    Glib::ustring old_css_properties;
    Glib::ustring linked;
    void syncOriginal();
    void cloneAttributes(SPObject *origin, SPObject *dest, const gchar *attributes, const gchar *css_properties,
                         bool init);
    bool sync;
    bool holderRemove = false; // move to effect if use more
    LPECloneOriginal(const LPECloneOriginal&) = delete;
    LPECloneOriginal& operator=(const LPECloneOriginal&) = delete;
};

}; //namespace LivePathEffect
}; //namespace Inkscape

#endif
