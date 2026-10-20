// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINAL_ITEM_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINAL_ITEM_H

/*
 * Inkscape::LiveItemEffectParameters
 *
 * Copyright (C) Johan Engelen 2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/parameter/satellite.h"

namespace Inkscape {

namespace LivePathEffect {

class OriginalSatelliteParam : public SatelliteParam
{
public:
    OriginalSatelliteParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                           Inkscape::UI::Widget::Registry *wr, Effect *effect);
    Gtk::Widget * param_newWidget() override;
    ParamType paramType() const override { return ParamType::ORIGINAL_SATELLITE; };
protected:
    void on_select_original_button_click();

private:
    OriginalSatelliteParam(const OriginalSatelliteParam &) = delete;
    OriginalSatelliteParam &operator=(const OriginalSatelliteParam &) = delete;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
