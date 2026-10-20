// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Christopher Brown <audiere@gmail.com>
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "extension/effect.h"
#include "extension/system.h"

#include "colorize.h"

#include <iostream>
#include <Magick++.h>

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {

Colorize::Colorize()
    : _color(0x000000ff)
{}

void
Colorize::applyEffect(Magick::Image *image) {
    if (auto color = _color.converted(Colors::Space::Type::RGB)) {
        Magick::ColorRGB mc((*color)[0], (*color)[1], (*color)[2]);
        image->colorize(color->getOpacity() * 100, mc);
    }
}

void
Colorize::refreshParameters(Inkscape::Extension::Effect *module) {	
    _color = module->get_param_color("color");
}

#include "../clear-n_.h"

void
Colorize::init()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Colorize") "</name>\n"
            "<id>org.inkscape.effect.bitmap.colorize</id>\n"
            "<param name=\"color\" gui-text=\"" N_("Color") "\" type=\"color\">0</param>\n"
            "<effect>\n"
                "<object-type>all</object-type>\n"
                "<effects-menu>\n"
                    "<submenu name=\"" N_("Raster") "\" />\n"
                "</effects-menu>\n"
                "<menu-tip>" N_("Colorize selected bitmap(s) with specified color, using given opacity") "</menu-tip>\n"
            "</effect>\n"
        "</inkscape-extension>\n", std::make_unique<Colorize>());
    // clang-format on
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
