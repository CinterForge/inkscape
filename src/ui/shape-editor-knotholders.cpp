// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Node editing extension to objects
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

// Declared in shape-editor.cpp.

#include <iomanip>
#include <glibmm/i18n.h>

#include "knot/knot.h"
#include "livarot/Path.h"
#include "live_effects/effect.h"
#include "object/box3d.h"
#include "object/sp-ellipse.h"
#include "object/sp-flowtext.h"
#include "object/sp-marker.h"
#include "object/sp-offset.h"
#include "object/sp-pattern.h"
#include "object/sp-rect.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "object/sp-textpath.h"
#include "path/path-curve.h"
#include "style.h"
#include "svg/css-ostringstream.h"
#include "ui/icon-names.h"
#include "ui/knot/knot-holder-entity.h"
#include "ui/knot/knot-holder.h"
#include "ui/modifiers.h"
#include "ui/popup-menu.h"
#include "ui/widget/canvas-grid.h"
#include "ui/widget/canvas.h"
#include "ui/widget/desktop-widget.h"
#include "ui/widget/textpath-popover.h"

class RectKnotHolder : public KnotHolder {
public:
    RectKnotHolder(SPDesktop *desktop, SPItem *item);
    ~RectKnotHolder() override = default;;
};

class Box3DKnotHolder : public KnotHolder {
public:
    Box3DKnotHolder(SPDesktop *desktop, SPItem *item);
    ~Box3DKnotHolder() override = default;;
};

class MarkerKnotHolder : public KnotHolder {
public:
    MarkerKnotHolder(SPDesktop *desktop, SPItem *item, double edit_rotation, int edit_marker_mode);
    ~MarkerKnotHolder() override = default;;
};

class ArcKnotHolder : public KnotHolder {
public:
    ArcKnotHolder(SPDesktop *desktop, SPItem *item);
    ~ArcKnotHolder() override = default;;
};

class StarKnotHolder : public KnotHolder {
public:
    StarKnotHolder(SPDesktop *desktop, SPItem *item);
    ~StarKnotHolder() override = default;;
};

class SpiralKnotHolder : public KnotHolder {
public:
    SpiralKnotHolder(SPDesktop *desktop, SPItem *item);
    ~SpiralKnotHolder() override = default;;
};

class OffsetKnotHolder : public KnotHolder {
public:
    OffsetKnotHolder(SPDesktop *desktop, SPItem *item);
    ~OffsetKnotHolder() override = default;;
};

class TextKnotHolder : public KnotHolder {
public:
    TextKnotHolder(SPDesktop *desktop, SPItem *item);
    ~TextKnotHolder() override = default;;
};

class TextPathKnotHolder : public KnotHolder
{
public:
    TextPathKnotHolder(SPDesktop *desktop, SPText *text);
    ~TextPathKnotHolder() override = default;
};

class FlowtextKnotHolder : public KnotHolder {
public:
    FlowtextKnotHolder(SPDesktop *desktop, SPItem *item);
    ~FlowtextKnotHolder() override = default;;
};

class MiscKnotHolder : public KnotHolder {
public:
    MiscKnotHolder(SPDesktop *desktop, SPItem *item);
    ~MiscKnotHolder() override = default;;
};

namespace {

std::unique_ptr<KnotHolder> make_lpe_knot_holder(SPLPEItem *item, SPDesktop *desktop)
{
    auto knot_holder = std::make_unique<KnotHolder>(desktop, item);

    item->getCurrentLPE()->addHandles(knot_holder.get(), item);
    for (auto i : knot_holder->entity) {
        i->knot->is_lpe = true;
    }
    return knot_holder;
}
} // namespace

namespace Inkscape {
namespace UI {

std::unique_ptr<KnotHolder> create_knot_holder(SPItem *item, SPDesktop *desktop, double edit_rotation,
                                               int edit_marker_mode)
{
    std::unique_ptr<KnotHolder> knotholder;

    if (is<SPRect>(item)) {
        knotholder = std::make_unique<RectKnotHolder>(desktop, item);
    } else if (is<SPBox3D>(item)) {
        knotholder = std::make_unique<Box3DKnotHolder>(desktop, item);
    } else if (is<SPMarker>(item)) {
        knotholder = std::make_unique<MarkerKnotHolder>(desktop, item, edit_rotation, edit_marker_mode);
    } else if (is<SPGenericEllipse>(item)) {
        knotholder = std::make_unique<ArcKnotHolder>(desktop, item);
    } else if (is<SPStar>(item)) {
        knotholder = std::make_unique<StarKnotHolder>(desktop, item);
    } else if (is<SPSpiral>(item)) {
        knotholder = std::make_unique<SpiralKnotHolder>(desktop, item);
    } else if (is<SPOffset>(item)) {
        knotholder = std::make_unique<OffsetKnotHolder>(desktop, item);
    } else if (auto text = cast<SPText>(item)) {
        // Do not allow conversion to 'inline-size' wrapped text if on path!
        // <textPath> might not be first child if <title> or <desc> is present.
        if (auto const text_children = text->childList(false);
            std::any_of(text_children.begin(), text_children.end(), is<SPTextPath>)) {
            knotholder = std::make_unique<TextPathKnotHolder>(desktop, text);
        } else {
            knotholder = std::make_unique<TextKnotHolder>(desktop, item);
        }
    } else {
        auto flowtext = cast<SPFlowtext>(item);
        if (flowtext && flowtext->has_internal_frame()) {
            knotholder = std::make_unique<FlowtextKnotHolder>(desktop, flowtext->get_frame(nullptr));
        } else if ((item->style->fill.isPaintserver() && cast<SPPattern>(item->style->getFillPaintServer())) ||
                   (item->style->stroke.isPaintserver() && cast<SPPattern>(item->style->getStrokePaintServer()))) {
            knotholder = std::make_unique<KnotHolder>(desktop, item);
            knotholder->add_pattern_knotholder();
        }
    }
    if (!knotholder) {
        knotholder = std::make_unique<KnotHolder>(desktop, item);
    }
    knotholder->add_filter_knotholder();

    return knotholder;
}

std::unique_ptr<KnotHolder> create_LPE_knot_holder(SPItem *item, SPDesktop *desktop)
{
    auto lpe = cast<SPLPEItem>(item);
    if (lpe &&
        lpe->getCurrentLPE() &&
        lpe->getCurrentLPE()->isVisible() &&
        lpe->getCurrentLPE()->providesKnotholder()) {
        return make_lpe_knot_holder(lpe, desktop);
    }
    return {};
}

}
} // namespace Inkscape

/* SPRect */

/* handle for horizontal rounding radius */
class RectKnotHolderEntityRX : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

    std::string get_mod_label() const { return mod_trans_confine->get_label(); }

protected:
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

/* handle for vertical rounding radius */
class RectKnotHolderEntityRY : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

    std::string get_mod_label() const { return mod_trans_confine->get_label(); }

protected:
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

/* handle for width/height adjustment */
class RectKnotHolderEntityWH : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_grabbed(Geom::Point const & grab_position, unsigned state) override;

    std::string get_mod_label() const { return mod_trans_confine->get_label(); }

protected:
    void set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state);
    void set_internal(Geom::Point const &opposite_point);

private:
    Geom::Point initialXY;
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

/* handle for x/y adjustment */
class RectKnotHolderEntityXY : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_grabbed(Geom::Point const & grab_position, unsigned state) override;

    std::string get_mod_label() const { return mod_trans_confine->get_label(); }

private:
    void set_internal(Geom::Point const &opposite_of_the_opposite_point);
    Geom::Point initial_opposite_XY;
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

/* handle for position */
class RectKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
RectKnotHolderEntityRX::knot_get() const
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed - rect->rx.computed, rect->y.computed);
}

void
RectKnotHolderEntityRX::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    //In general we cannot just snap this radius to an arbitrary point, as we have only a single
    //degree of freedom. For snapping to an arbitrary point we need two DOF. If we're going to snap
    //the radius then we should have a constrained snap. snap_knot_position() is unconstrained
    Geom::Point const s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed), Geom::Point(-1, 0)), state);

    if (mod_trans_confine->active(state)) {
        gdouble temp = MIN(rect->height.computed, rect->width.computed) / 2.0;
        rect->rx = rect->ry = CLAMP(rect->x.computed + rect->width.computed - s[Geom::X], 0.0, temp);
    } else {
        rect->rx = CLAMP(rect->x.computed + rect->width.computed - s[Geom::X], 0.0, rect->width.computed / 2.0);
    }

    update_knot();

    rect->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void
RectKnotHolderEntityRX::knot_click(unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    if (state & GDK_SHIFT_MASK) {
        /* remove rounding from rectangle */
        rect->getRepr()->removeAttribute("rx");
        rect->getRepr()->removeAttribute("ry");
    } else if (mod_trans_confine->active(state)) {
        /* Ctrl-click sets the vertical rounding to be the same as the horizontal */
        rect->getRepr()->setAttribute("ry", rect->getRepr()->attribute("rx"));
    }
}

Geom::Point
RectKnotHolderEntityRY::knot_get() const
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->ry.computed);
}

void
RectKnotHolderEntityRY::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    //In general we cannot just snap this radius to an arbitrary point, as we have only a single
    //degree of freedom. For snapping to an arbitrary point we need two DOF. If we're going to snap
    //the radius then we should have a constrained snap. snap_knot_position() is unconstrained
    Geom::Point const s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed), Geom::Point(0, 1)), state);

    if (mod_trans_confine->active(state)) { // When holding control then rx will be kept equal to ry,
                                            // resulting in a perfect circle (and not an ellipse)
        gdouble temp = MIN(rect->height.computed, rect->width.computed) / 2.0;
        rect->rx = rect->ry = CLAMP(s[Geom::Y] - rect->y.computed, 0.0, temp);
    } else {
        if (!rect->rx._set || rect->rx.computed == 0) {
            rect->ry = CLAMP(s[Geom::Y] - rect->y.computed,
                             0.0,
                             MIN(rect->height.computed / 2.0, rect->width.computed / 2.0));
        } else {
            rect->ry = CLAMP(s[Geom::Y] - rect->y.computed,
                             0.0,
                             rect->height.computed / 2.0);
        }
    }

    update_knot();

    rect->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void
RectKnotHolderEntityRY::knot_click(unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    if (state & GDK_SHIFT_MASK) {
        /* remove rounding */
        rect->getRepr()->removeAttribute("rx");
        rect->getRepr()->removeAttribute("ry");
    } else if (mod_trans_confine->active(state)) {
        /* Ctrl-click sets the vertical rounding to be the same as the horizontal */
        rect->getRepr()->setAttribute("rx", rect->getRepr()->attribute("ry"));
    }
}

#define SGN(x) ((x)>0?1:((x)<0?-1:0))

static void sp_rect_clamp_radii(SPRect *rect)
{
    // clamp rounding radii so that they do not exceed width/height
    if (2 * rect->rx.computed > rect->width.computed) {
        rect->rx = 0.5 * rect->width.computed;
    }
    if (2 * rect->ry.computed > rect->height.computed) {
        rect->ry = 0.5 * rect->height.computed;
    }
}

Geom::Point
RectKnotHolderEntityWH::knot_get() const
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed);
}

