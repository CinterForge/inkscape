// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * KnotHolderEntity definition.
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2001 Mitsuru Oka
 * Copyright (C) 2004 Monash University
 * Copyright (C) 2008 Maximilian Albert
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "knot-holder-entity.h"

#include <glibmm/i18n.h>

#include "desktop.h"
#include "knot-holder.h"
#include "knot.h"
#include "live_effects/effect.h"
#include "object/filters/gaussian-blur.h"
#include "object/sp-hatch.h"
#include "object/sp-namedview.h"
#include "object/sp-pattern.h"
#include "preferences.h"
#include "style.h"

void KnotHolderEntity::create(SPDesktop *desktop, SPItem *item, KnotHolder *parent,
                              Inkscape::CanvasItemCtrlType type,
                              Glib::ustring const &name,
                              char const *tip, uint32_t color)
{
    if (!desktop) {
        desktop = parent->getDesktop();
    }

    g_assert(item == parent->getItem());
    g_assert(desktop && desktop == parent->getDesktop());
    g_assert(!knot);

    parent_holder = parent;
    this->item = item; // TODO: remove the item either from here or from knotholder.cpp
    this->desktop = desktop;

    knot = new SPKnot(desktop, tip, type, name);
    on_created();
    update_knot();
    knot->show();

    _mousedown_connection = knot->mousedown_signal.connect(sigc::mem_fun(*parent_holder, &KnotHolder::knot_mousedown_handler));
    _moved_connection = knot->moved_signal.connect(sigc::mem_fun(*parent_holder, &KnotHolder::knot_moved_handler));
    _click_connection = knot->click_signal.connect(sigc::mem_fun(*parent_holder, &KnotHolder::knot_clicked_handler));
    _ungrabbed_connection = knot->ungrabbed_signal.connect(sigc::mem_fun(*parent_holder, &KnotHolder::knot_ungrabbed_handler));
}

KnotHolderEntity::~KnotHolderEntity()
{
    if (knot) {
        // unref should call destroy
        SPKnot::unref(knot);
    } else {
        // FIXME: This shouldn't occur. Perhaps it is caused by LPE PointParams being knotholder entities, too
        //        If so, it will likely be fixed with upcoming refactoring efforts.
    }
}

void KnotHolderEntity::update_knot()
{
    if (auto const knot_pos = knot_get(); knot_pos.isFinite()) {
        auto const dp = knot_pos * parent_holder->getEditTransform() * item->i2dt_affine();
        _moved_connection.block();
        knot->setPosition(dp, SP_KNOT_STATE_NORMAL);
        _moved_connection.unblock();
    } else {
        // knot coords are non-finite, hide knot
        knot->hide();
    }
}

Geom::Point KnotHolderEntity::snap_knot_position(Geom::Point const &p, unsigned state)
{
    if (state & GDK_SHIFT_MASK) { // Don't snap when shift-key is held
        return p;
    }

    auto const i2dt = parent_holder->getEditTransform() * item->i2dt_affine();
    Geom::Point s = p * i2dt;

    if (!desktop) {
        std::cerr << "No desktop" << std::endl;
        return {};
    } else if (!desktop->getNamedView()) {
        std::cerr << "No named view" << std::endl;
        return {};
    }

    auto &m = desktop->getNamedView()->snap_manager;
    m.setup(desktop, true, item);
    m.freeSnapReturnByRef(s, Inkscape::SNAPSOURCE_NODE_HANDLE);
    m.unSetup();

    return s * i2dt.inverse();
}

Geom::Point KnotHolderEntity::snap_knot_position_constrained(Geom::Point const &p, Inkscape::Snapper::SnapConstraint const &constraint, unsigned state)
{
    if (state & GDK_SHIFT_MASK) { // Don't snap when shift-key is held
        return p;
    }

    auto const i2d = parent_holder->getEditTransform() * item->i2dt_affine();
    Geom::Point s = p * i2d;

    auto &m = desktop->getNamedView()->snap_manager;
    m.setup(desktop, true, item);

    // constrainedSnap() will first project the point p onto the constraint line and then try to snap along that line.
    // This way the constraint is already enforced, no need to worry about that later on
    Inkscape::Snapper::SnapConstraint transformed_constraint = Inkscape::Snapper::SnapConstraint(constraint.getPoint() * i2d, (constraint.getPoint() + constraint.getDirection()) * i2d - constraint.getPoint() * i2d);
    m.constrainedSnapReturnByRef(s, Inkscape::SNAPSOURCE_NODE_HANDLE, transformed_constraint);
    m.unSetup();

    return s * i2d.inverse();
}

