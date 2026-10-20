// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_MESHROW_H
#define SEEN_SP_MESHROW_H

/** \file
 * SPMeshrow: SVG <meshrow> implementation.
 */
/*
 * Authors: Tavmjong Bah
 * Copyright (C) 2012 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-object.h"

/** Gradient Meshrow. */
class SPMeshrow final : public SPObject {
public:
    SPMeshrow();
    ~SPMeshrow() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    SPMeshrow* getNextMeshrow();
    SPMeshrow* getPrevMeshrow();

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void set(SPAttr key, const char* value) override;
    void modified(unsigned int flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

#endif /* !SEEN_SP_MESHROW_H */

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