void
RectKnotHolderEntityWH::set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    Geom::Point s = p;

    if (mod_trans_confine->active(state)) {
        // original width/height when drag started
        gdouble const w_orig = (origin[Geom::X] - initialXY.x());
        gdouble const h_orig = (origin[Geom::Y] - initialXY.y());

        //original ratio
        gdouble ratio = (w_orig / h_orig);

        // mouse displacement since drag started
        gdouble minx = p[Geom::X] - origin[Geom::X];
        gdouble miny = p[Geom::Y] - origin[Geom::Y];

        Geom::Point p_handle(
            initialXY.x() + rect->width.computed * (initialXY.x() < p[Geom::X] ? 1 : -1),
            initialXY.y() + rect->height.computed * (initialXY.y() < p[Geom::Y] ? 1 : -1));

        double oppositeX = initialXY.x();
        double oppositeY = initialXY.y();
        if (fabs(minx) > fabs(miny)) {
            // snap to horizontal or diagonal
            if (minx != 0 && fabs(miny/minx) > 0.5 * 1/ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                oppositeY += h_orig + minx / ratio;
            } else {
                // closer to the horizontal, change only width, height is h_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-1, 0)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                oppositeY += h_orig;
            }
            oppositeX += + w_orig + minx;

        } else {
            // snap to vertical or diagonal
            if (miny != 0 && fabs(minx/miny) > 0.5 * ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                oppositeX += w_orig + miny * ratio;
            } else {
                // closer to the vertical, change only height, width is w_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(0, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                oppositeX += + w_orig;
            }
            oppositeY += h_orig + miny;
        }
        set_internal(Geom::Point(oppositeX, oppositeY));
    } else {
        // move freely
        s = snap_knot_position(p, state);
        set_internal(s);
    }

    sp_rect_clamp_radii(rect);

    rect->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void RectKnotHolderEntityWH::set_internal(Geom::Point const &opposite_point)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    auto geomRect = Geom::Rect(initialXY, opposite_point);
    rect->x = geomRect.left();
    rect->y = geomRect.top();
    rect->width = geomRect.width();
    rect->height = geomRect.height();
}

void
RectKnotHolderEntityWH::knot_grabbed(Geom::Point const & grab_position, unsigned state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    initialXY = Geom::Point(rect->x.computed, rect->y.computed);
}

void
RectKnotHolderEntityWH::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    set_internal(p, origin, state);
    update_knot();
}

Geom::Point
RectKnotHolderEntityXY::knot_get() const
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed, rect->y.computed);
}

void
RectKnotHolderEntityXY::knot_grabbed(Geom::Point const & grab_position, unsigned state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    initial_opposite_XY = Geom::Point(
        rect->x.computed + rect->width.computed,
        rect->y.computed + rect->height.computed);
}

void
RectKnotHolderEntityXY::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    Geom::Point s = p;
    gdouble opposite_of_opposite_X, opposite_of_opposite_Y;
    if (mod_trans_confine->active(state)) {
        // original width/height when drag started
        gdouble w_orig = initial_opposite_XY.x() - origin[Geom::X];
        gdouble h_orig = initial_opposite_XY.y() - origin[Geom::Y];

        //original ratio
        gdouble ratio = (w_orig / h_orig);

        // mouse displacement since drag started
        gdouble minx = p[Geom::X] - origin[Geom::X];
        gdouble miny = p[Geom::Y] - origin[Geom::Y];

        Geom::Point p_handle(
           rect->x.computed + (rect->x.computed < initial_opposite_XY.x() ? 0 : rect->width.computed),
           rect->y.computed + (rect->y.computed < initial_opposite_XY.y() ? 0 : rect->height.computed));

        if (fabs(minx) > fabs(miny)) {
            // snap to horizontal or diagonal
            if (minx != 0 && fabs(miny/minx) > 0.5 * 1/ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                opposite_of_opposite_Y = origin[Geom::Y] + minx / ratio;
            } else {
                // closer to the horizontal, change only width, height is h_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-1, 0)), state);
                minx = s[Geom::X] - origin[Geom::X];
                // Dead assignment: Value stored to 'miny' is never read
                //miny = s[Geom::Y] - origin[Geom::Y];
                opposite_of_opposite_Y = origin[Geom::Y];
            }
            opposite_of_opposite_X = s[Geom::X];
        } else {
            // snap to vertical or diagonal
            if (miny != 0 && fabs(minx/miny) > 0.5 *ratio && (SGN(minx) == SGN(miny))) {
                // closer to the diagonal and in same-sign quarters, change both using ratio
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(-ratio, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                opposite_of_opposite_X = origin[Geom::X] + miny * ratio;
            } else {
                // closer to the vertical, change only height, width is w_orig
                s = snap_knot_position_constrained(p, Inkscape::Snapper::SnapConstraint(p_handle, Geom::Point(0, -1)), state);
                // Dead assignment: Value stored to 'minx' is never read
                //minx = s[Geom::X] - origin[Geom::X];
                miny = s[Geom::Y] - origin[Geom::Y];
                opposite_of_opposite_X = origin[Geom::X];

            }
            opposite_of_opposite_Y = s[Geom::Y];
        }
        set_internal(Geom::Point(opposite_of_opposite_X, opposite_of_opposite_Y));
    } else {
        // move freely
        s = snap_knot_position(p, state);
        set_internal(s);
    }

    sp_rect_clamp_radii(rect);

    update_knot();

    rect->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void
RectKnotHolderEntityXY::set_internal(Geom::Point const &opposite_of_the_opposite_point)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    auto geomRect = Geom::Rect(initial_opposite_XY, opposite_of_the_opposite_point);
    rect->x = geomRect.left();
    rect->y = geomRect.top();
    rect->width = geomRect.width();
    rect->height = geomRect.height();
}

Geom::Point
RectKnotHolderEntityCenter::knot_get() const
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + (rect->width.computed / 2.), rect->y.computed + (rect->height.computed / 2.));
}

void
RectKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    rect->x = s[Geom::X] - (rect->width.computed / 2.);
    rect->y = s[Geom::Y] - (rect->height.computed / 2.);

    // No need to call sp_rect_clamp_radii(): width and height haven't changed.
    // No need to call update_knot(): the knot is set directly by the user.

    rect->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

RectKnotHolder::RectKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    RectKnotHolderEntityRX *entity_rx = new RectKnotHolderEntityRX();
    RectKnotHolderEntityRY *entity_ry = new RectKnotHolderEntityRY();
    RectKnotHolderEntityWH *entity_wh = new RectKnotHolderEntityWH();
    RectKnotHolderEntityXY *entity_xy = new RectKnotHolderEntityXY();
    RectKnotHolderEntityCenter *entity_center = new RectKnotHolderEntityCenter();

    entity_rx->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Rect:rx",
                      Glib::ustring::compose(_("Adjust the <b>horizontal rounding</b> radius; with <b>%1</b> to make the vertical radius the same"),
                          entity_rx->get_mod_label())
                            .c_str());

    entity_ry->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Rect:ry",
                      Glib::ustring::compose(_("Adjust the <b>vertical rounding</b> radius; with <b>%1</b> to make the horizontal radius the same"),
                         entity_ry->get_mod_label())
                            .c_str());

    entity_wh->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Rect:wh",
                      Glib::ustring::compose(_("Adjust the <b>width and height</b> of the rectangle; with <b>%1</b> to lock ratio or stretch in one dimension only"),
                         entity_wh->get_mod_label())
                            .c_str());

    entity_xy->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Rect:xy",
                      Glib::ustring::compose(_("Adjust the <b>width and height</b> of the rectangle; with <b>%1</b> to lock ratio or stretch in one dimension only"),
                         entity_xy->get_mod_label())
                            .c_str());

    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Rect:center",
                          _("Drag to move the rectangle"));

    entity.push_back(entity_rx);
    entity.push_back(entity_ry);
    entity.push_back(entity_wh);
    entity.push_back(entity_xy);
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* Box3D (= the new 3D box structure) */

class Box3DKnotHolderEntity : public KnotHolderEntity {
public:
    Box3DKnotHolderEntity(unsigned int knot_id) : knot_id(knot_id) {}
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    bool default_direction_is_xy() const;

    std::string get_modconfine_label() const { return mod_trans_confine->get_label(); }
    std::string get_modchangedir_label() const { return mod_change_direction->get_label(); }

private:
    unsigned int knot_id;
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
    Modifiers::Modifier *mod_change_direction = Modifiers::Modifier::get(Modifiers::Type::BOX3D_CHANGE_KNOT_DIRECTION);
};

Geom::Point
Box3DKnotHolderEntity::knot_get() const
{
    if (auto box = cast<SPBox3D>(item)) {
        return box->get_corner_screen(knot_id);
    }
    return Geom::Point(); // TODO investigate proper fallback
}

void
Box3DKnotHolderEntity::knot_set(Geom::Point const &new_pos, Geom::Point const &/*origin*/, unsigned int state)
{
    Geom::Point const s = snap_knot_position(new_pos, state);

    g_assert(item != nullptr);
    auto box = cast<SPBox3D>(item);
    g_assert(box != nullptr);
    Geom::Affine const i2dt (item->i2dt_affine ());

    Box3D::Axis movement;
    if (default_direction_is_xy() != mod_change_direction->active(state)) {
        movement = Box3D::XY;
    } else {
        movement = Box3D::Z;
    }

    box->set_corner (knot_id, s * i2dt, movement, mod_trans_confine->active(state));
    box->set_z_orders();
    box->position_set();
}

bool Box3DKnotHolderEntity::default_direction_is_xy() const
{
    return knot_id < 4;
}

class Box3DKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;

private:
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
    Modifiers::Modifier *mod_change_direction = Modifiers::Modifier::get(Modifiers::Type::BOX3D_CHANGE_KNOT_DIRECTION);
};

Geom::Point
Box3DKnotHolderEntityCenter::knot_get() const
{
    if (auto box = cast<SPBox3D>(item)) {
        return box->get_center_screen();
    }
    return Geom::Point(); // TODO investigate proper fallback
}

void
Box3DKnotHolderEntityCenter::knot_set(Geom::Point const &new_pos, Geom::Point const &origin, unsigned int state)
{
    Geom::Point const s = snap_knot_position(new_pos, state);

    auto box = cast<SPBox3D>(item);
    g_assert(box != nullptr);
    Geom::Affine const i2dt (item->i2dt_affine ());

    box->set_center(s * i2dt, origin * i2dt, !mod_change_direction->active(state) ? Box3D::XY : Box3D::Z,
                    mod_trans_confine->active(state));

    box->set_z_orders();
    box->position_set();
}

Box3DKnotHolder::Box3DKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    for (int knot_id = 0; knot_id < 8; ++knot_id) {
        auto *entity_corner = new Box3DKnotHolderEntity(knot_id);
        auto tooltip = _("Resize box in X/Y direction; with <b>%1</b> along the Z axis; with <b>%2</b> to constrain to the directions of edges or diagonals");
        if (!entity_corner->default_direction_is_xy()) {
            tooltip = _("Resize box along the Z axis; with <b>%1</b> in X/Y direction; with <b>%2</b> to constrain to the directions of edges or diagonals");
        }
        auto tooltip_with_shortcuts = Glib::ustring::compose(tooltip, entity_corner->get_modchangedir_label(), entity_corner->get_modconfine_label());
        entity_corner->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Box3D:corner" + std::to_string(knot_id),
                              tooltip_with_shortcuts.c_str());

        entity.push_back(entity_corner);
    }

    Box3DKnotHolderEntityCenter *entity_center = new Box3DKnotHolderEntityCenter();
    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Box3D:center",
                          _("Move the box in perspective"));

    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPMarker */

