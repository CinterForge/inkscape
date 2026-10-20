// SPDX-License-Identifier: GPL-2.0-or-later

#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include "pattern-manipulation.h"

#include "desktop-style.h"
#include "colors/color.h"
#include "document.h"
#include "fill-or-stroke.h"
#include "helper/stock-items.h"
#include "object/sp-hatch.h"
#include "object/sp-pattern.h"
#include "xml/repr.h"
#include "style.h"

std::vector<SPDocument *> sp_get_stock_patterns() {
    auto patterns = StockPaintDocuments::get().get_paint_documents([](SPDocument* doc){
        return !sp_get_pattern_list(doc).empty();
    });
    if (patterns.empty()) {
        g_warning("No stock patterns!");
    }
    return patterns;
}

std::vector<SPDocument *> sp_get_stock_hatches() {
    auto hatches = StockPaintDocuments::get().get_paint_documents([](SPDocument* doc){
        return !sp_get_hatch_list(doc).empty();
    });
    if (hatches.empty()) {
        g_warning("No stock hatches!");
    }
    return hatches;
}

std::vector<SPPaintServer*> sp_get_pattern_list(SPDocument* source) {
    std::vector<SPPaintServer*> list;
    if (!source) return list;

    std::vector<SPObject*> patterns = source->getResourceList("pattern");
    for (auto pattern : patterns) {
        auto p = cast<SPPattern>(pattern);
        if (p && p == p->rootPattern() && p->hasChildren()) { // only if this is a valid root pattern
            list.push_back(p);
        }
    }

    return list;
}

std::vector<SPPaintServer*> sp_get_hatch_list(SPDocument* source) {
    std::vector<SPPaintServer*> list;
    if (!source) return list;

    std::vector<SPObject*> hatches = source->getResourceList("hatch");
    for (auto hatch : hatches) {
        auto h = cast<SPHatch>(hatch);
        if (h && h == h->rootHatch() && h->hasChildren()) { // only if this is a valid root hatch
            list.push_back(h);
        }
    }

    return list;
}

void sp_pattern_set_color(SPPattern* pattern, Inkscape::Colors::Color const &c)
{
    if (!pattern) return;

    SPCSSAttr* css = sp_repr_css_attr_new();
    sp_repr_css_set_property_string(css, "fill", c.toString());
    pattern->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);
}

void sp_pattern_set_transform(SPPattern* pattern, const Geom::Affine& transform) {
    if (!pattern) return;

    // for now, this is that simple
    pattern->transform_multiply(transform, true);
}

void sp_pattern_set_offset(SPPattern* pattern, const Geom::Point& offset) {
    if (!pattern) return;

    // TODO: verify
    pattern->setAttributeDouble("x", offset.x());
    pattern->setAttributeDouble("y", offset.y());
}

void sp_pattern_set_uniform_scale(SPPattern* pattern, bool uniform) {
    if (!pattern) return;

    //TODO: make smarter to keep existing value when possible
    pattern->setAttribute("preserveAspectRatio", uniform ? "xMidYMid" : "none");
}

void sp_pattern_set_gap(SPPattern* link_pattern, Geom::Scale gap_percent) {
    if (!link_pattern) return;
    auto root = link_pattern->rootPattern();
    if (!root || root == link_pattern) {
        g_assert(false && "Setting pattern gap requires link and root patterns objects");
        return;
    }

    auto set_gap = [=](double size, double percent, const char* attr) {
        if (percent == 0.0 || size <= 0.0) {
            // no gap
            link_pattern->removeAttribute(attr);
        }
        else if (percent > 0.0) {
            // positive gap
            link_pattern->setAttributeDouble(attr, size + size * percent / 100.0);
        }
        else if (percent < 0.0 && percent > -100.0) {
            // negative gap - overlap
            percent = -percent;
            link_pattern->setAttributeDouble(attr, size - size * percent / 100.0);
        }
    };

    set_gap(root->width(), gap_percent[Geom::X], "width");
    set_gap(root->height(), gap_percent[Geom::Y], "height");
}

