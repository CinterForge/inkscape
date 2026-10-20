// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 * Some code and ideas migrated from dimensioning.py by
 * Johannes B. Rutzmoser, johannes.rutzmoser (at) googlemail (dot) com
 * https://github.com/Rutzmoser/inkscape_dimensioning
 *
 * Copyright (C) 2014 Author(s)
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_MEASURE_SEGMENTS_H
#define INKSCAPE_LPE_MEASURE_SEGMENTS_H

#include <vector>
#include <glibmm/ustring.h>

#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/colorpicker.h"
#include "live_effects/parameter/enum.h"
#include "live_effects/parameter/fontbutton.h"
#include "live_effects/parameter/message.h"
#include "live_effects/parameter/satellitearray.h"
#include "live_effects/parameter/text.h"
#include "live_effects/parameter/unit.h"

namespace Gtk {
class Widget;
} // namespace Gtk

namespace Inkscape::LivePathEffect {

enum OrientationMethod {
    OM_HORIZONTAL,
    OM_VERTICAL,
    OM_PARALLEL,
    OM_END
};

class LPEMeasureSegments final : public Effect {
public:
    LPEMeasureSegments(LivePathEffectObject *lpeobject);

    void doOnApply(SPLPEItem const* lpeitem) override;
    void doBeforeEffect (SPLPEItem const* lpeitem) override;
    void doOnRemove(SPLPEItem const* /*lpeitem*/) override;
    void doEffect(Geom::PathVector &curve) override {};
    void doOnVisibilityToggled(SPLPEItem const* /*lpeitem*/) override;
    bool doOnOpen(SPLPEItem const *lpeitem) override;
    void processObjects(LPEAction lpe_action) override;
    Gtk::Widget * newWidget() override;
    void createLine(Geom::Point start,Geom::Point end, Glib::ustring name, size_t counter, bool main, bool remove, bool arrows = false);
    void createTextLabel(Geom::Point &pos, size_t counter, double length, Geom::Coord angle, bool remove, bool valid);
    void createArrowMarker(Glib::ustring mode);
    bool isWhitelist(size_t i,  std::string listsegments, bool whitelist);
    void on_my_switch_page(Gtk::Widget* page, guint page_number);
private:
    UnitParam unit;
    EnumParam<OrientationMethod> orientation;
    ColorPickerParam coloropacity;
    FontButtonParam fontbutton;
    ScalarParam precision;
    ScalarParam fix_overlaps;
    ScalarParam position;
    ScalarParam text_top_bottom;
    ScalarParam helpline_distance;
    ScalarParam helpline_overlap;
    ScalarParam line_width;
    ScalarParam scale;
    TextParam format;
    TextParam blacklist;
    BoolParam scale_sensitive;
    BoolParam active_projection;
    BoolParam whitelist;
    BoolParam showindex;
    BoolParam arrows_outside;
    BoolParam flip_side;
    BoolParam local_locale;
    BoolParam rotate_anotation;
    BoolParam hide_back;
    BoolParam hide_arrows;
    BoolParam onbbox;
    BoolParam bboxonly;
    BoolParam centers;
    BoolParam maxmin;
    BoolParam smallx100;
    std::vector<Glib::ustring> items;
    SatelliteArrayParam linked_items;
    ScalarParam distance_projection;
    ScalarParam angle_projection;
    BoolParam avoid_overlapping;
    MessageParam helpdata;
    Colors::Color color;
    bool legacy = false;
    double fontsize;
    double prevfontsize = 0;
    double anotation_width;
    double previous_size;
    Glib::ustring prevunit = "";
    double arrow_gap;
    guint pagenumber;
    gchar const* locale_base;
    size_t prevsatellitecount = 0;
    bool prev_active_projection = false;
    SPObject *parent = nullptr;
    LPEMeasureSegments(const LPEMeasureSegments &) = delete;
    LPEMeasureSegments &operator=(const LPEMeasureSegments &) = delete;

};

} // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_LPE_MEASURE_SEGMENTS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
