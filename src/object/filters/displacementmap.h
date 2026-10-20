// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * SVG displacement map filter effect
 *//*
 * Authors:
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_FEDISPLACEMENTMAP_H_SEEN
#define SP_FEDISPLACEMENTMAP_H_SEEN

#include "sp-filter-primitive.h"
#include "display/nr-filter-types.h"

enum FilterDisplacementMapChannelSelector
{
    DISPLACEMENTMAP_CHANNEL_RED,
    DISPLACEMENTMAP_CHANNEL_GREEN,
    DISPLACEMENTMAP_CHANNEL_BLUE,
    DISPLACEMENTMAP_CHANNEL_ALPHA,
    DISPLACEMENTMAP_CHANNEL_ENDTYPE
};

class SPFeDisplacementMap final
    : public SPFilterPrimitive
{
public:
    int tag() const override { return tag_of<decltype(*this)>; }

    int get_in2() const { return in2_slot; }

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned flags) override;

    void resolve_slots(SlotResolver &) override;
    std::unique_ptr<Inkscape::Filters::FilterPrimitive> build_renderer(Inkscape::DrawingItem *item) const override;

private:
    double scale = 0.0;
    FilterDisplacementMapChannelSelector xChannelSelector = DISPLACEMENTMAP_CHANNEL_ALPHA;
    FilterDisplacementMapChannelSelector yChannelSelector = DISPLACEMENTMAP_CHANNEL_ALPHA;

    std::optional<std::string> in2_name;
    int in2_slot = Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
};

#endif // SP_FEDISPLACEMENTMAP_H_SEEN

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