// marker x scale = (marker width)/(view box width) 
double 
getMarkerXScale(SPItem* item){

    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    return ((sp_marker->viewBox.width() != 0) ? sp_marker->markerWidth.computed/sp_marker->viewBox.width() : 1.0);
}

double 
getMarkerYScale(SPItem* item){

    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    return ((sp_marker->viewBox.height() != 0) ? sp_marker->markerHeight.computed/sp_marker->viewBox.height() : 1.0);
}

/* 
- edit_rotation is the tangent angle that is used in orient auto mode.
- edit_rotation is applied in the edit_transform, it needs to be undone and then the orient.computed can be applied. 
*/
Geom::Affine 
getMarkerRotation(SPItem* item, double edit_rotation, int edit_marker_mode, bool reverse = false){

    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    Geom::Affine rot = Geom::Rotate::from_degrees(0.0);

    if ((sp_marker->orient_mode == MARKER_ORIENT_AUTO_START_REVERSE) && (edit_marker_mode == SP_MARKER_LOC_START)) {
        rot = Geom::Rotate::from_degrees(180.0);
    } else if (sp_marker->orient_mode == MARKER_ORIENT_ANGLE) {
        rot = reverse? Geom::Rotate::from_degrees(edit_rotation - sp_marker->orient.computed) : Geom::Rotate::from_degrees(sp_marker->orient.computed - edit_rotation);
    }

    return rot;
}
 
// used to translate the knots when the marker's minimum bounds are less than zero. 
Geom::Rect 
getMarkerBounds(SPItem* item, SPDesktop *desktop){
    auto sp_marker = cast<SPMarker>(item);
    SPDocument *doc = desktop->getDocument();

    g_assert(sp_marker != nullptr);
    g_assert(doc != nullptr);

    std::vector<SPObject*> items = sp_marker->childList(false, SPObject::ActionBBox);
    Geom::OptRect r;

    for (auto *i : items) {
        auto item = cast<SPItem>(i);
        r.unionWith(item->desktopVisualBounds());
    }
    Geom::Rect bounds(r->min() * doc->dt2doc(), r->max() * doc->dt2doc());
    return bounds;
}

/* 
- this knot sets the refX/refY attributes of the marker
- this knot is actually shown in the center of the shape vs the actual
refX/refY position to make it more intuitive
*/

class MarkerKnotHolderEntityReference : public KnotHolderEntity {
public:
    double _edit_rotation = 0.0;
    int _edit_marker_mode = -1;

    MarkerKnotHolderEntityReference(double edit_rotation, int edit_marker_mode) 
    : _edit_rotation(edit_rotation),
    _edit_marker_mode(edit_marker_mode) 
    {
    }

    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};


Geom::Point
MarkerKnotHolderEntityReference::knot_get() const
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    // knot is actually shown at center of marker, not at its reference point
    return Geom::Point((-sp_marker->refX.computed + getMarkerBounds(item, desktop).min()[Geom::X] + sp_marker->viewBox.width()/2) * getMarkerXScale(item), 
    (-sp_marker->refY.computed + getMarkerBounds(item, desktop).min()[Geom::Y] + sp_marker->viewBox.height()/2) * getMarkerYScale(item))
    * getMarkerRotation(item, _edit_rotation, _edit_marker_mode);
}

void
MarkerKnotHolderEntityReference::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    Geom::Point s = -p;
    s = s * getMarkerRotation(item, _edit_rotation, _edit_marker_mode, true);
    sp_marker->refX = (s[Geom::X]/ getMarkerXScale(item)) + getMarkerBounds(item, desktop).min()[Geom::X] + sp_marker->viewBox.width()/2;
    sp_marker->refY = (s[Geom::Y]/ getMarkerYScale(item)) + getMarkerBounds(item, desktop).min()[Geom::Y] + sp_marker->viewBox.height()/2;

    sp_marker->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// marker orient section - handles rotation

class MarkerKnotHolderEntityOrient : public KnotHolderEntity {
public:
    double _edit_rotation = 0.0;
    int _edit_marker_mode = -1;

    bool originals_set = false;

    // angle that the center of the marker makes with the orient knot
    double original_center_angle = 0;
    double original_radius = 0;
    Geom::Point original_center = Geom::Point(0, 0);
    
    MarkerKnotHolderEntityOrient(double edit_rotation, int edit_marker_mode) 
    : _edit_rotation(edit_rotation),
    _edit_marker_mode(edit_marker_mode) 
    {
    }

    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;

protected:
    void set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state);

};

void MarkerKnotHolderEntityOrient::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) {
    originals_set = false;
}

Geom::Point
MarkerKnotHolderEntityOrient::knot_get() const
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    return Geom::Point(
    (-sp_marker->refX.computed + sp_marker->viewBox.width() + getMarkerBounds(item, desktop).min()[Geom::X]) * getMarkerXScale(item), 
    (-sp_marker->refY.computed + getMarkerBounds(item, desktop).min()[Geom::Y]) * getMarkerYScale(item)) 
    * getMarkerRotation(item, _edit_rotation, _edit_marker_mode);
}

void
MarkerKnotHolderEntityOrient::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    if(!originals_set) {
        auto sp_marker = cast<SPMarker>(item);
        g_assert(sp_marker != nullptr);

        /* 
        - if the marker is set to auto or auto-start-reverse, set its type to orient
        - calculate and set the default angle for the orient mode
        */
        if (sp_marker->orient_mode != MARKER_ORIENT_ANGLE) {
            sp_marker->orient = (((sp_marker->orient_mode == MARKER_ORIENT_AUTO_START_REVERSE) && (_edit_marker_mode == SP_MARKER_LOC_START)) ? _edit_rotation + 180.0 : _edit_rotation);
            sp_marker->orient_mode = MARKER_ORIENT_ANGLE;
            sp_marker->orient_set = true;
        }

        /* 
        - the original marker center is used to calculate the angle with mouse
        - the refX/refY will be changing to adjust for the new rotation to give appearance that it is stationary onCanvas while editing. 
        */
        original_center = Geom::Point(
        (-sp_marker->refX.computed + getMarkerBounds(item, desktop).min()[Geom::X] + sp_marker->viewBox.width()/2) * getMarkerXScale(item), 
        (-sp_marker->refY.computed + getMarkerBounds(item, desktop).min()[Geom::Y] + sp_marker->viewBox.height()/2) * getMarkerYScale(item))
        * getMarkerRotation(item, _edit_rotation, _edit_marker_mode);

        original_center_angle = atan2(
        sp_marker->markerHeight.computed - sp_marker->markerHeight.computed/2, 
        sp_marker->markerWidth.computed - sp_marker->markerWidth.computed/2
        ) * 180.0/M_PI;
        
        original_radius = L2(original_center);
        originals_set = true;
    }

    set_internal(p, origin, state);
    update_knot();
}

void
MarkerKnotHolderEntityOrient::set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    // edit_rotation is the tangest angle to the shapes and needs to be taken into account while setting the orient angle
    double new_angle = atan2(p[Geom::Y] - original_center[Geom::Y], p[Geom::X] - original_center[Geom::X]) * 180.0/M_PI;
    new_angle = new_angle + _edit_rotation + original_center_angle;

    double axis_angle = -((atan2(original_center) * 180.0/M_PI) + _edit_rotation);

    sp_marker->orient = new_angle;
    sp_marker->orient_mode = MARKER_ORIENT_ANGLE;
    sp_marker->orient_set = true;

    Geom::Point ref = Geom::Point(
    (-(original_radius * cos(-(axis_angle + sp_marker->orient.computed) * M_PI/180.0))/getMarkerXScale(item)) + getMarkerBounds(item, desktop).min()[Geom::X] + sp_marker->viewBox.width()/2,
    (-(original_radius * sin(-(axis_angle + sp_marker->orient.computed) * M_PI/180.0))/getMarkerYScale(item)) + getMarkerBounds(item, desktop).min()[Geom::Y] + sp_marker->viewBox.height()/2);

    sp_marker->refX = ref[Geom::X];
    sp_marker->refY = ref[Geom::Y];

    sp_marker->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

// marker has multiple scaling knots at its corners

class MarkerKnotHolderEntityScale : public KnotHolderEntity {
public:
    double _edit_rotation = 0.0;
    int _edit_marker_mode = -1;

    /*
    - related to the position(+/-) of the scaling knot in reference to the center
    - makes sure scaling works correctly for derived classes
    */
    int _x_Sign = 1;
    int _y_Sign = 1;

    bool originals_set = false;

    double original_scaleX = 1;
    double original_scaleY = 1;

    double original_refX = 0;
    double original_refY = 0;

    double original_width = 0;
    double original_height = 0;

    MarkerKnotHolderEntityScale(double edit_rotation, int edit_marker_mode, int x_Sign, int y_Sign) 
    : _edit_rotation(edit_rotation),
    _edit_marker_mode(edit_marker_mode),
    _x_Sign(x_Sign),
    _y_Sign(y_Sign)
    {
    }

    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    Geom::Point knot_get() const override;

protected:
    void set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state);

private:
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

void 
MarkerKnotHolderEntityScale::knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) {
    originals_set = false;
}

Geom::Point 
MarkerKnotHolderEntityScale::knot_get() const
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    return Geom::Point(
    (-sp_marker->refX.computed + sp_marker->viewBox.width() + getMarkerBounds(item, desktop).min()[Geom::X]) * getMarkerXScale(item), 
    (-sp_marker->refY.computed + sp_marker->viewBox.height() + getMarkerBounds(item, desktop).min()[Geom::Y]) * getMarkerYScale(item)) 
    * getMarkerRotation(item, _edit_rotation, _edit_marker_mode);
}

void 
MarkerKnotHolderEntityScale::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    // keep track of the original values before the knot/mouse position is being moved
    if(!originals_set) {

        auto sp_marker = cast<SPMarker>(item);
        g_assert(sp_marker != nullptr);

        original_scaleX = getMarkerXScale(item);
        original_scaleY = getMarkerYScale(item);

        original_refX = sp_marker->refX.computed;
        original_refY = sp_marker->refY.computed;

        original_width = sp_marker->viewBox.width();
        original_height = sp_marker->viewBox.height();

        originals_set = true;
    }

    set_internal(p, origin, state);
    update_knot();
}

