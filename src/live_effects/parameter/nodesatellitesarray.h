// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_NODESATELLITES_ARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_NODESATELLITES_ARRAY_H

/*
 * Inkscape::LivePathEffectParameters
 * Copyright (C) Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Special thanks to Johan Engelen for the base of the effect -powerstroke-
 * Also to ScislaC for pointing me to the idea
 * Also su_v for his constructive feedback and time
 * To Nathan Hurst for his review and help on refactor
 * and finally to Liam P. White for his big help on coding,
 * that saved me a lot of hours
 *
 *
 * This parameter acts as a bridge from pathVectorNodeSatellites class to serialize it as a LPE
 * parameter
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>

#include "helper/geom-pathvector_nodesatellites.h"
#include "live_effects/effect-enum.h"
#include "live_effects/parameter/array.h"
#include "ui/knot/knot-holder-entity.h"
#include "ui/knot/knot-holder.h"

namespace Inkscape {

namespace LivePathEffect {

class FilletChamferKnotHolderEntity;

class NodeSatelliteArrayParam : public ArrayParam<std::vector<NodeSatellite>>
{
public:
    NodeSatelliteArrayParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                            Inkscape::UI::Widget::Registry *wr, Effect *effect);

    Gtk::Widget *param_newWidget() override
    {
        return nullptr;
    }
    void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item) override;
    virtual void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item, bool mirror);
    void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec) override;
    virtual void updateCanvasIndicators();
    virtual void updateCanvasIndicators(bool mirror);
    bool providesKnotHolderEntities() const override
    {
        return true;
    }
    void param_transform_multiply(Geom::Affine const &postmul, bool /*set*/) override;
    void setUseDistance(bool use_knot_distance);
    void setCurrentZoom(double current_zoom);
    void setGlobalKnotHide(bool global_knot_hide);
    void setEffectType(EffectType et);
    void reloadKnots();
    void updateAmmount(double amount);
    void setPathVectorNodeSatellites(PathVectorNodeSatellites *pathVectorNodeSatellites, bool write = true);

    void set_oncanvas_looks(Inkscape::CanvasItemCtrlShape shape, uint32_t color);


    friend class FilletChamferKnotHolderEntity;
    friend class LPEFilletChamfer;
    ParamType paramType() const override { return ParamType::NODE_SATELLITE_ARRAY; };
private:
    NodeSatelliteArrayParam(const NodeSatelliteArrayParam &) = delete;
    NodeSatelliteArrayParam &operator=(const NodeSatelliteArrayParam &) = delete;

    Inkscape::CanvasItemCtrlShape _knot_shape = Inkscape::CANVAS_ITEM_CTRL_SHAPE_DIAMOND;
    guint32 _knot_color = 0xaaff8800;
    Geom::PathVector _hp;
    bool _use_distance = false;
    bool _global_knot_hide = false;
    double _current_zoom = 0;
    EffectType _effectType = FILLET_CHAMFER;
    PathVectorNodeSatellites *_last_pathvector_nodesatellites = nullptr;
};

class FilletChamferKnotHolderEntity : public KnotHolderEntity {
public:
    FilletChamferKnotHolderEntity(NodeSatelliteArrayParam *p, size_t index);
    void knot_set(Geom::Point const &p, Geom::Point const &origin,
                          guint state) override;
    Geom::Point knot_get() const override;
    Geom::Point knot_get_gap();    
    void knot_click(guint state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_set_offset(NodeSatellite);
    /** Checks whether the index falls within the size of the parameter's vector
     */
    bool valid_index(size_t index, size_t subindex) const
    {
        return (_pparam->_vector.size() > index && _pparam->_vector[index].size() > subindex);
    };

private:
    NodeSatelliteArrayParam *_pparam;
    size_t _index;
    bool _updating = false;
    mutable bool _set_circle = false;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
