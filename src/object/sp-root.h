// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SPRoot: SVG \<svg\> implementation.
 */
/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_ROOT_H_SEEN
#define SP_ROOT_H_SEEN

#include "display/drawing-item.h" // for DrawingItem
#include "sp-dimensions.h"        // for SPDimensions
#include "sp-item-group.h"        // for SPGroup
#include "util/cast.h"            // for tag_of
#include "version.h"              // for Version
#include "viewbox.h"              // for SPViewBox
#include "xml/node.h"             // for Node

class SPDefs;
class SPDocument;
enum class SPAttr;

/** \<svg\> element */
class SPRoot final : public SPGroup, public SPViewBox, public SPDimensions {
public:
	SPRoot();
	~SPRoot() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    char *onload;

    /**
     * Primary \<defs\> element where we put new defs (patterns, gradients etc.).
     *
     * At the time of writing, this is chosen as the first \<defs\> child of
     * this \<svg\> element: see writers of this member in sp-root.cpp.
     */
    SPDefs *defs;

	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void set(SPAttr key, char const* value) override;
	void update(SPCtx *ctx, unsigned int flags) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;

	void modified(unsigned int flags) override;
	void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
	void remove_child(Inkscape::XML::Node* child) override;

	Inkscape::DrawingItem* show(Inkscape::Drawing &drawing, unsigned int key, unsigned int flags) override;
	void print(SPPrintContext *ctx) override;
        const char* typeName() const override;
        const char* displayName() const override;

    void updateDocVersion();

    // Copy out the x/y coordinates for editing
    SVGLength root_x;
    SVGLength root_y;

    // Version information
    Inkscape::Version svg_version;
    Inkscape::Version inkscape_version;
private:
    void unset_x_and_y();
    void setRootDimensions();
};

#endif /* !SP_ROOT_H_SEEN */

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
