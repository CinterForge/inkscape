// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 6/2/25.

#ifndef CSS_UTILS_H
#define CSS_UTILS_H

#include "object/sp-object.h"

namespace Inkscape::Css {

// Take the "style" attribute from a source object and apply it to destination.
// Leave the source object without the "style" attribute.
// Returns true if the style has been transferred.
bool transfer_item_style(SPObject* src, SPObject* dest);

// Remove the style attribute from the given object.
// Returns true if the style has been removed.
bool remove_item_style(SPObject* obj);

} // Inkscape

#endif //CSS_UTILS_H