// scaling takes place around center of marker, not its reference point
void
MarkerKnotHolderEntityScale::set_internal(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    Geom::Point adjusted_origin = origin;
    Geom::Point adjusted_p = p; 

    if(sp_marker->orient_mode == MARKER_ORIENT_ANGLE) {

        adjusted_origin = adjusted_origin
        * Geom::Translate(getMarkerBounds(item, desktop).min()) 
        * Geom::Rotate::from_degrees(_edit_rotation - sp_marker->orient.computed); 

        adjusted_p = adjusted_p
        * Geom::Translate(getMarkerBounds(item, desktop).min()) 
        * Geom::Rotate::from_degrees(_edit_rotation - sp_marker->orient.computed); 

    } else if ((sp_marker->orient_mode == MARKER_ORIENT_AUTO_START_REVERSE) && (_edit_marker_mode == SP_MARKER_LOC_START)) {

        adjusted_origin = adjusted_origin
        * Geom::Translate(getMarkerBounds(item, desktop).min()) 
        * Geom::Rotate::from_degrees(180.0);

        adjusted_p = adjusted_p
        * Geom::Translate(getMarkerBounds(item, desktop).min()) 
        * Geom::Rotate::from_degrees(180.0);
    }

    // x_Sign and y_Sign are (+/- 1) to set the appropriate sign for derived classes
    double orig_width = _x_Sign*((original_width * original_scaleX)/2);
    double orig_height = _y_Sign*((original_height * original_scaleY)/2);

    // x & y displacement between origin and new mouse displacement
    double dx = adjusted_p[Geom::X] - adjusted_origin[Geom::X];
    double dy = adjusted_p[Geom::Y] - adjusted_origin[Geom::Y];
    double adjusted_scaleX = 0.0;
    double adjusted_scaleY = 0.0;

    adjusted_scaleX = (dx/orig_width) + 1;
    adjusted_scaleY = (dy/orig_height) + 1;

    // uniform scaling when ctrl+key is pressed
    if(mod_trans_confine->active(state)) {
        adjusted_scaleX = fabs(adjusted_scaleX);
        adjusted_scaleY = fabs(adjusted_scaleY);

        // possible areas based on which x/y coord is used to calculate uniform scale
        double dx_area = (sp_marker->viewBox.width()*adjusted_scaleX) * (sp_marker->viewBox.height()*adjusted_scaleX); // A = W*H
        double dy_area = (sp_marker->viewBox.width()*adjusted_scaleY) * (sp_marker->viewBox.height()*adjusted_scaleY);

        if (dy_area > dx_area) {
            adjusted_scaleX = adjusted_scaleY;
        } else if (dx_area > dy_area) {
            adjusted_scaleY = adjusted_scaleX;
        }

        adjusted_scaleX = adjusted_scaleX * original_scaleX;
        adjusted_scaleY = adjusted_scaleY * original_scaleY;

        sp_marker->markerWidth = sp_marker->viewBox.width() * adjusted_scaleX;
        sp_marker->markerHeight = sp_marker->viewBox.height() * adjusted_scaleY;

        sp_marker->refX = ((original_refX * original_scaleX)/adjusted_scaleX) - ((getMarkerBounds(item, desktop).min()[Geom::X] + sp_marker->viewBox.width()/2) * (original_scaleX/adjusted_scaleX  - 1));
        sp_marker->refY = ((original_refY * original_scaleY)/adjusted_scaleY) - ((getMarkerBounds(item, desktop).min()[Geom::Y] + sp_marker->viewBox.height()/2) * (original_scaleY/adjusted_scaleY  - 1));
    } else {

        adjusted_scaleX = adjusted_scaleX * original_scaleX;
        adjusted_scaleY = adjusted_scaleY * original_scaleY;

        // make sure the preserveAspectRatio is none when the user wants to use non-uniform scaling
        if (sp_marker->aspect_align != SP_ASPECT_NONE) {
            sp_marker->setAttribute("preserveAspectRatio", "none");
        }

        if(adjusted_scaleX > 0.0 && adjusted_scaleY > 0.0) {
            sp_marker->markerWidth = sp_marker->viewBox.width() * adjusted_scaleX;
            sp_marker->markerHeight = sp_marker->viewBox.height() * adjusted_scaleY;

            sp_marker->refX = ((original_refX * original_scaleX)/adjusted_scaleX) - ((getMarkerBounds(item, desktop).min()[Geom::X] + sp_marker->viewBox.width()/2) * (original_scaleX/adjusted_scaleX  - 1));
            sp_marker->refY = ((original_refY * original_scaleY)/adjusted_scaleY) - ((getMarkerBounds(item, desktop).min()[Geom::Y] + sp_marker->viewBox.height()/2) * (original_scaleY/adjusted_scaleY  - 1));
        }
    }

    sp_marker->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
}

class MarkerKnotHolderEntityScale2 : public MarkerKnotHolderEntityScale {
public:
    MarkerKnotHolderEntityScale2(double edit_rotation, int edit_marker_mode, int x_Sign, int y_Sign) 
    : MarkerKnotHolderEntityScale(edit_rotation, edit_marker_mode, x_Sign, y_Sign)
    {
    }

    Geom::Point knot_get() const override;
};

Geom::Point
MarkerKnotHolderEntityScale2::knot_get() const
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    // this corresponds to the reference point
    return Geom::Point((-sp_marker->refX.computed + getMarkerBounds(item, desktop).min()[Geom::X]) * getMarkerXScale(item), 
    (-sp_marker->refY.computed + getMarkerBounds(item, desktop).min()[Geom::Y]) * getMarkerYScale(item))
    * getMarkerRotation(item, _edit_rotation, _edit_marker_mode);
}


class MarkerKnotHolderEntityScale3 : public MarkerKnotHolderEntityScale {
public:
    MarkerKnotHolderEntityScale3(double edit_rotation, int edit_marker_mode, int x_Sign, int y_Sign) 
    : MarkerKnotHolderEntityScale(edit_rotation, edit_marker_mode, x_Sign, y_Sign)
    {
    }

    Geom::Point knot_get() const override;
};

Geom::Point
MarkerKnotHolderEntityScale3::knot_get() const
{
    auto sp_marker = cast<SPMarker>(item);
    g_assert(sp_marker != nullptr);

    return Geom::Point(
    (-sp_marker->refX.computed + getMarkerBounds(item, desktop).min()[Geom::X]) * getMarkerXScale(item), 
    (-sp_marker->refY.computed + sp_marker->viewBox.height() + getMarkerBounds(item, desktop).min()[Geom::Y]) * getMarkerYScale(item)) 
    * getMarkerRotation(item, _edit_rotation, _edit_marker_mode);
}

MarkerKnotHolder::MarkerKnotHolder(SPDesktop *desktop, SPItem *item, double edit_rotation, int edit_marker_mode)
    : KnotHolder(desktop, item)
{
    MarkerKnotHolderEntityReference *entity_reference = new MarkerKnotHolderEntityReference(edit_rotation, edit_marker_mode);
    MarkerKnotHolderEntityOrient *entity_orient = new MarkerKnotHolderEntityOrient(edit_rotation, edit_marker_mode);

    MarkerKnotHolderEntityScale *entity_scale = new MarkerKnotHolderEntityScale(edit_rotation, edit_marker_mode, 1, 1);
    // these two additional knots have the same scaling functionality but also serve as a fill in for the empty corners of the marker bounding box
    MarkerKnotHolderEntityScale2 *entity_scale2 = new MarkerKnotHolderEntityScale2(edit_rotation, edit_marker_mode, -1, -1);
    MarkerKnotHolderEntityScale3 *entity_scale3 = new MarkerKnotHolderEntityScale3(edit_rotation, edit_marker_mode, -1, 1);

    entity_reference->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Marker:reference",
                    _("Drag to adjust the refX/refY position of the marker"));

    entity_orient->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Marker:orient",
                    _("Adjust marker orientation through rotation"));

    entity_scale->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Marker:scale",
                    _("Adjust the <b>size</b> of the marker"));

    entity_scale2->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Marker:scale",
                    _("Adjust the <b>size</b> of the marker"));

    entity_scale3->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Marker:scale",
                    _("Adjust the <b>size</b> of the marker"));             

    entity.push_back(entity_reference);
    entity.push_back(entity_orient);
    entity.push_back(entity_scale);
    entity.push_back(entity_scale2);
    entity.push_back(entity_scale3);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPArc */

class ArcKnotHolderEntityExtremity : public KnotHolderEntity
{
public:
    ArcKnotHolderEntityExtremity(int is_start_knot) : is_start_knot(is_start_knot) {}
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

    std::string get_mod_moveinc_label() const { return mod_move_increment->get_label(); }
    std::string get_mod_maintaingap_label() const { return mod_maintain_gap->get_label(); }

private:
    double& get_holded_extremity() const;
    double& get_other_extremity() const;

    bool is_start_knot;
    Modifiers::Modifier *mod_move_increment = Modifiers::Modifier::get(Modifiers::Type::MOVE_INCREMENT);
    Modifiers::Modifier *mod_maintain_gap   = Modifiers::Modifier::get(Modifiers::Type::ARC_MAINTAIN_GAP);
};

class ArcKnotHolderEntityRX : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

    std::string get_mod_label() const { return mod_trans_confine->get_label(); }

private:
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

class ArcKnotHolderEntityRY : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

    std::string get_mod_label() const { return mod_trans_confine->get_label(); }

private:
    Modifiers::Modifier *mod_trans_confine = Modifiers::Modifier::get(Modifiers::Type::TRANS_CONFINE);
};

class ArcKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

/*
 * return values:
 *   1  : inside
 *   0  : on the curves
 *   -1 : outside
 */
static gint
sp_genericellipse_side(SPGenericEllipse *ellipse, Geom::Point const &p)
{
    gdouble dx = (p[Geom::X] - ellipse->cx.computed) / ellipse->rx.computed;
    gdouble dy = (p[Geom::Y] - ellipse->cy.computed) / ellipse->ry.computed;

    gdouble s = dx * dx + dy * dy;
    // We add a bit of a buffer, so there's a decent chance the user will
    // be able to adjust the arc without the closed status flipping between
    // open and closed during micro mouse movements.
    if (s < 0.75) return 1;
    if (s > 1.25) return -1;
    return 0;
}

void
ArcKnotHolderEntityExtremity::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    double snaps = Inkscape::Preferences::get()->getDoubleLimited("/options/rotationsnapsperpi/value", 12.0, 0.1, 1800.0);

    auto arc = cast<SPGenericEllipse>(item);
    g_assert(arc != nullptr);

    gint side = sp_genericellipse_side(arc, p);
    if(side != 0) { arc->setArcType( (side == -1) ?
                                     SP_GENERIC_ELLIPSE_ARC_TYPE_SLICE :
                                     SP_GENERIC_ELLIPSE_ARC_TYPE_ARC); }

    Geom::Point delta = p - Geom::Point(arc->cx.computed, arc->cy.computed);
    Geom::Scale sc(arc->rx.computed, arc->ry.computed);

    double extremity_initial_position = get_holded_extremity();
    double offset = extremity_initial_position - atan2(delta * sc.inverse());
    get_holded_extremity() -= offset;

    if (mod_move_increment->active(state) && snaps) {
        double snaps_radian = M_PI/snaps;
        get_holded_extremity() = std::round(get_holded_extremity()/snaps_radian) * snaps_radian;
        offset = extremity_initial_position - get_holded_extremity();
    }
    if (mod_maintain_gap->active(state)) {
        get_other_extremity() -= offset;
    }

    arc->normalize();
    arc->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

double&
ArcKnotHolderEntityExtremity::get_holded_extremity() const
{
    auto arc = cast<SPGenericEllipse>(item);
    g_assert(arc != nullptr);

    return is_start_knot ? arc->start : arc->end;
}

double&
ArcKnotHolderEntityExtremity::get_other_extremity() const
{
    auto arc = cast<SPGenericEllipse>(item);
    g_assert(arc != nullptr);

    return is_start_knot ? arc->end : arc->start;
}


Geom::Point
ArcKnotHolderEntityExtremity::knot_get() const
{
    SPGenericEllipse *ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    return ge->getPointAtAngle(get_holded_extremity());
}

void
ArcKnotHolderEntityExtremity::knot_click(unsigned int state)
{
    auto ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    if (state & GDK_SHIFT_MASK) {
        ge->end = ge->start = 0;
        ge->updateRepr();
    }
}

