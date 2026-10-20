// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_ANCHOR_H
#define SEEN_SP_ANCHOR_H

/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-item-group.h"
#include "uri-references.h"

class SPAnchor final : public SPGroup
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

    char *href = nullptr;
    char *type = nullptr;
    char *title = nullptr;
    SPDocument *page = nullptr;

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void set(SPAttr key, char const* value) override;
    virtual void updatePageAnchor();
    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned flags) override;
    void getLinked(std::vector<SPObject *> &objects, LinkedObjectNature direction) const override;

    const char* typeName() const override;
    const char* displayName() const override;
	char* description() const override;

    std::unique_ptr<Inkscape::URIReference> local_link;
};

#endif
