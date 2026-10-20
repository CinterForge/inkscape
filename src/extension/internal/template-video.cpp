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

#include "template-video.h"

#include "clear-n_.h"
#include "extension/extension.h"
#include "extension/system.h"

namespace Inkscape::Extension::Internal {

void TemplateVideo::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">"
            "<id>org.inkscape.template.video</id>"
            "<name>" N_("Video Sizes") "</name>"
            "<description>" N_("Document formats using common video resolutions") "</description>"
            "<category>" NC_("TemplateCategory", "Video") "</category>"

            "<param name='unit' gui-text='" N_("Unit") "' type='string'>px</param>"
            "<param name='width' gui-text='" N_("Width") "' type='float' min='1.0' max='100000.0'>100.0</param>"
            "<param name='height' gui-text='" N_("Height") "' type='float' min='1.0' max='100000.0'>100.0</param>"

            "<template icon='video_landscape' unit='px' priority='-50' visibility='all'>"

"<preset name='" N_("Video SD PAL") "' label='768 × 576 px' width='768' height='576' />"
"<preset name='" N_("Video SD Widescreen / PAL") "' label='1024 × 576 px' width='1024' height='576' />"
"<preset name='" N_("Video SD NTSC") "' label='544 × 480 px' width='544' height='480' />"
"<preset name='" N_("Video SD Widescreen NTSC") "' label='872 × 486 px' width='872' height='486' />"
"<preset name='" N_("Video HD 720p") "' label='1280 × 720 px' width='1280' height='720' />"
"<preset name='" N_("Video HD 1080p") "' label='1920 × 1080 px' width='1920' height='1080' />"
"<preset name='" N_("Video DCI 2k (Full Frame)") "' label='2048 × 1080 px' width='2048' height='1080' />"
"<preset name='" N_("Video UHD 4k") "' label='3840 × 2160 px' width='3840' height='2160' />"
"<preset name='" N_("Video DCI 4k (Full Frame)") "' label='4096 × 2160 px' width='4096' height='2160' />"
"<preset name='" N_("Video UHD 8k") "' label='7680 × 4320 px' width='7680' height='4320' />"

            "</template>"
        "</inkscape-extension>",
        std::make_unique<TemplateVideo>());
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
