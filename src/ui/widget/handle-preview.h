// SPDX-License-Identifier: GPL-2.0-or-later

#include <cairomm/surface.h>

// Helper function drawing a few selected handles at current handle size
// for use by the preferences dialog.

namespace Inkscape {

Cairo::RefPtr<Cairo::ImageSurface> draw_handles_preview(int device_scale);

}