// Pattern manipulation

void PatternKnotHolderEntity::on_created()
{
    // Setup an initial pattern transformation in the center
    if (auto rect = item->documentGeometricBounds()) {
        _cell = offset_to_cell(rect->midpoint());
    }
}

/**
 * Returns the position based on the pattern's origin, shifted by the percent x/y of its size.
 */
Geom::Point PatternKnotHolderEntity::_get_pos(double x, double y, bool transform) const
{
    auto pat = _pattern();
    auto pt = (_cell + Geom::Point(x, y)) * Geom::Point(pat->width(), pat->height());
    return transform ? pt * pat->getTransform() : pt;
}

bool PatternKnotHolderEntity::set_item_clickpos(Geom::Point loc)
{
    _cell = offset_to_cell(loc);
    update_knot();
    return true;
}

void PatternKnotHolderEntity::update_knot()
{
    KnotHolderEntity::update_knot();
}

Geom::IntPoint PatternKnotHolderEntity::offset_to_cell(Geom::Point loc) const
{
    auto pat = _pattern();

    // 1. Turn the location into the pattern grid coordinate
    auto scale = Geom::Scale(pat->width(), pat->height());
    auto d2i = item->i2doc_affine().inverse();
    auto i2p = pat->getTransform().inverse();

    // Get grid index of nearest pattern repetition.
    return (loc * d2i * i2p * scale.inverse()).floor();
}


SPPattern *PatternKnotHolderEntity::_pattern() const
{
    return cast<SPPattern>(_fill ? item->style->getFillPaintServer() : item->style->getStrokePaintServer());
}

bool PatternKnotHolderEntity::knot_missing() const
{
    return !_pattern();
}

// Pattern X/Y knot

void PatternKnotHolderEntityXY::on_created()
{
    PatternKnotHolderEntity::on_created();
    // TODO: Move to constructor when desktop is generally available
    _quad = make_canvasitem<Inkscape::CanvasItemQuad>(desktop->getCanvasControls());
    _quad->lower_to_bottom();
    _quad->set_fill(0x00000000);
    _quad->set_stroke(0x808080ff);
    _quad->set_inverted(true);
    _quad->set_visible(false);
}

void PatternKnotHolderEntityXY::update_knot()
{
    PatternKnotHolderEntity::update_knot();
    auto tr = item->i2dt_affine();
    _quad->set_coords(_get_pos(0, 0) * tr, _get_pos(0, 1) * tr,
                      _get_pos(1, 1) * tr, _get_pos(1, 0) * tr);
    _quad->set_visible(true);
}

Geom::Point PatternKnotHolderEntityXY::knot_get() const
{
    return _get_pos(0, 0);
}

