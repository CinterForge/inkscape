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

#ifndef SEEN_SP_HATCH_PATH_H
#define SEEN_SP_HATCH_PATH_H

#include <vector>
#include <cstddef>
#include <optional>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>
#include <2geom/generic-interval.h>
#include <2geom/pathvector.h>

#include "svg/svg-length.h"
#include "object/sp-object.h"
#include "display/drawing-item-ptr.h"

namespace Inkscape {

class Drawing;
class DrawingShape;
class DrawingItem;

} // namespace Inkscape

class SPHatchPath final : public SPObject
{
public:
    SPHatchPath();
    ~SPHatchPath() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    SVGLength offset;

    bool isValid() const;

    Inkscape::DrawingItem *show(Inkscape::Drawing &drawing, unsigned int key, Geom::OptInterval extents);
    void hide(unsigned int key);

    void setStripExtents(unsigned int key, Geom::OptInterval const &extents);
    Geom::Interval bounds() const;

    Geom::PathVector calculateRenderCurve(unsigned key) const;

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void set(SPAttr key, const gchar* value) override;
    void update(SPCtx* ctx, unsigned int flags) override;

private:
    struct View
    {
        DrawingItemPtr<Inkscape::DrawingShape> drawingitem;
        Geom::OptInterval extents;
        unsigned key;
        View(DrawingItemPtr<Inkscape::DrawingShape> drawingitem, Geom::OptInterval const &extents, unsigned key);
    };
    std::vector<View> views;

    gdouble _repeatLength() const;
    void _updateView(View &view);
    Geom::PathVector _calculateRenderCurve(View const &view) const;

    void _readHatchPathVector(char const *str, Geom::PathVector &pathv, bool &continous_join);

    std::optional<Geom::PathVector> _curve;
    bool _continuous = false;
};

#endif // SEEN_SP_HATCH_PATH_H

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
