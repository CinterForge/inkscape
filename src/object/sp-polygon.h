// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_POLYGON_H
#define SEEN_SP_POLYGON_H

/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-shape.h"

class SPPolygon : public SPShape
{
public:
    SPPolygon();
    ~SPPolygon() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
    void set(SPAttr key, char const *value) override;
    char const *typeName() const override;
    char *description() const override;
};

// Functionality shared with SPPolyline
enum SPPolyParseError : uint8_t
{
    POLY_OK = 0,
    POLY_END_OF_STRING,
    POLY_INVALID_NUMBER,
    POLY_INFINITE_VALUE,
    POLY_NOT_A_NUMBER
};
SPPolyParseError sp_poly_get_value(char const **p, double *v);
std::optional<Geom::Path> sp_poly_parse_curve(char const *points);

#endif
