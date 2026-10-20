// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_KNOT_HOLDER_ENTITY_H
#define SEEN_KNOT_HOLDER_ENTITY_H
/*
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2001 Mitsuru Oka
 * Copyright (C) 2004 Monash University
 * Copyright (C) 2008 Maximilian Albert
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdint>
#include <2geom/forward.h>
#include <glibmm/i18n.h>

#include "snapper.h"

#include "display/control/canvas-item-quad.h"
#include "display/control/canvas-item-curve.h"
#include "display/control/canvas-item-ptr.h"
#include "util-string/context-string.h"

class SPHatch;
class SPItem;
class SPKnot;
class SPDesktop;
class SPPattern;
class SPGaussianBlur;
class KnotHolder;

namespace Inkscape::LivePathEffect { class Effect; }

/**
 * KnotHolderEntity definition.
 */
class KnotHolderEntity
{
public:
    virtual ~KnotHolderEntity();

    void create(SPDesktop *desktop, SPItem *item, KnotHolder *parent,
                Inkscape::CanvasItemCtrlType type = Inkscape::CANVAS_ITEM_CTRL_TYPE_DEFAULT,
                Glib::ustring const &name = "unknown",
                char const *tip = "",
                uint32_t color = 0xffffff00);

    /* the get/set/click handlers are virtual functions; each handler class for a knot
       should be derived from KnotHolderEntity and override these functions */
    virtual void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) = 0;
    virtual void knot_grabbed(Geom::Point const &/*grab_position*/, unsigned /*state*/) {}
    virtual void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, unsigned state) = 0;
    virtual bool knot_missing() const { return false; }
    virtual Geom::Point knot_get() const = 0;
    virtual void knot_click(unsigned /*state*/) {}
    virtual bool set_item_clickpos(Geom::Point loc) { return false; }

    virtual void on_created() {}
    virtual void update_knot();

    // private:
    Geom::Point snap_knot_position(Geom::Point const &p, unsigned state);
    Geom::Point snap_knot_position_constrained(Geom::Point const &p, Inkscape::Snapper::SnapConstraint const &constraint, unsigned state);

    SPKnot *knot = nullptr;
    SPItem *item = nullptr;
    SPDesktop *desktop = nullptr;
    KnotHolder *parent_holder = nullptr;

private:
    sigc::scoped_connection _mousedown_connection;
    sigc::scoped_connection _moved_connection;
    sigc::scoped_connection _click_connection;
    sigc::scoped_connection _ungrabbed_connection;
};

// LPEs

template <typename T>
class LPEKnotHolderEntity : public KnotHolderEntity
{
public:
    LPEKnotHolderEntity(T *effect) : _effect(effect) {}

    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, unsigned state) override {
        if (_effect) {
            _effect->makeUndoDone(RC_("Undo", "Move handle"));
        }
    }

protected:
    T *const _effect;
};

// Pattern manipulation

class PatternKnotHolderEntity : public KnotHolderEntity
{
public:
    PatternKnotHolderEntity(bool fill) : _fill(fill) {}
    void on_created() override;
    bool knot_missing() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, unsigned state) override {};
    bool set_item_clickpos(Geom::Point loc) override;
    void update_knot() override;

protected:
    SPPattern *_pattern() const;
    Geom::Point _get_pos(double x, double y, bool transform = true) const;
    Geom::IntPoint offset_to_cell(Geom::Point loc) const;

    /// true if the entity tracks fill, false for stroke
    bool _fill;
    Geom::IntPoint _cell;
};

class PatternKnotHolderEntityXY : public PatternKnotHolderEntity
{
public:
    PatternKnotHolderEntityXY(bool fill) : PatternKnotHolderEntity(fill) {}
    void on_created() override;
    void update_knot() override;
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;

private:
    /// Extra visual element to show the pattern editing area
    CanvasItemPtr<Inkscape::CanvasItemQuad> _quad;
};

class PatternKnotHolderEntityAngle : public PatternKnotHolderEntity
{
public:
    PatternKnotHolderEntityAngle(bool fill) : PatternKnotHolderEntity(fill) {}
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;
};

class PatternKnotHolderEntityScale : public PatternKnotHolderEntity
{
public:
    PatternKnotHolderEntityScale(bool fill) : PatternKnotHolderEntity(fill) {}
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;
    void knot_grabbed(Geom::Point const &grab_pos, unsigned state) override;

private:
    /// Maximum number of pattern repetitons allowed in an item
    static constexpr double MAX_REPETITIONS = 1e6;
    Geom::Affine _cached_transform, _cached_inverse_linear;
    Geom::Point _cached_origin, _cached_diagonal;
    double _cached_min_scale;
};

// Hatch manipulation

class HatchKnotHolderEntity : public KnotHolderEntity
{
public:
    HatchKnotHolderEntity(bool fill) : _fill(fill) {}
    bool knot_missing() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, unsigned state) override {};

protected:
    SPHatch *_hatch() const;

    /// true if the entity tracks fill, false for stroke
    bool _fill;
};

class HatchKnotHolderEntityXY : public HatchKnotHolderEntity
{
public:
    using HatchKnotHolderEntity::HatchKnotHolderEntity;
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;
};

class HatchKnotHolderEntityAngle : public HatchKnotHolderEntity
{
public:
    using HatchKnotHolderEntity::HatchKnotHolderEntity;
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;
};

class HatchKnotHolderEntityScale : public HatchKnotHolderEntity
{
public:
    using HatchKnotHolderEntity::HatchKnotHolderEntity;
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;
};


// Filter manipulation

class FilterKnotHolderEntity : public KnotHolderEntity
{
public:
    FilterKnotHolderEntity(bool topleft) : _topleft(topleft) {}
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, unsigned state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;

private:
    bool _topleft; ///< true for top left point, false for bottom right
};

// Blur manipulation

class BlurKnotHolderEntity : public KnotHolderEntity
{
public:
    BlurKnotHolderEntity(int direction) : _dir(direction) {}
    void on_created() override;
    void update_knot() override;
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, unsigned state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state) override;

private:
    SPGaussianBlur *_blur() const;
    Geom::Point _pos() const;

    int _dir;
    CanvasItemPtr<Inkscape::CanvasItemCurve> _line;
    sigc::scoped_connection _watch_filter;
    sigc::scoped_connection _watch_blur;
};

#endif // SEEN_KNOT_HOLDER_ENTITY_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
