// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "sp-title.h"

#include "xml/node.h"
#include "object/sp-object.h"  // for SPObject
#include "xml/node.h"          // for Node

namespace Inkscape::XML {
struct Document;
} // namespace Inkscape::XML

SPTitle::SPTitle() : SPObject() {
}

SPTitle::~SPTitle() = default;

Inkscape::XML::Node* SPTitle::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
	SPTitle* object = this;

    if (!repr) {
        repr = object->getRepr()->duplicate(xml_doc);
    }

    SPObject::write(xml_doc, repr, flags);

    return repr;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
