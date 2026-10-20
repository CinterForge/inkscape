// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * LPE <ruler> implementation, see lpe-ruler.cpp.
 */

/*
 * Authors:
 *   Maximilian Albert
 *
 * Copyright (C) Maximilian Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "live_effects/lpe-ruler.h"
#include <2geom/sbasis-to-bezier.h>
// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

static const Util::EnumData<MarkDirType> MarkDirData[] = {
    {MARKDIR_LEFT   , N_("Left"),  "left"},
    {MARKDIR_RIGHT  , N_("Right"), "right"},
    {MARKDIR_BOTH   , N_("Both"),  "both"},
};
static const Util::EnumDataConverter<MarkDirType> MarkDirTypeConverter(MarkDirData, sizeof(MarkDirData)/sizeof(*MarkDirData));

static const Util::EnumData<BorderMarkType> BorderMarkData[] = {
    {BORDERMARK_NONE    , NC_("Border mark", "None"),  "none"},
    {BORDERMARK_START   , N_("Start"), "start"},
    {BORDERMARK_END     , N_("End"),   "end"},
    {BORDERMARK_BOTH    , N_("Both"),  "both"},
};
static const Util::EnumDataConverter<BorderMarkType> BorderMarkTypeConverter(BorderMarkData, sizeof(BorderMarkData)/sizeof(*BorderMarkData));

LPERuler::LPERuler(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    mark_distance(_("_Mark distance:"), _("Distance between successive ruler marks"), "mark_distance", &wr, this, 20.0),
    unit(_("Unit:"), _("Unit"), "unit", &wr, this),
    mark_length(_("Ma_jor length:"), _("Length of major ruler marks"), "mark_length", &wr, this, 14.0),
    minor_mark_length(_("Mino_r length:"), _("Length of minor ruler marks"), "minor_mark_length", &wr, this, 7.0),
    minor_mark_gap(_("Minor mark _gap:"),
                   // xgettext:no-c-format
                   _("Space between path and minor ruler mark, % of mark length"),
                   "minor_mark_gap", &wr, this, 0.0),
    major_mark_gap(_("Major mar_k gap:"),
                   // xgettext:no-c-format
                   _("Space between path and major ruler mark, % of mark length"),
                   "major_mark_gap", &wr, this, 0.0),
    major_mark_steps(_("Major steps_:"), _("Draw a major mark every ... steps"), "major_mark_steps", &wr, this, 5),
    mark_angle(_("Mark angle:"), _("Rotate marks (-180° to 180°)"), "mark_angle", &wr, this, 0.0),
    shift(_("Shift marks _by:"), _("Shift marks by this many steps"), "shift", &wr, this, 0),
    mark_dir(_("Mark direction:"), _("Direction of marks (when viewing along the path from start to end)"), "mark_dir", MarkDirTypeConverter, &wr, this, MARKDIR_LEFT),
    offset(_("_Offset:"), _("Offset of first mark"), "offset", &wr, this, 0.0),
    border_marks(_("Border marks:"), _("Choose whether to draw marks at the beginning and end of the path"), "border_marks", BorderMarkTypeConverter, &wr, this, BORDERMARK_BOTH)
{
    registerParameter(&unit);
    registerParameter(&mark_distance);
    registerParameter(&mark_angle);
    registerParameter(&mark_length);
    registerParameter(&minor_mark_length);
    registerParameter(&minor_mark_gap);
    registerParameter(&major_mark_gap);
    registerParameter(&major_mark_steps);
    registerParameter(&shift);
    registerParameter(&offset);
    registerParameter(&mark_dir);
    registerParameter(&border_marks);

    mark_angle.param_make_integer();
    mark_angle.param_set_range(-180, 180);
    major_mark_steps.param_make_integer();
    major_mark_steps.param_set_range(1, 1000);
    shift.param_make_integer();
    mark_distance.param_set_range(0.01, std::numeric_limits<double>::max());
    mark_distance.param_set_digits(2);
    minor_mark_gap.param_make_integer();
    minor_mark_gap.param_set_range(0, 100);
    major_mark_gap.param_make_integer();
    major_mark_gap.param_set_range(0, 100);
    mark_length.param_set_increments(1.0, 10.0);
    minor_mark_length.param_set_increments(1.0, 10.0);
    offset.param_set_increments(1.0, 10.0);
}

LPERuler::~LPERuler() = default;

Geom::Point LPERuler::n_major;
Geom::Point LPERuler::n_minor;

Geom::Piecewise<Geom::D2<Geom::SBasis> >
LPERuler::ruler_mark(Geom::Point const &A, Geom::Point const &n, MarkType const &marktype)
{
    using namespace Geom;

    double real_mark_length = mark_length;
    SPDocument *document = getSPDoc();
    if (document) {
        if (legacy) {
            real_mark_length = Inkscape::Util::Quantity::convert(real_mark_length, unit.get_abbreviation(), document->getWidth().unit->abbr.c_str());
        } else {
            real_mark_length = Inkscape::Util::Quantity::convert(real_mark_length, unit.get_abbreviation(), "px") / document->getDocumentScale()[Geom::X];
        }
    }
    double real_minor_mark_length = minor_mark_length;
    if (document) {
        if (legacy) {
            real_minor_mark_length = Inkscape::Util::Quantity::convert(real_minor_mark_length, unit.get_abbreviation(), document->getWidth().unit->abbr.c_str());
        } else {
            real_minor_mark_length = Inkscape::Util::Quantity::convert(real_minor_mark_length, unit.get_abbreviation(), "px") / document->getDocumentScale()[Geom::X];
        }
    }
    n_major = real_mark_length * n;
    n_minor = real_minor_mark_length * n;
    if (mark_dir == MARKDIR_BOTH) {
        n_major = n_major * 0.5;
        n_minor = n_minor * 0.5;
    }

    Point C, D;
    double factor = 1.0;
    double mark_gap = 0;
    switch (marktype) {
        case MARK_MAJOR:
            mark_gap = major_mark_gap;
            C = A;
            D = A + n_major;
            if (real_mark_length && real_minor_mark_length && real_mark_length < real_minor_mark_length) {
                factor = real_mark_length/real_minor_mark_length;
            }
            if (mark_dir == MARKDIR_BOTH)
                C -= n_major;
            break;
        case MARK_MINOR:
            mark_gap = minor_mark_gap;
            if (real_mark_length && real_minor_mark_length && real_mark_length > real_minor_mark_length) {
                factor = real_minor_mark_length / real_mark_length;
            }
            C = A;
            D = A + n_minor;
            if (mark_dir == MARKDIR_BOTH)
                C -= n_minor;
            break;
        default:
            // do nothing
            break;
    }

    Piecewise<D2<SBasis> > seg(D2<SBasis>(SBasis(C[X], D[X]), SBasis(C[Y], D[Y])));
    if (mark_angle || mark_gap) {
        Geom::PathVector pvec = path_from_piecewise(seg,0.0001);
        if (mark_angle) {
            pvec *= Geom::Translate(A).inverse();
            pvec *= Geom::Rotate(Geom::rad_from_deg(mark_angle));
            pvec *= Geom::Translate(A);
            seg = paths_to_pw(pvec);  
        }
        if (mark_gap) {
            Geom::PathVector pv;
            if (mark_dir == MARKDIR_BOTH) {
                pv.push_back(pvec[0].portion(0, 0.5 - ((mark_gap * 0.5 * (1 + (1 - factor)))/100.0)));  
                pv.push_back(pvec[0].portion(0.5 + ((mark_gap * 0.5 * (1 + (1 - factor)))/100.0), 1));  
            } else {
                pv.push_back(pvec[0].portion((mark_gap * (1 + (1 - factor)))/100.0,1));  
            }
            seg = paths_to_pw(pv);  
        }
    }
    return seg;
}

void
LPERuler::doOnApply(SPLPEItem const* lpeitem)
{
    lpeversion.param_setValue("1.3.1", true);
    legacy = false;
}

Geom::Piecewise<Geom::D2<Geom::SBasis> >
LPERuler::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in)
{
    if (is_load) {
        legacy = lpeversion.param_getSVGValue() < "1.3.1";
    }
    using namespace Geom;

    const int mminterval = static_cast<int>(major_mark_steps);
    const int i_shift = static_cast<int>(shift) % mminterval;
    int sign = (mark_dir == MARKDIR_RIGHT ? 1 : -1 );

    Piecewise<D2<SBasis> >output(pwd2_in);
    Piecewise<D2<SBasis> >speed = derivative(pwd2_in);
    Piecewise<SBasis> arclength = arcLengthSb(pwd2_in);
    double totlength = arclength.lastValue();
    
    //find at which times to draw a mark:
    std::vector<double> s_cuts;
    SPDocument *document = getSPDoc();
    
    if (document) {
        bool write = false;
        if (legacy) {
            auto const punit = prev_unit;
            prev_unit = getSPDoc()->getDisplayUnit()->abbr;
            if (!punit.empty() && prev_unit != punit) {
                //_document->getDocumentScale().inverse()
                mark_distance.param_set_value(Inkscape::Util::Quantity::convert(mark_distance, prev_unit.c_str(), punit.c_str()));
                offset.param_set_value(Inkscape::Util::Quantity::convert(offset, prev_unit.c_str(), punit.c_str()));
                minor_mark_length.param_set_value(Inkscape::Util::Quantity::convert(minor_mark_length, prev_unit.c_str(), punit.c_str()));
                mark_length.param_set_value(Inkscape::Util::Quantity::convert(mark_length, prev_unit.c_str(), punit.c_str()));
                write = true;
            }
        } else {
            auto const punit = prev_unit;
            prev_unit = unit.get_abbreviation();
            if (!punit.empty() && prev_unit != punit) {
                mark_distance.param_set_value(Inkscape::Util::Quantity::convert(mark_distance, punit, unit.get_abbreviation()));
                offset.param_set_value(Inkscape::Util::Quantity::convert(offset, punit, unit.get_abbreviation()));
                minor_mark_length.param_set_value(Inkscape::Util::Quantity::convert(minor_mark_length, punit, unit.get_abbreviation()));
                mark_length.param_set_value(Inkscape::Util::Quantity::convert(mark_length, punit, unit.get_abbreviation()));
                write = true;
            }
        }
        if (write) {
            minor_mark_length.write_to_SVG();
            mark_length.write_to_SVG();
            mark_distance.write_to_SVG();
            offset.write_to_SVG();
        }
    }
    double real_mark_distance = mark_distance;
    if (document) {
        if (legacy) {
            real_mark_distance = Inkscape::Util::Quantity::convert(real_mark_distance, unit.get_abbreviation(), document->getWidth().unit->abbr.c_str());
        } else {
            real_mark_distance = Inkscape::Util::Quantity::convert(real_mark_distance, unit.get_abbreviation(), "px")  / document->getDocumentScale()[Geom::X];
        }
    }  
    double real_offset = offset;
    if (document) {
        if (legacy) {
            real_offset = Inkscape::Util::Quantity::convert(real_offset, unit.get_abbreviation(), document->getWidth().unit->abbr.c_str());
        } else {
            real_offset = Inkscape::Util::Quantity::convert(real_offset, unit.get_abbreviation(), "px")  / document->getDocumentScale()[Geom::X];
        }
    }
    for (double s = real_offset; s<totlength; s+=real_mark_distance){
        s_cuts.push_back(s);
    }
    std::vector<std::vector<double> > roots = multi_roots(arclength, s_cuts);
    std::vector<double> t_cuts;
    for (auto & root : roots){
        //FIXME: 2geom multi_roots solver seem to sometimes "repeat" solutions.
        //Here, we are supposed to have one and only one solution for each s.
        if(root.size()>0) 
            t_cuts.push_back(root[0]);
    }
    //draw the marks
    for (size_t i = 0; i < t_cuts.size(); i++) {
        Point A = pwd2_in(t_cuts[i]);
        Point n = rot90(unit_vector(speed(t_cuts[i])))*sign;
        if (static_cast<int>(i % mminterval) == i_shift) {
            output.concat (ruler_mark(A, n, MARK_MAJOR));
        } else {
            output.concat (ruler_mark(A, n, MARK_MINOR));
        }
    }
    //eventually draw a mark at start
    if ((border_marks == BORDERMARK_START || border_marks == BORDERMARK_BOTH) && (offset != 0.0 || i_shift != 0)){
        Point A = pwd2_in.firstValue();
        Point n = rot90(unit_vector(speed.firstValue()))*sign;
        output.concat (ruler_mark(A, n, MARK_MAJOR));
    }
    //eventually draw a mark at end
    if (border_marks == BORDERMARK_END || border_marks == BORDERMARK_BOTH){
        Point A = pwd2_in.lastValue();
        Point n = rot90(unit_vector(speed.lastValue()))*sign;
        //speed.lastValue() is sometimes wrong when the path is closed: a tiny line seg might added at the end to fix rounding errors...
        //TODO: Find a better fix!! (How do we know if the path was closed?)
        if ( A == pwd2_in.firstValue() &&
             speed.segs.size() > 1 &&
             speed.segs.back()[X].size() <= 1 &&
             speed.segs.back()[Y].size() <= 1 &&
             speed.segs.back()[X].tailError(0) <= 1e-10 &&
             speed.segs.back()[Y].tailError(0) <= 1e-10 
            ){
            n = rot90(unit_vector(speed.segs[speed.segs.size()-2].at1()))*sign;
        }
        output.concat (ruler_mark(A, n, MARK_MAJOR));
    }

    return output;
}

} //namespace LivePathEffect
} /* namespace Inkscape */

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
