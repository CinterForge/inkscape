// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG <filter> implementation, see sp-filter.cpp.
 */
#ifndef SP_FEPOINTLIGHT_H_SEEN
#define SP_FEPOINTLIGHT_H_SEEN

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

class SPFePointLight final
    : public SPObject
{
public:
	SPFePointLight();
	~SPFePointLight() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    /// x coordinate of the light source
    float x; 
    bool x_set : 1;
    /// y coordinate of the light source
    float y; 
    bool y_set : 1;
    /// z coordinate of the light source
    float z; 
    bool z_set : 1;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
	void release() override;
    void set(SPAttr key, char const *value) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;
};

#endif // SP_FEPOINTLIGHT_H_SEEN

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
