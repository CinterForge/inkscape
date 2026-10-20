// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_PAINT_SERVER_H
#define SEEN_SP_PAINT_SERVER_H

/*
 * Base class for gradients and patterns
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2010 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <memory>
#include <cairo.h>
#include <2geom/rect.h>
#include <sigc++/slot.h>
#include "sp-object.h"

#include "object/sp-paint-server-data.h"

namespace Inkscape {
class Drawing;
class DrawingPattern;
class DrawingPaintServer;
} // namespace Inkscape

using namespace Inkscape;

class SPPaintServer
    : public SPObject
{
public:
    SPPaintServer();
    ~SPPaintServer() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    bool isSwatch() const;
    virtual bool isValid() const;

    // Informs the renderer which of the rendering data functions will return valid data
    // Must always return because of how sp-object works this can't be = 0
    virtual PaintServerType getPaintType() const { return PaintServerType::INVALID; }

    // Provide the paint server rendering data in a neutral format that rendering
    // code can consume without needing to import sp-object (and limit unit testing)
    virtual Colors::Color    const  getSolidColor() const { return Colors::Color(0x0); }
    virtual SPGradientMesh   const *getGradientMesh() const { return nullptr; }
    virtual SPGradientVector const *getGradientVector() const { return nullptr; }
    virtual SPGradientSpread getSpread() const { return SP_GRADIENT_SPREAD_UNDEFINED; }
    virtual SPGradientUnits  getUnits() const { return SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX; }
    virtual Geom::Affine     getGradientTransform() const { return Geom::identity(); }

    virtual Inkscape::DrawingPattern *show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox);
    virtual void hide(unsigned key);
    virtual void setBBox(unsigned key, Geom::OptRect const &bbox);

protected:
    bool swatch = false;
};

#endif // SEEN_SP_PAINT_SERVER_H
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
