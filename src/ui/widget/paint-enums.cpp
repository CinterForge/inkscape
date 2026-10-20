// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 11/1/25.
//

#include "paint-enums.h"
#include "style-internal.h"

namespace Inkscape::UI::Widget {

std::optional<PaintDerivedMode> get_inherited_paint_mode(const SPIPaint& paint) {
    if (!paint.isDerived()) {
        return {}; // not derived a derived paint
    }

    switch (paint.paintSource) {
    case SP_CSS_PAINT_ORIGIN_CONTEXT_FILL:
        return PaintDerivedMode::ContextFill;
    case SP_CSS_PAINT_ORIGIN_CONTEXT_STROKE:
        return PaintDerivedMode::ContextStroke;
    case SP_CSS_PAINT_ORIGIN_CURRENT_COLOR:
        return PaintDerivedMode::CurrentColor;
    default:
        // check of 'inherit' keyword and "unset" paint
        if (paint.inheritSource) {
            return PaintDerivedMode::Inherit;
        }
        if (!paint.set) {
            return PaintDerivedMode::Unset;
        }
    }

    // all other combinations, if any
    g_warning("get_inherited_paint_mode - unrecognized paint combination.");
    return {};
}

std::string get_inherited_paint_css_mode(PaintDerivedMode mode) {
    switch (mode) {
    case PaintDerivedMode::Unset:
        return {};
    case PaintDerivedMode::Inherit:
        return "inherit";
    case PaintDerivedMode::ContextFill:
        return "context-fill";
    case PaintDerivedMode::ContextStroke:
        return "context-stroke";
    case PaintDerivedMode::CurrentColor:
        return "currentColor";
    default:
        g_warning("get_inherited_paint_css_mode(): unknown mode");
    }

    return {};
}

}
