// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MISSING_GLYPH_H
#define SEEN_SP_MISSING_GLYPH_H

/*
 * SVG <missing-glyph> element implementation
 *
 * Authors:
 *    Felipe C. da S. Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2008 Felipe C. da S. Sanches
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

class SPMissingGlyph final : public SPObject {
public:
	SPMissingGlyph();
	~SPMissingGlyph() override;
    int tag() const override { return tag_of<decltype(*this)>; }

	char* d;

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void release() override;
	void set(SPAttr key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;

private:
    double horiz_adv_x;
    double vert_origin_x;
    double vert_origin_y;
    double vert_adv_y;
};

#endif // SEEN_SP_MISSING_GLYPH_H
