// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Stringstream internal linking
 *
 * Copyright (C) 2026 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "precision.h"

#ifndef INKSCAPE_UNIT_TEST
#  include "preferences.h"
#endif

namespace Inkscape::Util {

int get_default_numeric_precision()
{
// Make this function usable in a unit test environment.
#ifdef INKSCAPE_UNIT_TEST
    return 10;
#else
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    return prefs->getInt("/options/svgoutput/numericprecision", 10);
#endif
}

} // namespace Inkscape::Util

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