void
ArcKnotHolderEntityRX::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    ge->rx = fabs( ge->cx.computed - s[Geom::X] );

    if (mod_trans_confine->active(state)) {
        ge->ry = ge->rx.computed;
    }

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

Geom::Point
ArcKnotHolderEntityRX::knot_get() const
{
    SPGenericEllipse const *ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    return (Geom::Point(ge->cx.computed, ge->cy.computed) -  Geom::Point(ge->rx.computed, 0));
}

void
ArcKnotHolderEntityRX::knot_click(unsigned int state)
{
    auto ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    if (mod_trans_confine->active(state)) {
        ge->ry = ge->rx.computed;
        ge->updateRepr();
    }
}

void
ArcKnotHolderEntityRY::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    ge->ry = fabs( ge->cy.computed - s[Geom::Y] );

    if (mod_trans_confine->active(state)) {
        ge->rx = ge->ry.computed;
    }

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

Geom::Point
ArcKnotHolderEntityRY::knot_get() const
{
    SPGenericEllipse const *ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    return (Geom::Point(ge->cx.computed, ge->cy.computed) -  Geom::Point(0, ge->ry.computed));
}

void
ArcKnotHolderEntityRY::knot_click(unsigned int state)
{
    auto ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    if (mod_trans_confine->active(state)) {
        ge->rx = ge->ry.computed;
        ge->updateRepr();
    }
}

void
ArcKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    ge->cx = s[Geom::X];
    ge->cy = s[Geom::Y];

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

Geom::Point
ArcKnotHolderEntityCenter::knot_get() const
{
    SPGenericEllipse const *ge = cast<SPGenericEllipse>(item);
    g_assert(ge != nullptr);

    return Geom::Point(ge->cx.computed, ge->cy.computed);
}

ArcKnotHolder::ArcKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    ArcKnotHolderEntityRX *entity_rx = new ArcKnotHolderEntityRX();
    ArcKnotHolderEntityRY *entity_ry = new ArcKnotHolderEntityRY();
    ArcKnotHolderEntityExtremity *entity_start = new ArcKnotHolderEntityExtremity(true);
    ArcKnotHolderEntityExtremity *entity_end = new ArcKnotHolderEntityExtremity(false);
    ArcKnotHolderEntityCenter *entity_center = new ArcKnotHolderEntityCenter();

    entity_rx->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Arc:rx",
                      Glib::ustring::compose(_("Adjust ellipse <b>width</b>, with <b>%1</b> to make circle"),
                          entity_rx->get_mod_label())
                             .c_str());

    entity_ry->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Arc:ry",
                      Glib::ustring::compose(_("Adjust ellipse <b>height</b>, with <b>%1</b> to make circle"),
                          entity_ry->get_mod_label())
                             .c_str());

    entity_start->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Arc:start",
                         Glib::ustring::compose(_("Position the <b>start point</b> of the arc or segment; with <b>%1</b> to move "
                           "with <b>end point</b>; with <b>%2</b> to snap angle; drag <b>inside</b> the ellipse for arc, <b>outside</b> for segment"),
                           entity_start->get_mod_maintaingap_label(), entity_start->get_mod_moveinc_label())
                           .c_str());

    entity_end->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_ROTATE, "Arc:end",
                       Glib::ustring::compose(_("Position the <b>end point</b> of the arc or segment; with <b>%1</b> to move "
                         "with <b>start point</b>; with <b>%2</b> to snap angle; drag <b>inside</b> the ellipse for arc, <b>outside</b> for segment"),
                         entity_end->get_mod_maintaingap_label(), entity_end->get_mod_moveinc_label())
                         .c_str());

    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Arc:center",
                          _("Drag to move the ellipse"));

    entity.push_back(entity_rx);
    entity.push_back(entity_ry);
    entity.push_back(entity_start);
    entity.push_back(entity_end);
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPStar */
class StarKnotHolderEntityVertex : public KnotHolderEntity
{
public:
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

protected:
    SPStar* get_star();
    Geom::Point knot_get_internal(SPStarPoint starPoint) const;
    void knot_set_internal(Geom::Point const &p, unsigned int state);
    virtual double get_knot_arg() = 0;
    virtual void update_with_radial_ray_modifier(Geom::Point const &d) = 0;
    virtual void update_without_modifier(Geom::Point const &d, double arg1, double darg1) = 0;
};

class StarKnotHolderEntity1 : public StarKnotHolderEntityVertex {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;

protected:
    void update_with_radial_ray_modifier(Geom::Point const &d) override;
    void update_without_modifier(Geom::Point const &d, double arg1, double darg1) override;
    double get_knot_arg() override { return get_star()->arg[0]; }
};

class StarKnotHolderEntity2 : public StarKnotHolderEntityVertex {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;

protected:
    void update_with_radial_ray_modifier(Geom::Point const &d) override;
    void update_without_modifier(Geom::Point const &d, double arg1, double darg1) override;
    double get_knot_arg() override { return get_star()->arg[1]; }
};

class StarKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

void StarKnotHolderEntityVertex::knot_set_internal(Geom::Point const &p, unsigned int state)
{
    auto star = get_star();

    Geom::Point const s = snap_knot_position(p, state);

    Geom::Point d = s - star->center;

    double arg1 = atan2(d);
    double darg1 = arg1 - get_knot_arg();

    const double epsilon = 0.000001;
    double denominator = star->arg[0] - star->arg[1];
    denominator = fabs(denominator) < epsilon ? epsilon : denominator;
    if (state & GDK_ALT_MASK) {
        star->randomized = darg1/denominator;
    } else if (state & GDK_SHIFT_MASK) {
        star->rounded = darg1/denominator;
    } else if (state & GDK_CONTROL_MASK) {
        update_with_radial_ray_modifier(d);
    } else {
        update_without_modifier(d, arg1, darg1);
    }
    star->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

}

void
StarKnotHolderEntity1::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    knot_set_internal(p, state);
}

void
StarKnotHolderEntity1::update_with_radial_ray_modifier(Geom::Point const &d)
{
    get_star()->r[0] = L2(d);
}

void StarKnotHolderEntity1::update_without_modifier(Geom::Point const &d, double arg1, double darg1)
{
    auto star = get_star();

    star->r[0]    = L2(d);
    star->arg[0]  = arg1;
    star->arg[1] += darg1;
}

void
StarKnotHolderEntity2::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    if (!get_star()->flatsided) {
        knot_set_internal(p, state);
    }
}

void
StarKnotHolderEntity2::update_with_radial_ray_modifier(Geom::Point const &d)
{
    auto star = get_star();
    star->r[1]   = L2(d);
    star->arg[1] = star->arg[0] + M_PI / star->sides;
}

void StarKnotHolderEntity2::update_without_modifier(Geom::Point const &d, double arg1, double darg1)
{
    auto star = get_star();
    star->r[1]   = L2(d);
    star->arg[1] = atan2(d);
}

void
StarKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto star = cast<SPStar>(item);
    g_assert(star != nullptr);

    star->center = snap_knot_position(p, state);

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

SPStar*
StarKnotHolderEntityVertex::get_star()
{
    auto *star = cast<SPStar>(item);
    g_assert(star != nullptr);
    return star;
}

Geom::Point
StarKnotHolderEntityVertex::knot_get_internal(SPStarPoint starPoint) const
{
    g_assert(item != nullptr);

    SPStar const *star = cast<SPStar>(item);
    g_assert(star != nullptr);

    return sp_star_get_xy(star, starPoint, 0);
}

Geom::Point
StarKnotHolderEntity1::knot_get() const
{
    return knot_get_internal(SP_STAR_POINT_KNOT1);
}

Geom::Point
StarKnotHolderEntity2::knot_get() const
{
    return knot_get_internal(SP_STAR_POINT_KNOT2);
}

Geom::Point
StarKnotHolderEntityCenter::knot_get() const
{
    g_assert(item != nullptr);

    SPStar const *star = cast<SPStar>(item);
    g_assert(star != nullptr);

    return star->center;
}

void
StarKnotHolderEntityVertex::knot_click(unsigned int state)
{
    auto star = cast<SPStar>(item);
    g_assert(star != nullptr);

    if (state & GDK_ALT_MASK) {
        star->randomized = 0;
        star->updateRepr();
    } else if (state & GDK_SHIFT_MASK) {
        star->rounded = 0;
        star->updateRepr();
    } else if (state & GDK_CONTROL_MASK) {
        star->arg[1] = star->arg[0] + M_PI / star->sides;
        star->updateRepr();
    }
}

StarKnotHolder::StarKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    auto star = cast<SPStar>(item);
    g_assert(item != nullptr);

    StarKnotHolderEntity1 *entity1 = new StarKnotHolderEntity1();
    entity1->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Star:entity1",
                    _("Adjust the <b>tip radius</b> of the star or polygon; "
                      "with <b>Shift</b> to round; with <b>Alt</b> to randomize"));

    entity.push_back(entity1);

    if (star->flatsided == false) {
        StarKnotHolderEntity2 *entity2 = new StarKnotHolderEntity2();
        entity2->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Star:entity2",
                        _("Adjust the <b>base radius</b> of the star; with <b>Ctrl</b> to keep star rays "
                          "radial (no skew); with <b>Shift</b> to round; with <b>Alt</b> to randomize"));
        entity.push_back(entity2);
    }

    StarKnotHolderEntityCenter *entity_center = new StarKnotHolderEntityCenter();
    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Star:center",
                          _("Drag to move the star"));
    entity.push_back(entity_center);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPSpiral */

class SpiralKnotHolderEntityInner : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;
};

class SpiralKnotHolderEntityOuter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

class SpiralKnotHolderEntityCenter : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};


/*
 * set attributes via inner (t=t0) knot point:
 *   [default] increase/decrease inner point
 *   [shift]   increase/decrease inner and outer arg synchronizely
 *   [control] constrain inner arg to round per PI/4
 */
void
SpiralKnotHolderEntityInner::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    double snaps = Inkscape::Preferences::get()->getDoubleLimited("/options/rotationsnapsperpi/value", 12.0, 0.1, 1800.0);

    auto spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    gdouble   dx = p[Geom::X] - spiral->cx;
    gdouble   dy = p[Geom::Y] - spiral->cy;

    gdouble   moved_y = p[Geom::Y] - origin[Geom::Y];

    if (state & GDK_ALT_MASK) {
        // adjust divergence by vertical drag, relative to rad
        if (spiral->rad > 0) {
            double exp_delta = 0.1*moved_y/(spiral->rad); // arbitrary multiplier to slow it down
            spiral->exp += exp_delta;
            if (spiral->exp < 1e-3)
                spiral->exp = 1e-3;
        }
    } else {
        // roll/unroll from inside
        gdouble   arg_t0;
        spiral->getPolar(spiral->t0, nullptr, &arg_t0);

        gdouble   arg_tmp = atan2(dy, dx) - arg_t0;
        gdouble   arg_t0_new = arg_tmp - floor((arg_tmp+M_PI)/(2.0*M_PI))*2.0*M_PI + arg_t0;
        spiral->t0 = (arg_t0_new - spiral->arg) / (2.0*M_PI*spiral->revo);

        /* round inner arg per PI/snaps, if CTRL is pressed */
        if ( ( state & GDK_CONTROL_MASK )
             && ( fabs(spiral->revo) > SP_EPSILON_2 )
             && ( snaps != 0 ) ) {
            gdouble arg = 2.0*M_PI*spiral->revo*spiral->t0 + spiral->arg;
            double snaps_radian = M_PI/snaps;
            spiral->t0 = (std::round(arg/snaps_radian)*snaps_radian - spiral->arg)/(2.0*M_PI*spiral->revo);
        }

        spiral->t0 = CLAMP(spiral->t0, 0.0, 0.999);
    }

    spiral->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/*
 * set attributes via outer (t=1) knot point:
 *   [default] increase/decrease revolution factor
 *   [control] constrain inner arg to round per PI/4
 */
