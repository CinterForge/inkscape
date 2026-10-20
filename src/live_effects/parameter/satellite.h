// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_SATELLITE_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_SATELLITE_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <sigc++/sigc++.h>

#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/satellite-reference.h"

namespace Inkscape {

namespace LivePathEffect {
class LPECloneOriginal;

class SatelliteParam : public Parameter
{
public:
    SatelliteParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                   Inkscape::UI::Widget::Registry *wr, Effect *effect);
    ~SatelliteParam() override;
    bool param_readSVGValue(const gchar *strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_update_default(const gchar *default_value) override{};
    bool linksToItem() const;
    SPObject *getObject() const;
    // UI
    Gtk::Widget *param_newWidget() override;
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    void on_link_button_click();
    friend class LPEBool;
    friend class LPECloneOriginal;
    friend class Effect;
    Geom::Affine last_transform;
    bool isConnected() {return !(!linked_changed_connection);}
    void start_listening(SPObject *to);
    void unlink();
    bool lookup = false;
    ParamType paramType() const override { return ParamType::SATELLITE; };
protected:
    void link(Glib::ustring itemid);
    void quit_listening();
    void linked_released(SPObject *released_item);
    void linked_deleted(SPObject *deleted_item);
    void linked_transformed(Geom::Affine const *rel_transf, SPItem *moved_item);
    void linked_modified(SPObject *linked_obj, guint flags);
    void linked_changed(SPObject *old_obj, SPObject *new_obj);
    
    std::shared_ptr<SatelliteReference> lperef;

private:
    std::vector<SPObject *> param_get_satellites() override;
    sigc::connection linked_released_connection;
    sigc::connection linked_modified_connection;
    sigc::connection linked_transformed_connection;
    sigc::connection linked_changed_connection;
    SatelliteParam(const SatelliteParam &) = delete;
    SatelliteParam &operator=(const SatelliteParam &) = delete;
};

} // namespace LivePathEffect

} // namespace Inkscape

#endif
