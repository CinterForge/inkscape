// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_POLYLINE_H
#define SEEN_SP_POLYLINE_H

#include "sp-shape.h"

class SPPolyLine final : public SPShape {
public:
	SPPolyLine();
	~SPPolyLine() override;
    int tag() const override { return tag_of<decltype(*this)>; }

	void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
	void set(SPAttr key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

        const char* typeName() const override;
	char* description() const override;
};

#endif // SEEN_SP_POLYLINE_H

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