void PatternKnotHolderEntityXY::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state)
{
    auto p_snapped = snap_knot_position(p, state);

    if (state & GDK_CONTROL_MASK) {
        if (std::abs((p - origin).x()) > std::abs((p - origin).y())) {
            p_snapped.y() = origin.y();
        } else {
            p_snapped.x() = origin.x();
        }
    }

    if (state) {
        auto const q = p_snapped - knot_get();
        item->adjust_pattern(Geom::Translate(q), false, _fill ? TRANSFORM_FILL : TRANSFORM_STROKE);
    }

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// Pattern Angle knot

Geom::Point PatternKnotHolderEntityAngle::knot_get() const
{
    return _get_pos(1.0, 0);
}

void PatternKnotHolderEntityAngle::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned state)
{
    auto prefs = Inkscape::Preferences::get();
    double const snaps = prefs->getDoubleLimited("/options/rotationsnapsperpi/value", 12.0, 0.1, 1800.0);

    // get the angle from pattern 0,0 to the cursor pos
    Geom::Point transform_origin = _get_pos(0, 0);
    double theta = atan2(p - transform_origin);
    double theta_old = atan2(knot_get() - transform_origin);

    if (state & GDK_CONTROL_MASK) {
        // Snap theta
        double snaps_radian = M_PI / snaps;
        theta = std::round(theta / snaps_radian) * snaps_radian;
    }

    Geom::Affine rot = Geom::Translate(-transform_origin)
                     * Geom::Rotate(theta - theta_old)
                     * Geom::Translate(transform_origin);
    item->adjust_pattern(rot, false, _fill ? TRANSFORM_FILL : TRANSFORM_STROKE);
    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// Pattern scale knot

Geom::Point PatternKnotHolderEntityScale::knot_get() const
{
    return _get_pos(1.0, 1.0);
}

/** Store pattern geometry info when the scale knot is first grabbed. */
void PatternKnotHolderEntityScale::knot_grabbed(Geom::Point const &grab_pos, unsigned)
{
    _cached_transform = _pattern()->getTransform();
    _cached_origin = _get_pos(0, 0);
    _cached_inverse_linear = _cached_transform.withoutTranslation().inverse();
    _cached_diagonal = (grab_pos - _cached_origin) * _cached_inverse_linear;

    if (auto bounding_box = item->documentVisualBounds()) {
        // Compare the areas of the pattern and the item to find the number of repetitions.
        double const pattern_area = std::abs(_cached_diagonal[Geom::X] * _cached_diagonal[Geom::Y]);
        double const item_area = bounding_box->area() * _cached_inverse_linear.descrim2() /
                                 (item->i2doc_affine().descrim2() ? 1.0 : 1e-3);
        _cached_min_scale = std::sqrt(item_area / (pattern_area * MAX_REPETITIONS));
    } else {
        _cached_min_scale = 1e-6;
    }
}

void PatternKnotHolderEntityScale::knot_set(Geom::Point const &p, Geom::Point const &, unsigned state)
{
    using namespace Geom;
    // FIXME: this snapping should be done together with knowing whether control was pressed.
    // If GDK_CONTROL_MASK, then constrained snapping should be used.
    Point p_snapped = snap_knot_position(p, state);

    Point const new_extent = (p_snapped - _cached_origin) * _cached_inverse_linear;

    // 1. Calculate absolute scale factor first
    double scale_x = std::clamp(new_extent[X] / _cached_diagonal[X], _cached_min_scale, 1e9);
    double scale_y = std::clamp(new_extent[Y] / _cached_diagonal[Y], _cached_min_scale, 1e9);

    Affine new_transform = (state & GDK_CONTROL_MASK) ? Scale((scale_x + scale_y) * 0.5)
                                                      : Scale(scale_x, scale_y);

    // 2. Calculate offset to keep pattern origin aligned
    new_transform *= _cached_transform;
    auto const new_uncompensated_origin = _get_pos(0, 0, false) * new_transform;
    new_transform *= Translate(_cached_origin - new_uncompensated_origin);

    item->adjust_pattern(new_transform, true, _fill ? TRANSFORM_FILL : TRANSFORM_STROKE);
    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// Hatch manipulation

bool HatchKnotHolderEntity::knot_missing() const
{
    return !_hatch();
}

SPHatch *HatchKnotHolderEntity::_hatch() const
{
    return cast<SPHatch>(_fill ? item->style->getFillPaintServer() : item->style->getStrokePaintServer());
}

static Geom::Point sp_hatch_knot_get(SPHatch const *hatch, double x, double y)
{
    return Geom::Point(x, y) * hatch->hatchTransform();
}

Geom::Point HatchKnotHolderEntityXY::knot_get() const
{
    return sp_hatch_knot_get(_hatch(), 0, 0);
}

Geom::Point HatchKnotHolderEntityAngle::knot_get() const
{
    auto hatch = _hatch();
    return sp_hatch_knot_get(hatch, hatch->pitch(), 0);
}

Geom::Point HatchKnotHolderEntityScale::knot_get() const
{
    auto hatch = _hatch();
    auto pitch = hatch->pitch();
    return sp_hatch_knot_get(hatch, pitch, pitch);
}

void HatchKnotHolderEntityXY::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state)
{
    auto p_snapped = snap_knot_position(p, state);

    if (state & GDK_CONTROL_MASK) {
        if (std::abs((p - origin).x()) > std::abs((p - origin).y())) {
            p_snapped.y() = origin.y();
        } else {
            p_snapped.x() = origin.x();
        }
    }

    if (state) {
        auto const q = p_snapped - knot_get();
        item->adjust_hatch(Geom::Translate(q), false, _fill ? TRANSFORM_FILL : TRANSFORM_STROKE);
    }

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void HatchKnotHolderEntityAngle::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    double const snaps = prefs->getDoubleLimited("/options/rotationsnapsperpi/value", 12.0, 0.1, 1800.0);

    SPHatch *hatch = _hatch();

    // get the angle from hatch 0,0 to the cursor pos
    auto const transform_origin = sp_hatch_knot_get(hatch, 0, 0);
    double theta = atan2(p - transform_origin);
    double theta_old = atan2(knot_get() - transform_origin);

    if (state & GDK_CONTROL_MASK) {
        // Snap theta
        double snaps_radian = M_PI / snaps;
        theta = std::round(theta / snaps_radian) * snaps_radian;
    }

    Geom::Affine rot =
        Geom::Translate(-transform_origin) * Geom::Rotate(theta - theta_old) * Geom::Translate(transform_origin);
    item->adjust_hatch(rot, false, _fill ? TRANSFORM_FILL : TRANSFORM_STROKE);
    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void HatchKnotHolderEntityScale::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state)
{
    SPHatch *hatch = _hatch();

    // FIXME: this snapping should be done together with knowing whether control was pressed.
    // If GDK_CONTROL_MASK, then constrained snapping should be used.
    Geom::Point p_snapped = snap_knot_position(p, state);

    // Get the new scale from the position of the knotholder
    Geom::Affine transform = hatch->hatchTransform();
    Geom::Affine transform_inverse = transform.inverse();
    Geom::Point d = p_snapped * transform_inverse;
    Geom::Point d_origin = origin * transform_inverse;
    Geom::Point origin_dt;
    double hatch_pitch = hatch->pitch();
    if (state & GDK_CONTROL_MASK) {
        // if ctrl is pressed: use 1:1 scaling
        d = d_origin * (d.length() / d_origin.length());
    }

    Geom::Affine scale = Geom::Translate(-origin_dt) * Geom::Scale(d.x() / hatch_pitch, d.y() / hatch_pitch) *
                         Geom::Translate(origin_dt) * transform;

    item->adjust_hatch(scale, true, _fill ? TRANSFORM_FILL : TRANSFORM_STROKE);
    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// Filter manipulation

void FilterKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state)
{
    // FIXME: this snapping should be done together with knowing whether control was pressed. If GDK_CONTROL_MASK, then constrained snapping should be used.
    Geom::Point p_snapped = snap_knot_position(p, state);

    if (state & GDK_CONTROL_MASK) {
        if (std::abs((p - origin).x()) > std::abs((p - origin).y())) {
            p_snapped.y() = origin.y();
        } else {
            p_snapped.x() = origin.x();
        }
    }

    if (state)  {
        auto filter = item->style ? item->style->getFilter() : nullptr;
        if (!filter) {
            return;
        }

        auto const orig_bbox = item->visualBounds();
        auto const new_bbox = _topleft ? Geom::Rect(p, orig_bbox->max()) : Geom::Rect(orig_bbox->min(), p);

        if (!filter->width._set) {
            filter->width.set(SVGLength::PERCENT, 1.2);
        }
        if (!filter->height._set) {
            filter->height.set(SVGLength::PERCENT, 1.2);
        }
        if (!filter->x._set) {
            filter->x.set(SVGLength::PERCENT, -0.1);
        }
        if (!filter->y._set) {
            filter->y.set(SVGLength::PERCENT, -0.1);
        }

        if (_topleft) {
            float x_a = filter->width.computed;
            float y_a = filter->height.computed;
            filter->height.scale(new_bbox.height() / orig_bbox->height());
            filter->width.scale(new_bbox.width() / orig_bbox->width());
            float x_b = filter->width.computed;
            float y_b = filter->height.computed;
            filter->x.set(filter->x.unit, filter->x.computed + x_a - x_b);
            filter->y.set(filter->y.unit, filter->y.computed + y_a - y_b);
        } else {
            filter->height.scale(new_bbox.height() / orig_bbox->height());
            filter->width.scale(new_bbox.width() / orig_bbox->width());
        }
        filter->auto_region = false;
        filter->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

Geom::Point FilterKnotHolderEntity::knot_get() const
{
    auto filter = item->style ? item->style->getFilter() : nullptr;
    if (!filter) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    auto r = item->visualBounds();
    return _topleft ? r->min() : r->max();
}

// Blur manipulation

void BlurKnotHolderEntity::on_created()
{
    KnotHolderEntity::on_created();
    // TODO: Move to constructor when desktop is generally available

    _line = make_canvasitem<Inkscape::CanvasItemCurve>(desktop->getCanvasControls());
    _line->set_z_position(0);
    _line->set_stroke(0x0033cccc);
    _line->set_visible(false);

    // This watcher makes sure that adding or removing a blur results in updated knots.
    _watch_filter = item->style->signal_filter_changed.connect([this] (auto old_obj, auto obj) {
        update_knot();
    }); 
}

void BlurKnotHolderEntity::update_knot()
{
    if (auto blur = _blur()) {
        knot->show();
        // This watcher makes sure anything outside that modifies the blur changes the knot.
        _watch_blur = blur->connectModified([this] (auto item, unsigned flags) {
            KnotHolderEntity::update_knot();
        });
    } else {
        knot->hide();
        _watch_blur.disconnect();
        _line->set_visible(false);
    }
    KnotHolderEntity::update_knot();
}

/* Return the first blur primitive of any applied filter. */
SPGaussianBlur *BlurKnotHolderEntity::_blur() const
{
    if (auto filter = item->style->getFilter()) {
        for (auto &primitive : filter->children) {
            if (auto blur = cast<SPGaussianBlur>(&primitive)) {
                return blur;
            }
        }
    }
    return nullptr;
}

Geom::Point BlurKnotHolderEntity::_pos() const
{
    auto box = item->bbox(Geom::identity(), SPItem::VISUAL_BBOX);
    if (box.empty()) {
        return Geom::Point(Geom::infinity(), Geom::infinity());
    }
    if (_dir == Geom::Y) {
        return Geom::Point(box->midpoint()[Geom::X], box->top());
    }
    return Geom::Point(box->right(), box->midpoint()[Geom::Y]);
}

Geom::Point BlurKnotHolderEntity::knot_get() const
{
    auto blur = _blur();
    if (!blur)
        return Geom::Point(0, 0);

    // First let's find where the gradient is
    auto tr = item->i2dt_affine();
    auto dev = blur->get_std_deviation();

    // Blur visibility is 2.4 times the deviation in that direction.
    double x = dev.getNumber();
    double y = dev.getOptNumber(true);

    auto p0 = _pos();
    auto p1 = p0 + Geom::Point(x * 2.4, 0);
    if (_dir == Geom::Y) {
        p1 = p0 - Geom::Point(0, y * 2.4);
    }
    _line->set_visible(true);
    _line->set_coords(p0 * tr, p1 * tr);

    return p1;
}

void BlurKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned state)
{
    auto blur = _blur();
    if (!blur)
        return;

    NumberOptNumber dev = blur->get_std_deviation();
    auto dp = Geom::Point(dev.getNumber(), dev.getOptNumber(true));
    auto val = std::max(0.0, (((p - _pos()) * Geom::Scale(1, -1))[_dir]) / 2.4);

    if (state & GDK_CONTROL_MASK) {
        if (state & GDK_SHIFT_MASK) {
            dp[!_dir] *= val / dp[_dir];
        } else {
            dp[!_dir] = val;
        }
    }
    dp[_dir] = val;

    // When X is set to zero the Opt blur disapears
    dev.setNumber(std::max(0.001, dp[Geom::X]));
    dev.setOptNumber(std::max(0.0, dp[Geom::Y]));

    blur->set_deviation(dev);
}

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
