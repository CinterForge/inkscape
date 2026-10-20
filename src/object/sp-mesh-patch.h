// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MESHPATCH_H
#define SEEN_SP_MESHPATCH_H

/** \file
 * SPMeshpatch: SVG <meshpatch> implementation.
 */
/*
 * Authors: Tavmjong Bah
 *
 * Copyright (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/ustring.h>
#include "sp-object.h"

/** Gradient Meshpatch. */
class SPMeshpatch final : public SPObject {
public:
    SPMeshpatch();
    ~SPMeshpatch() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    SPMeshpatch* getNextMeshpatch();
    SPMeshpatch* getPrevMeshpatch();
    Glib::ustring * tensor_string;
    //SVGLength tx[4];  // Tensor points
    //SVGLength ty[4];  // Tensor points

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void set(SPAttr key, const char* value) override;
    void modified(unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

#endif /* !SEEN_SP_MESHPATCH_H */

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
