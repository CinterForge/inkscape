// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_POWERSTROKE_POINT_ARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_POWERSTROKE_POINT_ARRAY_H

/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <2geom/point.h>

#include "live_effects/parameter/array.h"

#include "ui/knot/knot-holder-entity.h"

namespace Inkscape {

namespace LivePathEffect {

class PowerStrokePointArrayParam : public ArrayParam<Geom::Point> {
public:
    PowerStrokePointArrayParam( const Glib::ustring& label,
                const Glib::ustring& tip,
                const Glib::ustring& key,
                Inkscape::UI::Widget::Registry* wr,
                Effect* effect);
    ~PowerStrokePointArrayParam() override;

    PowerStrokePointArrayParam(const PowerStrokePointArrayParam&) = delete;
    PowerStrokePointArrayParam& operator=(const PowerStrokePointArrayParam&) = delete;

    Gtk::Widget * param_newWidget() override;

    void param_transform_multiply(Geom::Affine const& postmul, bool /*set*/) override;

    void set_oncanvas_looks(CanvasItemCtrlShape shape, uint32_t color);

    float median_width();
    Geom::Point knot_get(size_t index);
    Geom::Point knot_reposition(size_t index, Geom::PathVector pathv);

    bool providesKnotHolderEntities() const override { return true; }
    void addKnotHolderEntities(KnotHolder *knotholder, SPItem *item) override;
    void param_update_default(const gchar * default_value) override{};

    void set_pwd2(Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in, Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_normal_in);
    Geom::Piecewise<Geom::D2<Geom::SBasis> > const & get_pwd2() const { return last_pwd2; }
    Geom::Piecewise<Geom::D2<Geom::SBasis> > const & get_pwd2_normal() const { return last_pwd2_normal; }

    void recalculate_controlpoints(Geom::PathVector pv);
    std::vector<Geom::Point> reverse_controlpoints(bool write);
    void set_scale_width(double scale_width){_scale_width = scale_width;};
    double _scale_width;
    ParamType paramType() const override { return ParamType::POWERSTROKE_POINT_ARRAY; };
    friend class PowerStrokePointArrayParamKnotHolderEntity;
    bool unplaced = false;
    size_t current_path = Glib::ustring::npos;
private:
    Inkscape::CanvasItemCtrlShape knot_shape = Inkscape::CANVAS_ITEM_CTRL_SHAPE_DIAMOND;
    guint32 knot_color;

    Geom::Piecewise<Geom::D2<Geom::SBasis> > last_pwd2;
    Geom::Piecewise<Geom::D2<Geom::SBasis> > last_pwd2_normal;
};

class PowerStrokePointArrayParamKnotHolderEntity : public KnotHolderEntity {
public:
    PowerStrokePointArrayParamKnotHolderEntity(PowerStrokePointArrayParam *p, unsigned int index);
    ~PowerStrokePointArrayParamKnotHolderEntity() override = default;

    void knot_set(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    Geom::Point knot_get() const override;
    virtual void knot_set_offset(Geom::Point offset);
    void knot_click(guint state) override;

    /** Checks whether the index falls within the size of the parameter's vector */
    bool valid_index(unsigned int index) const {
        return (_pparam->_vector.size() > index);
    };

private:
    PowerStrokePointArrayParam *_pparam;
    unsigned int _index;
};

} //namespace LivePathEffect

} //namespace Inkscape

#endif
