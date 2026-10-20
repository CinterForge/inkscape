// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Base class for gradients and patterns
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2010 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-paint-server-reference.h"
#include "sp-paint-server.h"
#include "display/drawing-paintserver.h"

SPPaintServer *SPPaintServerReference::getObject() const
{
    return static_cast<SPPaintServer *>(URIReference::getObject());
}

bool SPPaintServerReference::_acceptObject(SPObject *obj) const
{
    return is<SPPaintServer>(obj) && URIReference::_acceptObject(obj);
}

SPPaintServer::SPPaintServer() = default;

SPPaintServer::~SPPaintServer() = default;

bool SPPaintServer::isSwatch() const
{
    return swatch;
}

bool SPPaintServer::isValid() const
{
    return true;
}

Inkscape::DrawingPattern *SPPaintServer::show(Inkscape::Drawing &/*drawing*/, unsigned /*key*/, Geom::OptRect const &/*bbox*/)
{
    return nullptr;
}

void SPPaintServer::hide(unsigned key)
{
}

void SPPaintServer::setBBox(unsigned key, Geom::OptRect const &bbox)
{
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
