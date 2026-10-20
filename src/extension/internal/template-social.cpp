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

#include "template-social.h"

#include "clear-n_.h"
#include "extension/extension.h"
#include "extension/system.h"

namespace Inkscape::Extension::Internal {

void TemplateSocial::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">"
            "<id>org.inkscape.template.social</id>"
            "<name>" N_("Social Sizes") "</name>"
            "<description>" N_("Document formats for social media") "</description>"
            "<category>" NC_("TemplateCategory", "Social") "</category>"

            "<param name='unit' gui-text='" N_("Unit") "' type='string'>px</param>"
            "<param name='width' gui-text='" N_("Width") "' type='float' min='1.0' max='100000.0'>100.0</param>"
            "<param name='height' gui-text='" N_("Height") "' type='float' min='1.0' max='100000.0'>100.0</param>"

            "<template icon='social_landscape' unit='px' priority='-30' visibility='icon,search'>"

"<preset name='" N_("Facebook cover photo") "' label='820 × 462 px' height='462' width='820'/>"
"<preset name='" N_("Facebook event image") "' label='1920 × 1080 px' height='1080' width='1920'/>"
"<preset name='" N_("Facebook image post") "' label='1200 × 630 px' height='630' width='1200'/>"
"<preset name='" N_("Facebook link image") "' label='1200 × 630 px' height='630' width='1200'/>"
"<preset name='" N_("Facebook profile picture") "' label='180 × 180 px' height='180' width='180' icon='social_square'/>"
"<preset name='" N_("Facebook video") "' label='1280 × 720 px' height='720' width='1280'/>"
"<preset name='" N_("Instagram landscape") "' label='1080 × 608 px' height='608' width='1080'/>"
"<preset name='" N_("Instagram portrait") "' label='1080 × 1350 px' height='1350' width='1080' icon='social_portrait'/>"
"<preset name='" N_("Instagram square") "' label='1080 × 1080 px' height='1080' width='1080' icon='social_square'/>"
"<preset name='" N_("LinkedIn business banner image") "' label='646 × 220 px' height='220' width='646'/>"
"<preset name='" N_("LinkedIn company logo") "' label='300 × 300 px' height='300' width='300' icon='social_square'/>"
"<preset name='" N_("LinkedIn cover photo") "' label='1536 × 768 px' height='768' width='1536'/>"
"<preset name='" N_("LinkedIn dynamic ad") "' label='100 × 100 px' height='100' width='100' icon='social_square'/>"
"<preset name='" N_("LinkedIn hero image") "' label='1128 × 376 px' height='376' width='1128'/>"
"<preset name='" N_("LinkedIn sponsored content image") "' label='1200 × 627 px' height='627' width='1200'/>"
"<preset name='" N_("Snapchat advertisement") "' label='1080 × 1920 px' height='1920' width='1080' icon='social_portrait'/>"

            "</template>"
        "</inkscape-extension>",
        std::make_unique<TemplateSocial>());
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
