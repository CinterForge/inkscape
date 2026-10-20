//   SPDX-License-Identifier: GPL-2.0-or-later

// Simplistic color wheel factory

#ifndef COLOR_WHEEL_FACTORY_H
#define COLOR_WHEEL_FACTORY_H

#include "colors/spaces/enum.h"

namespace Inkscape::UI::Widget {
class ColorWheel;

// Create color wheel for requested type if there it one of that type; null otherwise
ColorWheel* create_managed_color_wheel(Colors::Space::Type type, bool disc);

// Is there a color wheel for this 'type'?
bool can_create_color_wheel(Colors::Space::Type type);

} // Inkscape

#endif //COLOR_WHEEL_FACTORY_H
