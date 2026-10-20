// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SubItem controls each fractured piece and links it to its original items.
 *
 *//*
 * Authors:
 *   Martin Owens
 *   PBS
 *
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "booleans-subitems.h"

#include <random>
#include <ranges>

#include "style.h"
#include "helper/geom-pathstroke.h"
#include "object/sp-item-group.h"
#include "object/sp-image.h"
#include "object/sp-text.h"
#include "object/sp-use.h"
#include "path/path-boolop.h"

namespace Inkscape {

// Todo: (Wishlist) Remove this function when no longer necessary to remove boolops artifacts.
static Geom::PathVector clean_pathvector(Geom::PathVector &&pathv)
{
    Geom::PathVector result;

    for (auto &path : pathv) {
        if (path.closed() && !is_path_empty(path)) {
            result.push_back(std::move(path));
        }
    }

    return result;
}

/**
 * Union operator, merges two subitems when requested by the user
 * The left hand side will retain priority for the resulting style
 * so you should be mindful of how you merge these shapes.
 */
SubItem &SubItem::operator+=(SubItem const &other)
{
    _paths = clean_pathvector(flattened(sp_pathvector_boolop(_paths, other._paths, bool_op_union, fill_nonZero, fill_nonZero), fill_nonZero));
    return *this;
}

/**
 * Test if this sub item is a special image type.
 */
bool SubItem::_get_is_image(SPItem const *item)
{
    return is<SPImage>(item) || is<SPUse>(item);
}

/**
 * A structure containing all the detected shapes and their respective spitems
 */
struct PathvectorItem {
    PathvectorItem(Geom::PathVector path, SPItem* item_root, SPItem* item_actual)
        : pathv(std::move(path))
        , root(item_root)
        , item(item_actual)
    {}
    Geom::PathVector pathv;
    SPItem* root;
    SPItem* item;
};
using PathvectorItems = std::vector<PathvectorItem>;

static void extract_pathvectors_recursive(SPItem *root, SPItem *item, PathvectorItems &result, Geom::Affine const &transform)
{
    if (is<SPGroup>(item)) {
        for (auto &child : item->children | std::views::reverse) {
            if (auto child_item = cast<SPItem>(&child)) {
                extract_pathvectors_recursive(root, child_item, result, child_item->transform * transform);
            }
        }
    } else if (auto img = cast<SPImage>(item)) {
        if (auto clip = img->getClipPathVector(root)) {
            // This needs to use the clipping region because get_curve is empty in this case
            result.emplace_back(*clip * transform, root, item);
        } else {
            result.emplace_back(*img->get_curve() * transform, root, item);
        }
    } else if (auto shape = cast<SPShape>(item)) {
        if (auto curve = shape->curve()) {
            result.emplace_back(*curve * transform, root, item);
        }
    } else if (auto text = cast<SPText>(item)) {
        result.emplace_back(text->getNormalizedBpath() * transform, root, item);
    } else if (auto use = cast<SPUse>(item)) {
        auto clip = use->getClipPathVector(root);
        if (clip && is<SPImage>(use->get_original())) {
            // A clipped clone of an image is consumed as a single object
            result.emplace_back(*clip * transform, root, item);
        } else if (use->child) {
            extract_pathvectors_recursive(root, use->child, result, use->child->transform * Geom::Translate(use->x.computed, use->y.computed) * transform);
        }
    }
}

/**
 * Take a list of items and fracture into a list of SubItems ready for
 * use inside the booleans interactive tool.
 */
WorkItems SubItem::build_mosaic(std::vector<SPItem*> &&items)
{
    // Sort so that topmost items come first.
    std::sort(items.begin(), items.end(), [] (auto a, auto b) {
        return sp_object_compare_position_bool(b, a);
    });

    // Extract all individual pathvectors within the collection of items,
    // keeping track of their associated item and style, again sorted topmost-first.
    PathvectorItems augmented;

    for (auto item : items) {
        // Get the correctly-transformed pathvectors, together with their corresponding styles.
        extract_pathvectors_recursive(item, item, augmented, item->i2dt_affine());
    }

    // We want the images to be the first items in augmented, so they get the priority for the style.
    // Bubble them to the front, otherwise preserving order.
    std::stable_partition(augmented.begin(), augmented.end(), [] (auto &pvi) {
        return is<SPImage>(pvi.item) || is<SPUse>(pvi.item);
    });

    // Compute a slightly expanded bounding box, collect together all lines, and cut the former by the latter.
    Geom::OptRect bounds;
    Geom::PathVector lines;

    for (auto &pvi : augmented) {
        bounds |= pvi.pathv.boundsExact();
        for (auto &path : pvi.pathv) {
            lines.push_back(path);
        }
    }

    if (!bounds) {
        return {};
    }

    constexpr double expansion = 10.0;
    bounds->expandBy(expansion);

    auto bounds_pathv = Geom::PathVector(Geom::Path(*bounds));
    auto pieces = pathvector_cut(bounds_pathv, lines);

    // Construct the SubItems, attempting to guess the corresponding augmented item for each piece.
    WorkItems result;

    auto gen = std::default_random_engine(std::random_device()());
    auto ranf = [&] { return std::uniform_real_distribution()(gen); };
    auto randpt = [&] { return Geom::Point(ranf(), ranf()); };

    for (auto &piece : pieces) {
        // Skip the big enclosing piece that is touching the outer boundary.
        if (auto rect = piece.boundsExact()) {
            if (   Geom::are_near(rect->top(), bounds->top(), expansion / 2)
                || Geom::are_near(rect->bottom(), bounds->bottom(), expansion / 2)
                || Geom::are_near(rect->left(), bounds->left(), expansion / 2)
                || Geom::are_near(rect->right(), bounds->right(), expansion / 2))
            {
                continue;
            }
        }

        // Remove junk paths that are open and/or tiny.
        for (auto it = piece.begin(); it != piece.end(); ) {
            if (!it->closed() || is_path_empty(*it)) {
                it = piece.erase(it);
            } else {
                ++it;
            }
        }

        // Skip empty pathvectors.
        if (piece.empty()) {
            continue;
        }

        // Determine the corresponding augmented item.
        // Fixme: (Wishlist) This is done unreliably and hackily, but livarot/2geom seemingly offer no alternative.
        std::unordered_map<PathvectorItem*, int> hits;

        auto rect = piece.boundsExact();

        auto add_hit = [&] (Geom::Point const &pt) {
            // Find an augmented item containing the point.
            for (auto &pvi : augmented) {
                auto fill_rule = pvi.item->style->fill_rule.computed;
                auto winding = pvi.pathv.winding(pt);
                if (fill_rule == SP_WIND_RULE_NONZERO ? winding : winding % 2) {
                    hits[&pvi]++;
                    return;
                }
            }

            // If none exists, register a background hit with nullptr.
            hits[nullptr]++;
        };

        for (int total_hits = 0, patience = 1000; total_hits < 20 && patience > 0; patience--) {
            // Attempt to generate a point strictly inside the piece.
            auto pt = rect->min() + randpt() * rect->dimensions();
            if (piece.winding(pt)) {
                add_hit(pt);
                total_hits++;
            }
        }

        // Pick the augmented item with the most hits.
        PathvectorItem *found = nullptr;
        int max_hits = 0;

        for (auto &[a, h] : hits) {
            if (h > max_hits) {
                max_hits = h;
                found = a;
            }
        }

        // Add the SubItem.
        auto root = found ? found->root : nullptr;
        auto item = found ? found->item : nullptr;
        auto style = item ? item->style : nullptr;
        result.emplace_back(std::make_shared<SubItem>(std::move(piece), root, item, style));
    }

    return result;
}

/**
 * Take a list of items and flatten into a list of SubItems.
 */
WorkItems SubItem::build_flatten(std::vector<SPItem*> &&items)
{
    // Sort so that topmost items come first.
    std::sort(items.begin(), items.end(), [] (auto a, auto b) {
        return sp_object_compare_position_bool(b, a);
    });

    WorkItems result;
    Geom::PathVector unioned;

    for (auto item : items) {
        // Get the correctly-transformed pathvectors, together with their corresponding styles.
        PathvectorItems extracted;
        extract_pathvectors_recursive(item, item, extracted, item->i2dt_affine());

        for (auto &[pathv, root, subitem] : extracted) {
            // Close any non closed objects
            for (auto &it : pathv) {
                if (!it.closed()) {
                    it.close();
                }
            }

            // Skip pathvectors that are empty
            if (pathv.empty()) {
                continue;
            }

            // Flatten the remaining pathvector according to its fill rule.
            auto fillrule = subitem->style->fill_rule.computed;
            flatten(pathv, to_livarot(fillrule));

            // Remove the union so far from the shape, then add the shape to the union so far.
            Geom::PathVector uniq;

            if (unioned.empty()) {
                uniq = pathv;
                unioned = std::move(pathv);
            } else {
                uniq = sp_pathvector_boolop(unioned, pathv, bool_op_diff, fill_nonZero, fill_nonZero);
                unioned = sp_pathvector_boolop(unioned, pathv, bool_op_union, fill_nonZero, fill_nonZero);
            }

            // Add the new SubItem.
            result.emplace_back(std::make_shared<SubItem>(std::move(uniq), root, subitem, subitem->style));
        }
    }

    return result;
}

/**
 * Return true if this subitem contains the give point.
 */
bool SubItem::contains(Geom::Point const &pt) const
{
    return _paths.winding(pt) % 2 != 0;
}

} // namespace Inkscape
