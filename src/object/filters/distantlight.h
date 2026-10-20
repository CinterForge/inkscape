// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SP_FEDISTANTLIGHT_H_SEEN
#define SP_FEDISTANTLIGHT_H_SEEN

/** \file
 * SVG <filter> implementation, see sp-filter.cpp.
 */
/*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *   Niko Kiirala <niko@kiirala.com>
 *   Jean-Rene Reinhard <jr@komite.net>
 *
 * Copyright (C) 2006,2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/sp-object.h"

class SPFeDistantLight final
    : public SPObject
{
public:
	SPFeDistantLight();
	~SPFeDistantLight() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    /// azimuth attribute
    float azimuth;
    bool azimuth_set : 1;
    /// elevation attribute
    float elevation;
    bool elevation_set : 1;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
	void release() override;
    void set(SPAttr key, char const *value) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;
};

#endif // SP_FEDISTANTLIGHT_H_SEEN

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
