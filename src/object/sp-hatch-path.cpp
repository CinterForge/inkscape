// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG <hatchPath> implementation
 */
/*
 * Author:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2014 Tomasz Boczkowski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <2geom/path.h>

#include "style.h"
#include "svg/svg.h"
#include "path/path-curve.h"
#include "display/drawing.h"
#include "display/drawing-shape.h"
#include "helper/geom.h"
#include "attributes.h"
#include "sp-item.h"
#include "sp-hatch-path.h"
#include "svg/css-ostringstream.h"

SPHatchPath::SPHatchPath() = default;

SPHatchPath::~SPHatchPath() = default;

void SPHatchPath::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
    SPObject::build(doc, repr);

    readAttr(SPAttr::D);
    readAttr(SPAttr::OFFSET);
    readAttr(SPAttr::STYLE);

    style->fill.setNone();
}

void SPHatchPath::release()
{
    views.clear();
    SPObject::release();
}

void SPHatchPath::set(SPAttr key, gchar const *value)
{
    switch (key) {
    case SPAttr::D:
        if (value) {
            Geom::PathVector pv;
            _readHatchPathVector(value, pv, _continuous);
            _curve.emplace(std::move(pv));
        } else {
            _curve.reset();
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::OFFSET:
        offset.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    default:
        if (SP_ATTRIBUTE_IS_CSS(key)) {
            style->clear(key);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        } else {
            SPObject::set(key, value);
        }
        break;
    }
}

void SPHatchPath::update(SPCtx *ctx, unsigned int flags)
{
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        flags &= ~SP_OBJECT_USER_MODIFIED_FLAG_B;
    }

    if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        if (style->stroke_width.unit == SP_CSS_UNIT_PERCENT) {
            //TODO: Check specification

            SPItemCtx *ictx = static_cast<SPItemCtx *>(ctx);
            double const aw = (ictx) ? 1.0 / ictx->i2vp.descrim() : 1.0;
            style->stroke_width.computed = style->stroke_width.value * aw;

            for (auto &v : views) {
                v.drawingitem->setStyle(style);
            }
        }
    }

    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG)) {
        for (auto &v : views) {
            _updateView(v);
        }
    }
}

bool SPHatchPath::isValid() const
{
    return !_curve || _repeatLength() > 0;
}

Inkscape::DrawingItem *SPHatchPath::show(Inkscape::Drawing &drawing, unsigned int key, Geom::OptInterval extents)
{
    views.emplace_back(make_drawingitem<Inkscape::DrawingShape>(drawing), extents, key);
    auto &v = views.back();
    auto s = v.drawingitem.get();

    _updateView(v);

    return s;
}

void SPHatchPath::hide(unsigned int key)
{
    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });

    if (it != views.end()) {
        views.erase(it);
        return;
    }

    g_assert_not_reached();
}

void SPHatchPath::setStripExtents(unsigned int key, Geom::OptInterval const &extents)
{
    for (auto &v : views) {
        if (v.key == key) {
            v.extents = extents;
            break;
        }
    }
}

Geom::Interval SPHatchPath::bounds() const
{
    Geom::OptRect bbox;
    Geom::Interval result;

    Geom::Affine transform = Geom::Translate(offset.computed, 0);
    if (!_curve) {
        auto test_curve = Geom::LineSegment{Geom::Point(0, 0), Geom::Point(0, 1)};
        test_curve *= transform;
        bbox = test_curve.boundsExact();
    } else {
        bbox = bounds_exact_transformed(*_curve, transform);
    }

    double stroke_width = style->stroke_width.computed;
    result.setMin(bbox->left() - stroke_width / 2);
    result.setMax(bbox->right() + stroke_width / 2);
    return result;
}

Geom::PathVector SPHatchPath::calculateRenderCurve(unsigned key) const
{
    for (auto const &v : views) {
        if (v.key == key) {
            return _calculateRenderCurve(v);
        }
    }
    g_assert_not_reached();
    return {};
}

double SPHatchPath::_repeatLength() const
{
    double val = 0;

    if (_curve && !_curve->empty()) {
        val = _curve->finalPoint().y();
    }

    return val;
}

void SPHatchPath::_updateView(View &view)
{
    auto calculated_curve = _calculateRenderCurve(view);

    Geom::Affine offset_transform = Geom::Translate(offset.computed, 0);
    view.drawingitem->setTransform(offset_transform);
    style->fill.setNone();
    view.drawingitem->setStyle(style);
    view.drawingitem->setPath(std::make_shared<Geom::PathVector>(std::move(calculated_curve)));
}

Geom::PathVector SPHatchPath::_calculateRenderCurve(View const &view) const
{
    if (!view.extents) {
        return {};
    }

    Geom::PathVector calculated_curve;

    if (!_curve) {
        calculated_curve = path_from_curve(Geom::LineSegment{Geom::Point{0, view.extents->min()}, Geom::Point{0, view.extents->max()}});
        //TODO: if hatch has a dasharray defined, adjust line ends
    } else {
        double repeatLength = _repeatLength();
        if (repeatLength > 0) {
            gdouble initial_y = floor(view.extents->min() / repeatLength) * repeatLength;
            int segment_cnt = ceil((view.extents->extent()) / repeatLength) + 1;

            auto segment = *_curve;
            segment *= Geom::Translate(0, initial_y);

            Geom::Affine step_transform = Geom::Translate(0, repeatLength);
            for (int i = 0; i < segment_cnt; ++i) {
                if (_continuous) {
                    pathvector_append_continuous(calculated_curve, segment);
                } else {
                    pathvector_append(calculated_curve, segment);
                }
                segment *= step_transform;
            }
        }
    }

    return calculated_curve;
}

void SPHatchPath::_readHatchPathVector(char const *str, Geom::PathVector &pathv, bool &continous_join)
{
    if (!str) {
        return;
    }

    // Find first non-space character:
    const char* str_ptr = str;
    while (*str_ptr == ' ') {
        str_ptr++;
    }

    if (*str_ptr == 'M' ||
        *str_ptr == 'm') {
        // Hatch path is not continuous.
        pathv = sp_svg_read_pathv(str);
        if (pathv.empty()) {
            return;
        }
        continous_join = false;
    } else {
        Glib::ustring str2 = Glib::ustring::compose("M0,0 %1", str);
        pathv = sp_svg_read_pathv(str2.c_str());
        if (pathv.empty()) {
            return;
        }

        gdouble last_point_x = pathv.back().finalPoint().x();
        Inkscape::CSSOStringStream stream;
        stream << last_point_x;
        Glib::ustring str3 = Glib::ustring::compose("M%1,0 %2", stream.str(), str);
        Geom::PathVector pathv3 = sp_svg_read_pathv(str3.c_str());

        //Path can be composed of relative commands only. In this case final point
        //coordinates would depend on first point position. If this happens, fall
        //back to using 0,0 as first path point
        if (pathv3.back().finalPoint().y() == pathv.back().finalPoint().y()) {
            pathv = pathv3;
        }
        continous_join = true;
    }
}

SPHatchPath::View::View(DrawingItemPtr<Inkscape::DrawingShape> drawingitem, Geom::OptInterval const &extents, unsigned key)
    : drawingitem(std::move(drawingitem))
    , extents(extents)
    , key(key) {}

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
