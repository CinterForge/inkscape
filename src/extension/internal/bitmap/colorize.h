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

#include "imagemagick.h"

#include "colors/color.h"

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {


class Colorize : public ImageMagick {
public:
    Colorize();

    void applyEffect(Magick::Image *image) override;
	void refreshParameters(Inkscape::Extension::Effect *module) override;

    static void init ();

private:
    Colors::Color _color;
};

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
