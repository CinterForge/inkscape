// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_TITLE_H
#define SEEN_SP_TITLE_H

/*
 * SVG <title> implementation
 *
 * Authors:
 *   Jeff Schiller <codedread@gmail.com>
 *
 * Copyright (C) 2008 Jeff Schiller
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

class SPTitle final : public SPObject {
public:
	SPTitle();
	~SPTitle() override;
    int tag() const override { return tag_of<decltype(*this)>; }

	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif
