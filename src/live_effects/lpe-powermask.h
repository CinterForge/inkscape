// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_POWERMASK_H
#define INKSCAPE_LPE_POWERMASK_H

/*
 * Inkscape::LPEPowerMask
 *
 * Authors:
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/colorpicker.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerMask : public Effect
{
public:
    explicit LPEPowerMask(LivePathEffectObject *lpeobject);
    ~LPEPowerMask() override;
    void doOnApply(SPLPEItem const *lpeitem) override;
    void doBeforeEffect(SPLPEItem const *lpeitem) override;
    void doEffect(Geom::PathVector &curve) override;
    void doOnRemove(SPLPEItem const * /*lpeitem*/) override;
    void doOnVisibilityToggled(SPLPEItem const *lpeitem) override;
    void doAfterEffect(SPLPEItem const *lpeitem, Geom::PathVector *curve) override;

private:
    Glib::ustring getId() const;
    bool update_mask_visibility(SPLPEItem const *lpeitem);
    void handle_inverse_filter(Glib::ustring const &filter_uri) const;
    Glib::ustring prepare_color_inversion_filter(SPLPEItem const *lpeitem);
    void update_mask_box();

    BoolParam invert;
    BoolParam hide_mask;
    BoolParam background;
    ColorPickerParam background_color;
    Geom::Path mask_box_path;
};

void sp_remove_powermask(Inkscape::Selection *sel);
void sp_inverse_powermask(Inkscape::Selection *sel);

} // namespace LivePathEffect
} // namespace Inkscape

#endif
