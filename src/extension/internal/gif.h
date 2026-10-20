// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 4/15/25.
//

#ifndef GIF_H
#define GIF_H

#include "extension/implementation/implementation.h"

// GIF image exporter. It supports animated GIFs as well.
// To create an animated GIF, prepare a document with multiple pages.
// Export dialog "Pages" option can then be used to create an animated multiframe GIF.

namespace Inkscape::Extension::Internal {

class Gif : public Implementation::Implementation {
public:
    void save(Output* mod, SPDocument* doc, char const* filename) override;

    static void init();
};

}

#endif //GIF_H