Geom::Scale sp_pattern_get_gap(SPPattern* link_pattern) {
    Geom::Scale gap(0, 0);

    if (!link_pattern) return gap;
    auto root = link_pattern->rootPattern();
    if (!root || root == link_pattern) {
        g_assert(false && "Reading pattern gap requires link and root patterns objects");
        return gap;
    }

    auto get_gap = [=](double root_size, double link_size) {
        if (root_size > 0.0 && link_size > 0.0) {
            if (link_size > root_size) {
                return (link_size - root_size) / root_size;
            }
            else if (link_size < root_size) {
                return -link_size / root_size;
            }
        }
        return 0.0;
    };

    return Geom::Scale(
        get_gap(root->width(),  link_pattern->width())  * 100.0,
        get_gap(root->height(), link_pattern->height()) * 100.0
    );
}

std::string sp_get_pattern_label(SPPaintServer* pattern) {
    if (!pattern) return std::string();

    Inkscape::XML::Node* repr = pattern->getRepr();
    if (auto label = pattern->getAttribute("inkscape:label")) {
        if (*label) {
            return std::string(gettext(label));
        }
    }
    const char* stock_id = _(repr->attribute("inkscape:stockid"));
    const char* pat_id = stock_id ? stock_id : _(repr->attribute("id"));
    return std::string(pat_id ? pat_id : "");
}

void sp_item_set_pattern_style(SPItem* item, SPPattern* root_pattern, SPCSSAttr* css, FillOrStroke kind) {
    if (!item || !item->style || !item->getRepr()) {
        g_warning("No valid item provided to sp_item_set_pattern");
        return;
    }

    SPStyle* style = item->style;
    auto server = kind == FILL ? style->getFillPaintServer() : style->getStrokePaintServer();

    if (auto pattern = cast<SPPattern>(server); pattern && pattern->rootPattern() == root_pattern) {
        // only if this object's pattern is not rooted in our selected pattern, apply
        return;
    }

    if (kind == FILL) {
        sp_desktop_apply_css_recursive(item, css, true);
    }
    else {
        sp_repr_css_change_recursive(item->getRepr(), css, "style");
    }

    // create a link to the pattern right away, without waiting for an object to be moved;
    // otherwise the pattern editor may end up modifying a pattern shared by different objects
    item->adjust_pattern(Geom::Affine());
}

// set a pattern as item's fill or stroke; modify the pattern's attributes
void sp_item_apply_pattern(SPItem* item, SPPattern* pattern, FillOrStroke kind, std::optional<Color> color, const Glib::ustring& label,
    const Geom::Affine& transform, const Geom::Point& offset, bool uniform_scale, const Geom::Scale& gap) {

    if (!pattern || !item) return;

    auto link_pattern = pattern;
    auto root_pattern = pattern->rootPattern();
    if (color) {
        sp_pattern_set_color(root_pattern, color.value());
    }
    // pattern name is applied to the root
    root_pattern->setAttribute("inkscape:label", label.c_str());
    // the remaining settings apply to a link pattern
    if (link_pattern != root_pattern) {
        sp_pattern_set_transform(link_pattern, transform);
        sp_pattern_set_offset(link_pattern, offset);
        sp_pattern_set_uniform_scale(link_pattern, uniform_scale);
        // a gap requires both patterns, but they are only created later by calling "adjust_pattern" below
        // it is OK to ignore it for now, during the initial creation the gap is 0,0
        sp_pattern_set_gap(link_pattern, gap);
    }

    auto url = Glib::ustring::compose("url(#%1)", root_pattern->getRepr()->attribute("id"));

    SPCSSAttr* css = sp_repr_css_attr_new();
    sp_repr_css_set_property(css, kind == FILL ? "fill" : "stroke", url.c_str());
    sp_item_set_pattern_style(item, root_pattern, css, kind);

    // create a link to the pattern right away, without waiting for this item to be moved;
    // otherwise the pattern editor may end up modifying a pattern shared by different objects
    item->adjust_pattern(Geom::Affine());
}

