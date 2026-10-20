// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A static view into the functions used to convert colors not using lcms2
 *
 * Copyright 2026 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

// Each convertable space should be imported here.
#include "cmyk.h"
#include "gray.h"
#include "hsl.h"
#include "hsluv.h"
#include "hsv.h"
#include "lch.h"
#include "luv.h"
#include "okhsl.h"
#include "okhsv.h"
#include "oklab.h"
#include "oklch.h"

namespace Inkscape::Colors::Space {

#define CASE(name, func) case Type::name:\
    return &func<T, PremultipliedAlpha>

template<typename T>
void blank(T const *i, T *o) {
    // noop
}

template<typename T, bool PremultipliedAlpha>
auto spaceToProfile(std::shared_ptr<Space::ProfileSpace<false>> const &space)
{
    switch (space->getType()) {
        CASE(CMYK,  DeviceCMYK::spaceToProfile);
        CASE(Gray,        Gray::spaceToProfile);
        CASE(HSL,          HSL::spaceToProfile);
        CASE(HSLUV,      HSLuv::spaceToProfile);
        CASE(HSV,          HSV::spaceToProfile);
        CASE(LCH,          Lch::spaceToProfile);
        CASE(LUV,          Luv::spaceToProfile);
        CASE(OKHSL,      OkHsl::spaceToProfile);
        CASE(OKHSV,      OkHsv::spaceToProfile);
        CASE(OKLAB,      OkLab::spaceToProfile);
        CASE(OKLCH,      OkLch::spaceToProfile);
        default:
            return &blank<T>;
    }
}

template<typename T, bool PremultipliedAlpha>
auto profileToSpace(std::shared_ptr<Space::ProfileSpace<false>> const &space)
{
    switch (space->getType()) {
        CASE(CMYK,  DeviceCMYK::profileToSpace);
        CASE(Gray,        Gray::profileToSpace);
        CASE(HSL,          HSL::profileToSpace);
        CASE(HSLUV,      HSLuv::profileToSpace);
        CASE(HSV,          HSV::profileToSpace);
        CASE(LCH,          Lch::profileToSpace);
        CASE(LUV,          Luv::profileToSpace);
        CASE(OKHSL,      OkHsl::profileToSpace);
        CASE(OKHSV,      OkHsv::profileToSpace);
        CASE(OKLAB,      OkLab::profileToSpace);
        CASE(OKLCH,      OkLch::profileToSpace);
        default:
            return &blank<T>;
    }
}

} // namespace Inkscape::Colors::Space

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
