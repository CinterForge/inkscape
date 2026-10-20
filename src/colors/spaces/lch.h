// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   2015 Alexei Boronine (original idea, JavaScript implementation)
 *   2015 Roger Tallada (Obj-C implementation)
 *   2017 Martin Mitas (C implementation, based on Obj-C implementation)
 *   2021 Massinissa Derriche (C++ implementation for Inkscape, based on C implementation)
 *   2023 Martin Owens (New Color classes)
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LCH_H
#define SEEN_COLORS_SPACES_LCH_H

#include <2geom/ray.h>

#include "lab.h"

namespace Inkscape::Colors::Space {

class Lch : public LabBase<Lch>
{
public:
    constexpr static int ProfileChannels = 3;
    constexpr static int OutputChannels = 3;

    constexpr static double LUMA_SCALE = 100;
    constexpr static double CHROMA_SCALE = 150;
    constexpr static double HUE_SCALE = 360;

    Lch(): LabBase(Type::LCH, "Lch", "Lch", "color-selector-lch", true) {
        _svgNames.emplace_back("lch");
    }
    ~Lch() override = default;

    template<typename T, bool PremultipliedAlpha = false>
    inline static void spaceToProfile(T const *i, T *o)
    {
        // Changes the values from 0..1, to typical lch scaling used in calculations.
        // L:0..100, C:0..150 H:0..360
        o[0] = SCALE_UP(i[0], 0, LUMA_SCALE);
        o[1] = SCALE_UP(i[1], 0, CHROMA_SCALE);
        o[2] = SCALE_UP(i[2], 0, HUE_SCALE);

        if constexpr (PremultipliedAlpha) {
            o[0] /= i[3];
            o[1] /= i[3];
            o[2] /= i[3];
            o[3] = i[3];
        }

        // Convert a color from the the LCH colorspace to the Lab colorspace.
        double sinhrad, coshrad;
        Geom::sincos(Geom::rad_from_deg(o[2]), sinhrad, coshrad);
        double a = coshrad * o[1];
        double b = sinhrad * o[1];

        o[1] = a;
        o[2] = b;

        // Changes the values from typical lab scaling (see above) to values 0..1.
        o[0] = SCALE_DOWN(o[0], 0, Lab::LUMA_SCALE);
        o[1] = SCALE_DOWN(o[1], Lab::MIN_SCALE, Lab::MAX_SCALE);
        o[2] = SCALE_DOWN(o[2], Lab::MIN_SCALE, Lab::MAX_SCALE);
    }
    template<typename T, bool PremultiplyAlpha = false>
    inline static void profileToSpace(T const *i, T *o)
    {
        // Changes the values from 0..1, to typical lab scaling used in calculations.
        o[0] = SCALE_UP(i[0], 0, Lab::LUMA_SCALE);
        o[1] = SCALE_UP(i[1], Lab::MIN_SCALE, Lab::MAX_SCALE);
        o[2] = SCALE_UP(i[2], Lab::MIN_SCALE, Lab::MAX_SCALE);

        // Convert a color from the the Lab colorspace to the LCH colorspace.
        double l = o[0];
        auto ab = Geom::Point(o[1], o[2]);
        double h;
        double const c = ab.length();

        /* Grays: disambiguate hue */
        if (c < 0.00000001) {
            h = 0;
        } else {
            h = Geom::deg_from_rad(Geom::atan2(ab));
            if (h < 0.0) {
                h += 360.0;
            }
        }

        if constexpr (PremultiplyAlpha) {
            o[0] = SCALE_DOWN(l, 0, LUMA_SCALE) * i[3];
            o[1] = SCALE_DOWN(c, 0, CHROMA_SCALE) * i[3];
            o[2] = SCALE_DOWN(h, 0, HUE_SCALE) * i[3];
            o[3] = i[3];
        } else {
            // Changes the values from typical lch scaling (see above)
            // to values 0..1 used in the color module.
            o[0] = SCALE_DOWN(l, 0, LUMA_SCALE);
            o[1] = SCALE_DOWN(c, 0, CHROMA_SCALE);
            o[2] = SCALE_DOWN(h, 0, HUE_SCALE);
        }
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("lch", Type::LCH)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LCH_H

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
