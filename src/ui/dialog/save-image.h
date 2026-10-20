// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEEN_SAVE_IMAGE_H
#define SEEN_SAVE_IMAGE_H

#include <gtkmm/image.h>

class SPImage;

namespace Inkscape {
class Pixbuf;

// Save 'pixbuf' image into a file
bool save_image(const std::string& fname, const Inkscape::Pixbuf* pixbuf);

// Use file chooser to select a path and then save the 'image'
bool extract_image(Gtk::Window* parent, SPImage* image);

} // namespace Inkscape

#endif // SEEN_SAVE_IMAGE_H
