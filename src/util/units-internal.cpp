// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape Units internal linking
 *
 * Copyright (C) 2026 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "util/units.h"
#include "io/resource.h"

namespace Inkscape::Util {

std::string UnitTable::getUnitsFilename()
{
    using namespace Inkscape::IO::Resource;
    return get_filename(UIS, "units.xml", false, true);
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
