// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_SATELLITE_REFERENCE_H
#define SEEN_SP_SATELLITE_REFERENCE_H

#include "object/sp-item.h"
#include "object/uri-references.h"
/**
 * The reference corresponding to a satelite in a LivePathEffectObject.
 */
namespace Inkscape {
namespace LivePathEffect {

class SatelliteReference : public Inkscape::URIReference
{
public:
    SatelliteReference(SPObject *owner, bool hasactive = false)
        : URIReference(owner)
        , _hasactive(hasactive)
        , _active(true)
    {
    }

    bool getHasActive() const { return _hasactive; }
    bool getActive() const { return _active; }
    void setActive(bool active) { _active = active; }

protected:
    bool _acceptObject(SPObject *obj) const override;

private:
    bool _active;
    bool _hasactive;
};

} // namespace LivePathEffect
} // namespace Inkscape

#endif /* !SEEN_SP_SATELLITE_REFERENCE_H */

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