void SpiralKnotHolderEntityOuter::knot_set(Geom::Point const &p, Geom::Point const & /*origin*/, unsigned int state)
{
    auto const snaps = Inkscape::Preferences::get()->getDoubleLimited("/options/rotationsnapsperpi/value", 12.0, 0.1, 1800.0);
    auto const spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    gdouble  dx = p[Geom::X] - spiral->cx;
    gdouble  dy = p[Geom::Y] - spiral->cy;

    if (state & GDK_SHIFT_MASK) { // rotate without roll/unroll
        spiral->arg = atan2(dy, dx) - 2.0*M_PI*spiral->revo;
        if (!(state & GDK_ALT_MASK)) {
            // if alt not pressed, change also rad; otherwise it is locked
            spiral->rad = MAX(hypot(dx, dy), 0.001);
        }
        if ( ( state & GDK_CONTROL_MASK ) && snaps ) {
            double snaps_radian = M_PI/snaps;
            spiral->arg = std::round(spiral->arg/snaps_radian) * snaps_radian;
        }
    } else { // roll/unroll
        // arg of the spiral outer end
        double arg_1;
        spiral->getPolar(1, nullptr, &arg_1);

        // its fractional part after the whole turns are subtracted
        static double _2PI = 2.0 * M_PI;
        double arg_r = arg_1 - std::round(arg_1/_2PI) * _2PI;

        // arg of the mouse point relative to spiral center
        double mouse_angle = atan2(dy, dx);
        if (mouse_angle < 0)
            mouse_angle += _2PI;

        // snap if ctrl
        if ( ( state & GDK_CONTROL_MASK ) && snaps ) {
            double snaps_radian = M_PI/snaps;
            mouse_angle = std::round(mouse_angle/snaps_radian) * snaps_radian;
        }

        // by how much we want to rotate the outer point
        double diff = mouse_angle - arg_r;
        if (diff > M_PI)
            diff -= _2PI;
        else if (diff < -M_PI)
            diff += _2PI;

        // calculate the new rad;
        // the value of t corresponding to the angle arg_1 + diff:
        double t_temp = ((arg_1 + diff) - spiral->arg)/(_2PI*spiral->revo);
        // the rad at that t:
        double rad_new = 0;
        if (t_temp > spiral->t0)
            spiral->getPolar(t_temp, &rad_new, nullptr);

        // change the revo (converting diff from radians to the number of turns)
        spiral->revo += diff/(2*M_PI);
        if (spiral->revo < 1e-3)
            spiral->revo = 1e-3;

        // if alt not pressed and the values are sane, change the rad
        if (!(state & GDK_ALT_MASK) && rad_new > 1e-3 && rad_new/spiral->rad < 2) {
            // adjust t0 too so that the inner point stays unmoved
            double r0;
            spiral->getPolar(spiral->t0, &r0, nullptr);
            spiral->rad = rad_new;
            spiral->t0 = pow(r0 / spiral->rad, 1.0/spiral->exp);
        }
        if (!std::isfinite(spiral->t0)) spiral->t0 = 0.0;
        spiral->t0 = CLAMP(spiral->t0, 0.0, 0.999);
    }

    spiral->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void
SpiralKnotHolderEntityCenter::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    spiral->cx = s[Geom::X];
    spiral->cy = s[Geom::Y];

    spiral->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

Geom::Point
SpiralKnotHolderEntityInner::knot_get() const
{
    SPSpiral const *spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    return spiral->getXY(spiral->t0);
}

Geom::Point
SpiralKnotHolderEntityOuter::knot_get() const
{
    SPSpiral const *spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    return spiral->getXY(1.0);
}

Geom::Point
SpiralKnotHolderEntityCenter::knot_get() const
{
    SPSpiral const *spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    return Geom::Point(spiral->cx, spiral->cy);
}

void
SpiralKnotHolderEntityInner::knot_click(unsigned int state)
{
    auto spiral = cast<SPSpiral>(item);
    g_assert(spiral != nullptr);

    if (state & GDK_ALT_MASK) {
        spiral->exp = 1;
        spiral->updateRepr();
    } else if (state & GDK_SHIFT_MASK) {
        spiral->t0 = 0;
        spiral->updateRepr();
    }
}

SpiralKnotHolder::SpiralKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    SpiralKnotHolderEntityCenter *entity_center = new SpiralKnotHolderEntityCenter();
    SpiralKnotHolderEntityInner *entity_inner = new SpiralKnotHolderEntityInner();
    SpiralKnotHolderEntityOuter *entity_outer = new SpiralKnotHolderEntityOuter();

    // NOTE: entity_central and entity_inner can overlap.
    //
    // In that case it would be a problem if the center control point was ON
    // TOP because it would steal the mouse focus and the user would loose the
    // ability to access the inner control point using only the mouse.
    //
    // However if the inner control point is ON TOP, taking focus, the
    // situation is a lot better: the user can still move the inner control
    // point with the mouse to regain access to the center control point.
    //
    // So, create entity_inner AFTER entity_center; this ensures that
    // entity_inner gets rendered ON TOP.
    entity_center->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, "Spiral:center",
                          _("Drag to move the spiral"));

    entity_inner->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Spiral:inner",
                         _("Roll/unroll the spiral from <b>inside</b>; with <b>Ctrl</b> to snap angle; "
                           "with <b>Alt</b> to converge/diverge"));

    entity_outer->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Spiral:outer",
                         _("Roll/unroll the spiral from <b>outside</b>; with <b>Ctrl</b> to snap angle; "
                           "with <b>Shift</b> to scale/rotate; with <b>Alt</b> to lock radius"));

    entity.push_back(entity_center);
    entity.push_back(entity_inner);
    entity.push_back(entity_outer);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPOffset */

class OffsetKnotHolderEntity : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

void
OffsetKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto offset = cast<SPOffset>(item);
    g_assert(offset != nullptr);

    Geom::Point const p_snapped = snap_knot_position(p, state);

    offset->rad = sp_offset_distance_to_original(offset, p_snapped);
    offset->knot = p_snapped;
    offset->knotSet = true;

    offset->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}


Geom::Point
OffsetKnotHolderEntity::knot_get() const
{
    SPOffset const *offset = cast<SPOffset>(item);
    g_assert(offset != nullptr);

    Geom::Point np;
    sp_offset_top_point(offset,&np);
    return np;
}

OffsetKnotHolder::OffsetKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    OffsetKnotHolderEntity *entity_offset = new OffsetKnotHolderEntity();
    entity_offset->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Offset:entity",
                          _("Adjust the <b>offset distance</b>"));
    entity.push_back(entity_offset);

    add_pattern_knotholder();
    add_hatch_knotholder();
}

/* SPText */
class TextKnotHolderEntityInlineSize : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_click(unsigned int state) override;

private:
    Modifiers::Modifier *mod_clear_inline_size = Modifiers::Modifier::get(Modifiers::Type::TEXT_CLEAR_INLINE_SIZE);
};

Geom::Point
TextKnotHolderEntityInlineSize::knot_get() const
{
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);

    SPStyle* style = text->style;
    double inline_size = style->inline_size.computed;
    unsigned mode      = style->writing_mode.computed;
    unsigned anchor    = style->text_anchor.computed;
    unsigned direction = style->direction.computed;

    Geom::Point p(text->attributes.firstXY());

    if (text->has_inline_size()) {
        // SVG 2 'inline-size'

        // Keep handle at end of text line.
        if (mode == SP_CSS_WRITING_MODE_LR_TB ||
            mode == SP_CSS_WRITING_MODE_RL_TB) {
            // horizontal
            if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_START  ) ||
                 (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_END) ) {
                p *= Geom::Translate (inline_size, 0);
            } else if ( direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                p *= Geom::Translate (inline_size/2.0, 0 );
            } else if ( direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                p *= Geom::Translate (-inline_size/2.0, 0 );
            } else if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_END  ) ||
                        (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_START) ) {
                p *= Geom::Translate (-inline_size, 0);
            }
        } else {
            // vertical
            if (anchor == SP_CSS_TEXT_ANCHOR_START) {
                p *= Geom::Translate (0, inline_size);
            } else if (anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                p *= Geom::Translate (0, inline_size/2.0);
            } else if (anchor == SP_CSS_TEXT_ANCHOR_END) {
                p *= Geom::Translate (0, -inline_size);
            }
        }
    } else {
        // Normal single line text.
        if (Geom::OptRect bbox = text->geometricBounds()) { // Check if this is best.
            if (mode == SP_CSS_WRITING_MODE_LR_TB ||
                mode == SP_CSS_WRITING_MODE_RL_TB) {
                // horizontal
                if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_START  ) ||
                     (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_END) ) {
                    p *= Geom::Translate ((*bbox).width(), 0);
                } else if ( direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                    p *= Geom::Translate ((*bbox).width()/2, 0);
                } else if ( direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                    p *= Geom::Translate (-(*bbox).width()/2, 0);
                } else if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_END  ) ||
                            (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_START) ) {
                    p *= Geom::Translate (-(*bbox).width(), 0);
                }
            } else {
                // vertical
                if (anchor == SP_CSS_TEXT_ANCHOR_START) {
                    p *= Geom::Translate (0, (*bbox).height());
                } else if (anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
                    p *= Geom::Translate (0, (*bbox).height()/2);
                } else if (anchor == SP_CSS_TEXT_ANCHOR_END) {
                    p *= Geom::Translate (0, -(*bbox).height());
                }
                if (mode == SP_CSS_WRITING_MODE_TB_LR) {
                    p += Geom::Point((*bbox).width(), 0); // Keep on right side
                }
            }
        }
    }

    return p;
}

// Conversion from Inkscape SVG 1.1 to SVG 2 'inline-size'.
void
TextKnotHolderEntityInlineSize::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);

    SPStyle* style = text->style;
    unsigned mode      = style->writing_mode.computed;
    unsigned anchor    = style->text_anchor.computed;
    unsigned direction = style->direction.computed;

    Geom::Point const s = snap_knot_position(p, state);
    Geom::Point delta = s - text->attributes.firstXY();
    double size = 0.0;
    if (mode == SP_CSS_WRITING_MODE_LR_TB ||
        mode == SP_CSS_WRITING_MODE_RL_TB) {
        // horizontal

        size = delta[Geom::X];
        if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_START  ) ||
             (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_END) ) {
            // Do nothing
        } else if ( (direction == SP_CSS_DIRECTION_LTR && anchor == SP_CSS_TEXT_ANCHOR_END  ) ||
                    (direction == SP_CSS_DIRECTION_RTL && anchor == SP_CSS_TEXT_ANCHOR_START) ) {
            size = -size;
        } else if ( anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
            size = 2.0 * abs(size);
        } else {
            std::cerr << "TextKnotHolderEntityInlinSize: Should not be reached!" << std::endl;
        }

    } else {
        // vertical

        size = delta[Geom::Y];
        if (anchor == SP_CSS_TEXT_ANCHOR_START) {
            // Do nothing
        } else if (anchor == SP_CSS_TEXT_ANCHOR_END) {
            size = -size;
        } else if (anchor == SP_CSS_TEXT_ANCHOR_MIDDLE) {
            size = 2.0 * abs(size);
        }
    }

    // Size should never be negative
    if (size < 0.0) {
        size = 0.0;
    }

    // Set 'inline-size'.
    text->style->inline_size.setDouble(size);
    text->style->inline_size.set = true;

    // Ensure we respect new lines.
    text->style->white_space.read("pre");
    text->style->white_space.set = true;

    // Convert sodipodi:role="line" to '\n'.
    text->sodipodi_to_newline();

    text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    text->updateRepr();
}

