// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
/** \file
 * SPStop: SVG <stop> implementation.
 */
/*
 * Authors:
 */

#ifndef SEEN_SP_STOP_H
#define SEEN_SP_STOP_H

#include <glibmm/ustring.h>

#include "sp-object.h"

typedef unsigned int guint32;

namespace Inkscape::Colors {
    class Color;
}

/** Gradient stop. */
class SPStop final : public SPObject {
public:
    SPStop();
    ~SPStop() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    /// \todo fixme: Should be SPSVGPercentage
    float offset;

    Glib::ustring path_string;

    SPStop* getNextStop();
    SPStop* getPrevStop();

    Inkscape::Colors::Color getColor() const;
    void setColor(Inkscape::Colors::Color const &color);

    static void setColorRepr(Inkscape::XML::Node *node, Inkscape::Colors::Color const &color);

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void set(SPAttr key, const char* value) override;
    void modified(guint flags) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned int flags) override;
};

#endif /* !SEEN_SP_STOP_H */

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
