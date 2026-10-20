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

#include <string>
#include <vector>

#include <libintl.h>

#include <gtkmm/box.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/spinbutton.h>

#include <glib/gstdio.h>

#include "desktop.h"
#include "selection.h"
#include "extension/effect.h"
#include "extension/system.h"
#include "imagemagick.h"
#include "xml/href-attribute-helper.h"

#include <Magick++.h>

namespace Inkscape {
namespace Extension {
namespace Internal {
namespace Bitmap {

namespace {
struct ImageInfo {
    Inkscape::XML::Node* node = nullptr;
    std::unique_ptr<Magick::Image> image;
    std::string cache;
    std::string original;
    SPItem* item = nullptr;
};
}
class ImageMagickDocCache: public Inkscape::Extension::Implementation::ImplementationDocumentCache {
    friend class ImageMagick;
private:
    void readImage(char const *xlink, char const *id, Magick::Image &image);
protected:
    std::vector<ImageInfo> images;
public:
    ImageMagickDocCache(SPDesktop *desktop);
};

ImageMagickDocCache::ImageMagickDocCache(SPDesktop *desktop)
    : Inkscape::Extension::Implementation::ImplementationDocumentCache(desktop)
{
    auto selected_item_list = desktop->getSelection()->items();
    images.reserve(std::ranges::distance(selected_item_list));

    // Loop through selected items
    for (auto item : selected_item_list) {
        Inkscape::XML::Node *node = item->getRepr();
        if (strcmp(node->name(), "image") != 0 && strcmp(node->name(), "svg:image") != 0) {
            continue;
        }
        char const *xlink = Inkscape::getHrefAttribute(*node).second;
        images.push_back({
            .node = node,
            .image = std::make_unique<Magick::Image>(),
            .original = xlink,
            .item = item
        });
        readImage(xlink, node->attribute("id"), *images.back().image);
    }
}

void ImageMagickDocCache::readImage(const char *xlink, const char *id, Magick::Image &image)
{
    // Find if the xlink:href is base64 data, i.e. if the image is embedded 
    gchar *search = g_strndup(xlink, 30);
    if (strstr(search, "base64") != (char*)NULL) {
        // 7 = strlen("base64") + strlen(",")
        const char* pureBase64 = strstr(xlink, "base64") + 7;        
        Magick::Blob blob;
        blob.base64(pureBase64);
        try {
            image.read(blob);
        } catch (Magick::Exception &error_) {
            g_warning("ImageMagick could not read '%s'\nDetails: %s", id, error_.what());
        }
    } else {
        gchar *path;
        if (strncmp (xlink,"file:", 5) == 0) {
            path = g_filename_from_uri(xlink, NULL, NULL);
        } else {
            path = g_strdup(xlink);
        }
        try {
            image.read(path);
        } catch (Magick::Exception &error_) {
            g_warning("ImageMagick could not read '%s' from '%s'\nDetails: %s", id, path, error_.what());
        }
        g_free(path);
    }
    g_free(search);
}

bool
ImageMagick::load(Inkscape::Extension::Extension */*module*/)
{
    return true;
}

Inkscape::Extension::Implementation::ImplementationDocumentCache *
ImageMagick::newDocCache(Inkscape::Extension::Extension * /*ext*/, SPDesktop *desktop) {
    return new ImageMagickDocCache(desktop);
}

void ImageMagick::effect(Inkscape::Extension::Effect *module, ExecutionEnv * /*executionEnv*/, SPDesktop *desktop,
                         Inkscape::Extension::Implementation::ImplementationDocumentCache *docCache)
{
    refreshParameters(module);
    ImageMagickDocCache * dc = dynamic_cast<ImageMagickDocCache *>(docCache);
    if (!dc) { // should really never happen
        return;
    }
    unsigned constexpr b64_line_length = 76;

    try {
        for (auto &image : dc->images) {
            Magick::Image effected_image = *image.image; // make a copy
            applyEffect(&effected_image);

            // postEffect can be used to change things on the item itself
            // e.g. resize the image element, after the effecti is applied
            postEffect(&effected_image, image.item);

            auto blob = std::make_unique<Magick::Blob>();
            effected_image.write(blob.get());

            std::string base64_string = blob->base64();
            for (size_t newline_pos = b64_line_length;
                 newline_pos < base64_string.length();
                 newline_pos += b64_line_length + 1) {
                base64_string.insert(newline_pos, "\n");
            }
            image.cache = "data:image/" + effected_image.magick() + ";base64, \n" + base64_string;

            Inkscape::setHrefAttribute(*image.node, image.cache);
            image.node->removeAttribute("sodipodi:absref");
        }
    } catch (Magick::Exception &error) {
        std::cerr << "ImageMagick effect exception:" << error.what() << std::endl;
    }
}

/** \brief  A function to get the preferences for the grid
    \param  module  Module which holds the params
    \param  desktop

    Uses AutoGUI for creating the GUI.
*/
Gtk::Widget *
ImageMagick::prefs_effect(Inkscape::Extension::Effect *module, SPDesktop *desktop, sigc::signal<void ()> * changeSignal,
                          Inkscape::Extension::Implementation::ImplementationDocumentCache * /*docCache*/)
{
  SPDocument * current_document = desktop->doc();

  auto selected = desktop->getSelection()->items();
  Inkscape::XML::Node * first_select = NULL;
  if (!selected.empty()) {
    first_select = (selected.front())->getRepr();
  }
  return module->autogui(current_document, first_select, changeSignal);
}

}; /* namespace Bitmap */
}; /* namespace Internal */
}; /* namespace Extension */
}; /* namespace Inkscape */