// Conversion from SVG 2 'inline-size' to Inkscape's SVG 1.1.
void
TextKnotHolderEntityInlineSize::knot_click(unsigned int state)
{
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);

    if (mod_clear_inline_size->active(state)) {

        text->style->inline_size.clear();
        text->remove_svg11_fallback();   // Else 'x' and 'y' will be interpreted as absolute positions.
        text->newline_to_sodipodi();     // Convert '\n' to tspans with sodipodi:role="line".

        text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        text->updateRepr();
    }
}

/* SPTextPath */
class TextPathKnotHolderEntityOffset final : public KnotHolderEntity
{
public:
    enum class Placement {
        START,
        MIDDLE = 2
    };

    TextPathKnotHolderEntityOffset(SPDesktop *desktop, SPTextPath *textpath, Placement placement);

    Geom::Point knot_get() const override { return _position; }
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void knot_grabbed(Geom::Point const & /*grab_position*/, unsigned /*state*/) override;
    void knot_ungrabbed(Geom::Point const & /*p*/, Geom::Point const & /*origin*/, guint /*state*/) override;
    void knot_click(unsigned int state) override;
    void set_initial_position();

private:
    SPDesktop *_desktop = nullptr;
    SPTextPath *_textpath = nullptr;
    SPText *_text = nullptr;
    const Geom::PathVector  *_path_vector = nullptr;

    double _total_path_len = 1.0;
    double _offset_val = 0.0;
    double _text_len = 0.0;

    Geom::Point _initial_position;
    Geom::Point _position;
    Geom::Point _direction;
    Geom::Piecewise<Geom::D2<Geom::SBasis>> _pwd2;

    Placement _placement;

    TextPathSide _initial_side = SP_TEXT_PATH_SIDE_LEFT;

    sigc::scoped_connection _path_modified_connection;

    TextPathSide get_desired_textpath_side(Geom::Point p, Geom::Point origin, double t0) const;
    void change_textpath_side(TextPathSide desired_side, gint precision);
    double compute_knot_placement_offset() const;
    void adjust_offset_for_curve_bounds(double &offset) const;
    void get_initial_offset_value();
    void set_middle_knot_positional_offset(Geom::Point tangent);
};

TextPathKnotHolderEntityOffset::TextPathKnotHolderEntityOffset(SPDesktop *desktop, SPTextPath *textpath,
                                                               Placement const placement)
    : _desktop(desktop)
    , _textpath(textpath)
    , _placement(placement)
{
    if (auto const path_item = dynamic_cast<SPShape *>(sp_textpath_get_path_item(textpath))) {
        _path_modified_connection = path_item->connectModified([this](SPObject *, unsigned int) {
            for (auto const ent: parent_holder->entity) {
                if (auto const textpath_ent = dynamic_cast<TextPathKnotHolderEntityOffset *>(ent)) {
                    // Re-calculate the positions for each of the hidden knot
                    // before marking them visible.
                    textpath_ent->set_initial_position();
                    textpath_ent->update_knot();
                }
            }
        });
    }
}

// Conversion from Inkscape SVG 1.1 to SVG 2 'start-offset'.
void TextPathKnotHolderEntityOffset::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    g_assert(_text != nullptr);
    g_assert(_textpath != nullptr);
    g_assert(_path_vector != nullptr);

    // Find the projection of this point on the path.
    auto const t0 = Geom::nearest_time(p, _pwd2);
    gint const precision = Inkscape::Preferences::get()->getInt("/options/svgoutput/numericprecision");

    if (_placement == Placement::MIDDLE) {
        if (auto const desired_side = get_desired_textpath_side(p, origin, t0); _textpath->side != desired_side) {
            change_textpath_side(desired_side, precision);
        }

        return;
    }

    _position = _pwd2.valueAt(t0);

    // Now, find out the length of the portion from the starting
    // point to this point.
    auto const tmp_pwd2 = Geom::portion(_pwd2, 0, t0);
    auto const portion_len = Geom::length(tmp_pwd2);

    // Offset = % length of the path covered until this point /
    // total length of the path.
    _offset_val = (100.0 * portion_len) / _total_path_len;

    if (_textpath->side == SP_TEXT_PATH_SIDE_RIGHT) {
        // Calculate the offset from the end.
        _offset_val = (100.0 * (_total_path_len - portion_len)) / _total_path_len;
    }

    // Offset should never be smaller than -100 or greater than 100
    _offset_val = std::clamp(_offset_val, -100.0, 100.0);

    // Set 'startOffset'.
    std::stringstream offset_stream;
    offset_stream << std::fixed << std::setprecision(precision) << _offset_val << "%";
    auto const offset_str = offset_stream.str();
    _textpath->setStartOffset(offset_str.c_str());
    Inkscape::DocumentUndo::maybeDone(_desktop->getDocument(), "textpath:startOffset", RC_("Undo", "Modify textpath startOffset"), "");
    _text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    _text->updateRepr();
}

void TextPathKnotHolderEntityOffset::knot_grabbed(Geom::Point const & /*grab_position*/, unsigned /*state*/)
{
    for (auto const ent : parent_holder->entity) {
        // Hide all the knots except the knot being dragged.
        if (ent != this) {
            ent->knot->hide();
        }
    }
}

void TextPathKnotHolderEntityOffset::knot_ungrabbed(Geom::Point const & /*p*/, Geom::Point const & /*origin*/,
                                                         guint /*state*/)
{
    if (_placement == Placement::MIDDLE && (_initial_side != _textpath->side)) {
        _initial_side = _textpath->side;
        auto const icon = _textpath->side ? "text-path-right" : "text-path-left";
        Inkscape::DocumentUndo::done(_desktop->getDocument(), RC_("Undo", "Change textpath side"), INKSCAPE_ICON(icon));
    }

    for (auto const ent : parent_holder->entity) {
        if (ent != this) {
            if (auto const textpath_ent = dynamic_cast<TextPathKnotHolderEntityOffset *>(ent)) {
                // Re-calculate the positions for each of the hidden knots
                // before marking them visible.
                textpath_ent->set_initial_position();
                textpath_ent->update_knot();
                ent->knot->show();
            }
        }
    }
}

// Conversion from SVG 2 'inline-offset' to Inkscape's SVG 1.1.
void TextPathKnotHolderEntityOffset::knot_click(unsigned int state)
{
    auto const popover = Gtk::make_managed<Inkscape::UI::Widget::TextpathPopover>(_text, _textpath, _desktop, _offset_val);
    popover->signal_closed().connect([this] {
        for (auto const ent : parent_holder->entity) {
            if (auto const textpath_ent = dynamic_cast<TextPathKnotHolderEntityOffset *>(ent)) {
                textpath_ent->set_initial_position();
                textpath_ent->update_knot();
            }
        }
    });

    auto const canvas = _desktop->getCanvas();
    _desktop->getDesktopWidget()->get_canvas_grid()->setPopover(popover);

    Inkscape::UI::popup_at(*popover, *canvas, canvas->get_last_mouse());
}

TextPathSide TextPathKnotHolderEntityOffset::get_desired_textpath_side(Geom::Point p, Geom::Point const origin, double const t0) const
{
    Geom::Path const &current_path = _path_vector->pathAt(t0);

    if (current_path.closed()) {
        // Winding number based approach to check if the textPath side needs
        // changing. Typically, points with non-zero winding or odd winding
        // are considered inside the path.
        auto const winding_number = current_path.winding(p);
        return (winding_number % 2) ? SP_TEXT_PATH_SIDE_RIGHT : SP_TEXT_PATH_SIDE_LEFT;
    }

    // This is an open path, use intersections based approach to determine
    // if the side attribute needs changing
    Geom::Path temp_path;
    temp_path.start(origin);
    temp_path.appendNew<Geom::LineSegment>(p);

    // Switch the side if the drag line intersects the path odd number of times.
    // For no intersections, switch back to the initial side.
    auto const intersection_results = temp_path.intersect(current_path);
    bool const switch_sides = (intersection_results.size() % 2);
    return (_initial_side) ?
        (switch_sides ? SP_TEXT_PATH_SIDE_LEFT : SP_TEXT_PATH_SIDE_RIGHT) :
        (switch_sides ? SP_TEXT_PATH_SIDE_RIGHT : SP_TEXT_PATH_SIDE_LEFT);
}

void TextPathKnotHolderEntityOffset::change_textpath_side(TextPathSide const desired_side, gint const precision)
{
    _textpath->setSide(desired_side);

    // Changing sides will cause the text to move to a different position on the path
    // if the start offset is not adjusted accordingly.
    double const text_len_offset = _text_len * 100.0 / _total_path_len;
    _offset_val += (_text->resolve_flip_offset_multiplier() * text_len_offset);
    _offset_val = 100 - _offset_val;

    std::stringstream offset_stream;
    offset_stream << std::fixed << std::setprecision(precision) << _offset_val << '%';
    auto const offset_str = offset_stream.str();
    _textpath->setStartOffset(offset_str.c_str());

    // Move the middle handle at a certain distance normal to the direction of the path.
    auto const style = _text->style;
    auto const font_size = style->font_size.computed;
    _direction *= -1.0;
    _position = _initial_position + font_size * _direction;
    knot->angle = Geom::atan2(_direction);
    knot->updateCtrl();
    _text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    _text->updateRepr();
}

double TextPathKnotHolderEntityOffset::compute_knot_placement_offset() const
{
    double offset = 0;

    if (_placement == Placement::MIDDLE) {
        auto const multiplier = 0.5 + _text->resolve_alignment_offset_multiplier();
        offset += (multiplier * _text_len);
    }

    return offset;
}

void TextPathKnotHolderEntityOffset::adjust_offset_for_curve_bounds(double &offset) const
{
    if (offset >= _total_path_len) {
        if (is_closed(*_path_vector)) {
            offset = std::fmod(offset, _total_path_len);
        } else {
            // Set the offset to a value slightly smaller than the total
            // length of the path to ensure that the CurvilignToPosition
            // call returns a non-null value.
            offset = _total_path_len - 0.01;
        }
    }
}

void TextPathKnotHolderEntityOffset::get_initial_offset_value()
{
    if (_textpath->startOffset._set) {
        _offset_val = _textpath->startOffset.value;

        if (_textpath->startOffset.unit == SVGLength::PERCENT) {
            _offset_val *= 100.0;
        } else {
            // Convert the in length units to percent equivalent
            _offset_val *= (100.0 / _total_path_len);
        }

        _offset_val = std::clamp(_offset_val, -100.0, 100.0);
    } else {
        // startOffset value is not set.
        _offset_val = 0.0;
    }
}