void sp_hatch_set_pitch(SPHatch* hatch, double pitch) {
    if (!hatch) return;

    hatch->setAttributeDouble("pitch", pitch);
}

void sp_hatch_set_rotation(SPHatch* hatch, double angle) {
    if (!hatch) return;

    hatch->setAttributeDouble("rotate", angle);
}

void sp_hatch_set_transform(SPHatch* hatch, const Geom::Affine& transform) {
    if (!hatch) return;

    hatch->transform_multiply(transform, true);
}

void sp_hatch_set_offset(SPHatch* hatch, const Geom::Point& offset) {
    if (!hatch) return;

    hatch->setAttributeDouble("x", offset.x());
    hatch->setAttributeDouble("y", offset.y());
}

void sp_hatch_set_color(SPHatch* hatch, Color const &c) {
    if (!hatch) return;

    SPCSSAttr* css = sp_repr_css_attr_new();
    sp_repr_css_set_property_string(css, "stroke", c.toString());
    hatch->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);
}

void sp_hatch_set_stroke_width(SPHatch* hatch, double thickness) {
    if (!hatch) return;

    SPCSSAttr* css = sp_repr_css_attr_new();
    sp_repr_css_set_property_double(css, "stroke-width", thickness);
    hatch->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);
}

void sp_item_set_hatch_style(SPItem* item, SPHatch* root_hatch, SPCSSAttr* css, FillOrStroke kind) {
    if (!item || !item->style || !item->getRepr()) {
        g_warning("No valid item provided to sp_item_set_hatch_style");
        return;
    }

    SPStyle* style = item->style;
    auto server = kind == FILL ? style->getFillPaintServer() : style->getStrokePaintServer();

    if (auto hatch = cast<SPHatch>(server); hatch && hatch->rootHatch() == root_hatch) {
        // only if this object's hatch is not rooted in our selected hatch, apply
        return;
    }

    if (kind == FILL) {
        sp_desktop_apply_css_recursive(item, css, true);
    }
    else {
        sp_repr_css_change_recursive(item->getRepr(), css, "style");
    }

    // create a link to the hatch right away, without waiting for this item to be moved;
    // otherwise the pattern editor may end up modifying a hatch shared by different objects
    item->adjust_hatch(Geom::identity());
}

void sp_item_apply_hatch(SPItem* item, SPHatch* hatch, FillOrStroke kind, std::optional<Color> color, const Glib::ustring& label,
    const Geom::Affine& transform, const Geom::Point& offset, double pitch, double rotation, double thickness) {

    if (!hatch || !item) return;

    auto link_hatch = hatch;
    auto root_hatch = hatch->rootHatch();
    if (color) {
        sp_hatch_set_color(root_hatch, color.value());
    }
    sp_hatch_set_stroke_width(root_hatch, thickness);
    // hatch name is applied to the root
    root_hatch->setAttribute("inkscape:label", label.c_str());
    // the remaining settings apply to a link hatch
    if (link_hatch != root_hatch) {
        sp_hatch_set_pitch(link_hatch, pitch);
        sp_hatch_set_rotation(link_hatch, rotation);
        sp_hatch_set_transform(link_hatch, transform);
        sp_hatch_set_offset(link_hatch, offset);
        //todo: other attributes
        // hatch->setAttributeDouble("x", 0);
        // link_hatch->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
    }

    auto url = Glib::ustring::compose("url(#%1)", hatch->getRepr()->attribute("id"));

    SPCSSAttr* css = sp_repr_css_attr_new();
    sp_repr_css_set_property(css, kind == FILL ? "fill" : "stroke", url.c_str());
    sp_item_set_hatch_style(item, hatch, css, kind);
}
