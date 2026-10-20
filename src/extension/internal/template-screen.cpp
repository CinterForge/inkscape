// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Contain internal sizes of paper which can be used in various
 * functions to make and size pages.
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "template-screen.h"

#include "clear-n_.h"
#include "extension/extension.h"
#include "extension/system.h"

namespace Inkscape::Extension::Internal {

void TemplateScreen::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">"
            "<id>org.inkscape.template.digital</id>"
            "<name>" N_("Screen Sizes") "</name>"
            "<description>" N_("Document formats using common screen resolutions") "</description>"
            "<category>" NC_("TemplateCategory", "Screen") "</category>"

            "<param name='unit' gui-text='" N_("Unit") "' type='string'>px</param>"
            "<param name='width' gui-text='" N_("Width") "' type='float' min='1.0' max='100000.0'>100.0</param>"
            "<param name='height' gui-text='" N_("Height") "' type='float' min='1.0' max='100000.0'>100.0</param>"

            "<template icon='desktop_hd_landscape' unit='px' priority='-20' visibility='all'>"

"<preset name='" N_("Desktop 1080p") "' label='1920 × 1080 px' height='1080' width='1920'/>"
"<preset name='" N_("Desktop 2K") "' label='2560 × 1440 px' height='1440' width='2560'/>"
"<preset name='" N_("Desktop 4K") "' label='3840 × 2160 px' height='2160' width='3840'/>"
"<preset name='" N_("Desktop 720p") "' label='1366 × 768 px' height='768' width='1366'/>"
"<preset name='" N_("Desktop SD") "' label='1024 × 768 px' height='768' width='1024' icon='desktop_landscape'/>"
"<preset name='" N_("iPhone 5") "' label='640 × 1136 px' height='1136' width='640' icon='mobile_portrait' visibility='icon,search'/>"
"<preset name='" N_("iPhone X") "' label='1125 × 2436 px' height='2436' width='1125' icon='mobile_portrait' visibility='icon,search'/>"
"<preset name='" N_("Mobile-smallest") "' label='360 × 640 px' height='640' width='360' icon='mobile_portrait' visibility='icon,search'/>"
"<preset name='" N_("iPad Pro") "' label='2388 × 1668 px' height='1668' width='2388' icon='tablet_landscape' visibility='icon,search'/>"
"<preset name='" N_("Tablet-smallest") "' label='1024 × 768 px' height='768' width='1024' icon='tablet_landscape' visibility='icon,search'/>"

            "</template>"
        "</inkscape-extension>",
        std::make_unique<TemplateScreen>());
    // clang-format on
}

} // namespace Inkscape::Extension::Internal

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
