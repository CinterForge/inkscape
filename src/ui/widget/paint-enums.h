// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 11/1/25.
//

#ifndef INKSCAPE_PAINT_ENUMS_H
#define INKSCAPE_PAINT_ENUMS_H

#include <optional>
#include <string>

class SPIPaint;

namespace Inkscape::UI::Widget {

enum class PaintMode {
    Solid,
    Swatch,
    Gradient,
    Mesh,
    Pattern,
    Hatch,
    Derived,
    None,   // set to no paint
};

// Different ways paint can be inherited:
enum class PaintDerivedMode {
    Unset,          // paint is not set (inherited implicitly)
    Inherit,        // paint is set to 'inherit' keyword (inherited explicitly)
    ContextFill,    // context-fill (markers and clones; inherited from context element)
    ContextStroke,  // context-stroke
    CurrentColor,   // currentColor (inherited from "color" property)
};

// Take inherited paint mode and return corresponding CSS string
std::string get_inherited_paint_css_mode(PaintDerivedMode mode);

// Examine 'paint' and return mode that describes how it is to be derived/inherited.
// For paint servers and solid color the return is empty.
std::optional<PaintDerivedMode> get_inherited_paint_mode(const SPIPaint& paint);

} // namespace

#endif //INKSCAPE_PAINT_ENUMS_H