void TextPathKnotHolderEntityOffset::set_middle_knot_positional_offset(Geom::Point const tangent)
{
    _initial_position = _position;
    _direction = -Geom::unit_vector(Geom::rot90(tangent));
    auto const angle = Geom::atan2(-tangent) + M_PI / 2;
    auto const style = _text->style;
    auto const font_size = style->font_size.computed / _text->i2dt_affine().descrim();
    _position += font_size * _direction;
    knot->angle = angle;
    knot->updateCtrl();
}

void TextPathKnotHolderEntityOffset::set_initial_position()
{
    // Calculate the position of the knot by querying the
    // startOffset until the knot is moved.
    _text = cast<SPText>(item);
    g_assert(_text != nullptr);

    if (!_textpath->originalPath) {
        if (auto const doc = _desktop->getDocument()) {
            doc->ensureUpToDate();
        }
    }

    if (auto const path_item = dynamic_cast<SPShape *>(sp_textpath_get_path_item(_textpath))) {
        if ((_path_vector = path_item->curve())) {
            _pwd2 = Geom::paths_to_pw(*_path_vector);
            _total_path_len = Geom::length(_pwd2);

            if (_total_path_len <= Geom::EPSILON) {
                // No need to display startOffset handles for extremely short paths
                this->knot->hide();
                return;
            }

            _initial_side = _textpath->side;

            // Set the position to the starting point of the path
            auto const last_path = _path_vector->back();
            _position = last_path.finalPoint();

            get_initial_offset_value();

            // Determine the point on the path at a certain distance
            // from the starting point.
            double offset = _offset_val * _total_path_len / 100.0;
            _text_len = _text->length();
            offset += compute_knot_placement_offset();
            adjust_offset_for_curve_bounds(offset);

            int unused = 0;
            Path::cut_position *point = _textpath->originalPath->CurvilignToPosition(1, &offset, unused);

            if (point && point[0].piece >= 0) {
                Geom::Point tangent;
                _textpath->originalPath->PointAndTangentAt(point[0].piece, point[0].t, _position, tangent);

                // Move the handle at a certain distance from the direction of the path.
                if (_placement == Placement::MIDDLE) {
                    set_middle_knot_positional_offset(tangent);
                }
            }
        }
    } else {
        g_warning("Couldn't find the path item of the curve, this should not happen");
        this->knot->hide();
    }
}

/**
 * Shape padding editor knot positioned top right corner of first object
 */
class TextKnotHolderEntityShapePadding : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
TextKnotHolderEntityShapePadding::knot_get() const
{
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);
    Geom::Point corner {Geom::infinity(), Geom::infinity()};

    if (!text->has_shape_inside()) {
        return corner;
    }

    auto shape = text->get_first_shape_dependency();
    if (!shape) {
        return corner;
    }

    Geom::OptRect bounds = shape->geometricBounds();
    if (bounds) {
        corner = (*bounds).corner(1);
        if (text->style->shape_padding.set) {
            auto padding = text->style->shape_padding.computed;
            corner *= Geom::Affine(Geom::Translate(-padding, padding));
        }
        corner *= shape->transform;
    }
    return corner;
}

void
TextKnotHolderEntityShapePadding::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    // Text in a shape: rectangle
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);
    if (!text->has_shape_inside()) {
        return;
    }

    if (auto shape = text->get_first_shape_dependency()) {
        if (Geom::OptRect optbounds = shape->geometricBounds()) {
            auto bounds = *optbounds;
            Geom::Point const point_a = snap_knot_position(p, state);
            Geom::Point point_b = point_a * shape->transform.inverse();

            double padding = 0.0;
            if (point_b[Geom::X] - 1 > bounds.midpoint()[Geom::X]) {
                padding = bounds.corner(1)[Geom::X] - point_b[Geom::X];
            }

            // Padding can only be a positive value according to the CSS/text-padding spec
            if (padding >= 0.0) {
                Inkscape::CSSOStringStream os;
                os << padding;
                text->style->shape_padding.read(os.str().c_str());

                text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                text->updateRepr();
            }
        }
    }
}


/**
 * Shape margin editor knot positioned top right corner of each object
 */
class TextKnotHolderEntityShapeMargin : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
    void set_shape(SPShape *shape) { linked_shape = shape; }
    SPShape *linked_shape;
};

Geom::Point
TextKnotHolderEntityShapeMargin::knot_get() const
{
    Geom::Point corner;
    if (linked_shape == nullptr) return corner;
    
    Geom::OptRect bounds = linked_shape->geometricBounds();
    if (bounds) {
        corner = (*bounds).corner(1);
        if (linked_shape->style->shape_margin.set) {
            auto margin = linked_shape->style->shape_margin.computed;
            corner *= Geom::Affine(Geom::Translate(margin, -margin));
        }
        corner *= linked_shape->transform;
    }
    return corner;
}

void
TextKnotHolderEntityShapeMargin::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    g_assert(linked_shape != nullptr);

    Geom::OptRect bounds = linked_shape->geometricBounds();
    if (bounds) {
        Geom::Point const point_a = snap_knot_position(p, state);
        Geom::Point point_b = point_a * linked_shape->transform.inverse();
        auto margin = -((*bounds).corner(1)[Geom::X] - point_b[Geom::X]);

        // Margins can only be `non-negative` according to the CSS/shape-margin spec
        if (margin >= 0.0) {
            Inkscape::CSSOStringStream os;
            os << margin;
            linked_shape->style->shape_margin.read(os.str().c_str());

            linked_shape->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            linked_shape->updateRepr();
        }
    }
}




class TextKnotHolderEntityShapeInside : public KnotHolderEntity {
public:
    Geom::Point knot_get() const override;
    void knot_ungrabbed(Geom::Point const &p, Geom::Point const &origin, guint state) override {};
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
TextKnotHolderEntityShapeInside::knot_get() const
{
    // SVG 2 'shape-inside'. We only get here if there is a rectangle shape.
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);

    Geom::Point p {Geom::infinity(), Geom::infinity()};
    if (text->has_shape_inside()) {
        Geom::OptRect frame = text->get_frame();
        if (frame) {
            p = (*frame).corner(2);
        } else {
            std::cerr << "TextKnotHolderEntityShapeInside::knot_get(): no frame!" << std::endl;
        }
    }
    return p;
}

void
TextKnotHolderEntityShapeInside::knot_set(Geom::Point const &p, Geom::Point const &/*origin*/, unsigned int state)
{
    // Text in a shape: rectangle
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);

    Geom::Point const s = snap_knot_position(p, state);

    Inkscape::XML::Node* rectangle = text->get_first_rectangle();
    if (!rectangle) {
        return;
    }
    double x = rectangle->getAttributeDouble("x", 0.0);;
    double y = rectangle->getAttributeDouble("y", 0.0);
    double width  = s[Geom::X] - x;
    double height = s[Geom::Y] - y;
    rectangle->setAttributeSvgDouble("width", width);
    rectangle->setAttributeSvgDouble("height", height);
    text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    text->updateRepr();
}

TextKnotHolder::TextKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    auto text = cast<SPText>(item);
    g_assert(text != nullptr);

    if (text->has_shape_inside()) {
        // 'shape-inside'

        if (text->get_first_rectangle()) {
            auto entity_shapeinside = new TextKnotHolderEntityShapeInside();
            entity_shapeinside->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Text:shapeinside",
                                       _("Adjust the <b>rectangular</b> region of the text."));
            entity.push_back(entity_shapeinside);
        }

        if (text->get_first_shape_dependency()) {
            auto entity_shapepadding = new TextKnotHolderEntityShapePadding();
            entity_shapepadding->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Text:shapepadding",
                                        _("Adjust the text <b>shape padding</b>."));
            entity.push_back(entity_shapepadding);
        }


        // Add knots for shape subtraction margins
        if (text->style->shape_subtract.set) {
            for (auto *href : text->style->shape_subtract.hrefs) {
                if (auto shape = href->getObject()) {
                    auto entity_shapemargin = new TextKnotHolderEntityShapeMargin();
                    entity_shapemargin->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SIZER, "Text:shapemargin",
                                                _("Adjust the shape's <b>text margin</b>."));
                    entity_shapemargin->set_shape(shape);
                    entity_shapemargin->update_knot();
                    entity.push_back(entity_shapemargin);
                }
            }
        }

    } else {
        // 'inline-size' or normal text
        TextKnotHolderEntityInlineSize *entity_inlinesize = new TextKnotHolderEntityInlineSize();

        entity_inlinesize->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "Text:inlinesize",
                                  _("Adjust the <b>inline size</b> (line length) of the text."));
        entity.push_back(entity_inlinesize);
    }

    add_pattern_knotholder();
    add_hatch_knotholder();
}

TextPathKnotHolder::TextPathKnotHolder(SPDesktop *desktop, SPText *text)
    : KnotHolder(desktop, text)
{
    g_assert(text != nullptr);

    // Find the first child that can be successfully cast to SPTextPath
    SPTextPath *textpath = nullptr;
    for (auto& child : text->childList(false)) {
        if (auto* const tp = dynamic_cast<SPTextPath*>(child)) {
            textpath = tp;
            break;
        }
    }

    auto tip = _("<b>Drag</b> to adjust the <b>start offset</b>. <b>Click</b> to open on-canvas textPath controls.");
    auto const entity_start_offset =
        new TextPathKnotHolderEntityOffset(desktop, textpath, TextPathKnotHolderEntityOffset::Placement::START);
    entity_start_offset->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_MOVE, "TextPath:startOffset", tip);
    entity_start_offset->set_initial_position();
    entity.push_back(entity_start_offset);

    tip = _("<b>Drag</b> to adjust the <b>textpath side</b>. <b>Click</b> to open on-canvas textPath controls.");
    auto const entity_side_knot =
        new TextPathKnotHolderEntityOffset(desktop, textpath, TextPathKnotHolderEntityOffset::Placement::MIDDLE);
    entity_side_knot->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_POINTER, "TextPath:startOffset", tip);
    entity_side_knot->set_initial_position();
    entity.push_back(entity_side_knot);
}

// TODO: this is derived from RectKnotHolderEntityWH because it used the same static function
// set_internal as the latter before KnotHolderEntity was C++ified. Check whether this also makes
// sense logically.
class FlowtextKnotHolderEntity : public RectKnotHolderEntityWH {
public:
    Geom::Point knot_get() const override;
    void knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state) override;
};

Geom::Point
FlowtextKnotHolderEntity::knot_get() const
{
    SPRect const *rect = cast<SPRect>(item);
    g_assert(rect != nullptr);

    return Geom::Point(rect->x.computed + rect->width.computed, rect->y.computed + rect->height.computed);
}

void
FlowtextKnotHolderEntity::knot_set(Geom::Point const &p, Geom::Point const &origin, unsigned int state)
{
    set_internal(p, origin, state);
}

FlowtextKnotHolder::FlowtextKnotHolder(SPDesktop *desktop, SPItem *item)
    : KnotHolder(desktop, item)
{
    g_assert(item != nullptr);

    FlowtextKnotHolderEntity *entity_flowtext = new FlowtextKnotHolderEntity();
    entity_flowtext->create(desktop, item, this, Inkscape::CANVAS_ITEM_CTRL_TYPE_SHAPER, "FlowText:entity",
                            _("Drag to resize the <b>flowed text frame</b>"));
    entity.push_back(entity_flowtext);
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
