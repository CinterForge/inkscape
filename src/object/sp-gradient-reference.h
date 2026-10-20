// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_GRADIENT_REFERENCE_H
#define SEEN_SP_GRADIENT_REFERENCE_H

#include "uri-references.h"
#include "sp-gradient.h"

class SPGradientReference : public Inkscape::URIReference
{
public:
    SPGradientReference(SPGradient *grad) : URIReference(grad) {}

    SPGradient *getObject() const {
        return static_cast<SPGradient *>(URIReference::getObject());
    }

protected:
    bool _acceptObject(SPObject *obj) const override;
};

#endif /* !SEEN_SP_GRADIENT_REFERENCE_H */

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
