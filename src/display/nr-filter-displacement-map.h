// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_NR_FILTER_DISPLACEMENT_MAP_H
#define SEEN_NR_FILTER_DISPLACEMENT_MAP_H

/*
 * feDisplacementMap filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "object/filters/displacementmap.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"

namespace Inkscape {
namespace Filters {

class FilterDisplacementMap : public FilterPrimitive
{
public:
    void render_cairo(FilterSlot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_input(int slot) override;
    void set_input(int input, int slot) override;
    void set_scale(double s);
    void set_channel_selector(int s, FilterDisplacementMapChannelSelector channel);

    Glib::ustring name() const override { return Glib::ustring("Displacement Map"); }

private:
    double scale;
    int _input2;
    unsigned Xchannel, Ychannel;
};

} // namespace Filters
} // namespace Inkscape

#endif // SEEN_NR_FILTER_DISPLACEMENT_MAP_H
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
