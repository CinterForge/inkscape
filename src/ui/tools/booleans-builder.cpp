// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Boolean tool shape builder.
 *
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "booleans-builder.h"

#include "desktop.h"
#include "document.h"
#include "style.h"

#include "actions/actions-undo-document.h"
#include "display/control/canvas-item-group.h"
#include "display/control/canvas-item-bpath.h"
#include "object/object-set.h"
#include "object/sp-image.h"
#include "object/sp-use.h"
#include "object/sp-clippath.h"
#include "object/sp-namedview.h"
#include "object/sp-defs.h"
#include "ui/widget/canvas.h"
#include "svg/svg.h"

namespace Inkscape {

static constexpr std::array<uint32_t, 6> fill_lite = {0x00000055, 0x0291ffff, 0x8eceffff, 0x0291ffff, 0xf299d6ff, 0xff0db3ff};
static constexpr std::array<uint32_t, 6> fill_dark = {0xffffff55, 0x0291ffff, 0x8eceffff, 0x0291ffff, 0xf299d6ff, 0xff0db3ff};

BooleanBuilder::BooleanBuilder(ObjectSet *set, bool flatten)
    : _set(set)
{
    // Current state of all the items
    _work_items = (flatten ? SubItem::build_flatten : SubItem::build_mosaic)(set->items_vector());

    if (_set->desktop()) {
        auto root = _set->desktop()->getCanvas()->get_canvas_item_root();
        _group = make_canvasitem<CanvasItemGroup>(root);

        // Build some image screen items
        for (auto &subitem : _work_items) {
            if (!subitem->is_image())
                continue;
            // Somehow show the image to the user.
        }

        auto nv = _set->desktop()->getNamedView();
        desk_modified_connection = nv->connectModified([this](SPObject *obj, guint flags) {
            redraw_items();
        });
        redraw_items();
    }
}

BooleanBuilder::~BooleanBuilder() = default;

/**
 * Control the visual appearence of this particular bpath
 */
void BooleanBuilder::redraw_item(CanvasItemBpath &bpath, bool selected, TaskType task, bool image)
{
    int i = (int)task * 2 + (int)selected;
    auto fill = _dark ? fill_dark[i] : fill_lite[i];
    if (image) {
        // Make image items less opaque
        fill = (fill | 0xff) - 0xcc;
    }
    bpath.set_fill(fill, SP_WIND_RULE_POSITIVE);
    bpath.set_stroke(task == TaskType::NONE ? 0x000000dd : 0xffffffff);
    bpath.set_stroke_width(task == TaskType::NONE ? 1.0 : 3.0);
}

/**
 * Update to visuals with the latest subitem list.
 */
void BooleanBuilder::redraw_items()
{
    if (!_set->desktop()) {
        return;
    }

    _dark = Colors::get_perceptual_lightness(_set->desktop()->getNamedView()->getDeskColor()) < 0.5;

    _screen_items.clear();

    for (auto &subitem : _work_items) {
        // Construct BPath from each subitem!
        auto bpath = make_canvasitem<Inkscape::CanvasItemBpath>(_group.get(), subitem->get_pathv(), false);
        redraw_item(*bpath, subitem->getSelected(), TaskType::NONE, subitem->is_image());
        _screen_items.push_back({ subitem, std::move(bpath), true });
    }

    // Selectively handle the undo actions being enabled / disabled
    enable_undo_actions(_set->document(), _undo.size(), _redo.size());
}

ItemPair *BooleanBuilder::get_item(const Geom::Point &point)
{
    for (auto &pair : _screen_items) {
        if (pair.vis->contains(point, 2.0))
            return &pair;
    }
    return nullptr;
}

/**
 * Highlight any shape under the mouse at this point.
 */
bool BooleanBuilder::highlight(const Geom::Point &point, bool add)
{
    if (has_task())
        return true;

    bool done = false;
    for (auto &si : _screen_items) {
        bool hover = !done && si.vis->contains(point, 2.0);
        redraw_item(*si.vis, si.work->getSelected(), hover ? (add ? TaskType::ADD : TaskType::DELETE) : TaskType::NONE, si.work->is_image());
        if (hover)
            si.vis->raise_to_top();
        done = done || hover;
    }
    return done;
}

/**
 * Returns true if this root item contains an image work item.
 */
bool BooleanBuilder::contains_image(SPItem *root) const
{
    for (auto &subitem : _work_items) {
        if (subitem->get_root() == root && subitem->is_image()) {
            return true;
        }
    }
    return false;
}

/**
 * Select the shape under the cursor
 */
bool BooleanBuilder::task_select(const Geom::Point &point, bool add_task)
{
    if (has_task())
        task_cancel();
    if (auto si = get_item(point)) {
        _add_task = add_task;
        _work_task = std::make_shared<SubItem>(*si->work);
        _work_task->setSelected(true);
        _screen_task = make_canvasitem<Inkscape::CanvasItemBpath>(_group.get(), _work_task->get_pathv(), false);
        redraw_item(*_screen_task, true, add_task ? TaskType::ADD : TaskType::DELETE, _work_task->is_image());
        si->vis->set_visible(false);
        si->visible = false;
        redraw_item(*si->vis, false, TaskType::NONE, _work_task->is_image());
        return true;
    }
    return false;
}

bool BooleanBuilder::task_add(const Geom::Point &point)
{
    if (!has_task())
        return false;
    if (auto si = get_item(point)) {
        // Invisible items are already processed.
        if (si->visible) {
            si->vis->set_visible(false);
            si->visible = false;
            *_work_task += *si->work;
            _screen_task->set_bpath(_work_task->get_pathv(), false);
            return true;
        }
    }
    return false;
}

void BooleanBuilder::task_cancel()
{
    _work_task.reset();
    _screen_task.reset();
    for (auto &si : _screen_items) {
        si.vis->set_visible(true);
        si.visible = true;
    }
}

void BooleanBuilder::task_commit()
{
    if (!has_task())
        return;

    // Manage undo/redo
    _undo.emplace_back(std::move(_work_items));
    _redo.clear();

    // A. Delete all items from _work_items that aren't visible
    _work_items.clear();
    for (auto &si : _screen_items) {
        if (si.visible) {
            _work_items.emplace_back(si.work);
        }
    }
    if (_add_task) {
        // B. Add _work_task to _work_items for union tasks
        _work_items.emplace_back(std::move(_work_task));
    }

    // C. Reset everything
    redraw_items();
    _work_task.reset();
    _screen_task.reset();
}

/**
 * Commit the changes to the document (finish)
 */
std::vector<SPObject *> BooleanBuilder::shape_commit(bool all, bool replace)
{
    std::vector<SPObject *> ret;
    std::map<SPItem *, SPItem *> used_images;
    auto doc = _set->document();
    auto items = _set->items_vector();
    auto defs = doc->getDefs();
    auto xml_doc = doc->getReprDoc();

    // Only commit anything if we have changes, return selection.
    if (!has_changes() && !all) {
        ret.insert(ret.begin(), items.begin(), items.end());
        return ret;
    }

    // Count number of selected items.
    int selected = 0;
    for (auto const &subitem : _work_items) {
        selected += (int)subitem->getSelected();
    }

    for (auto const &subitem : _work_items) {
        // Either this object is selected, or no objects are selected at all.
        if (!subitem->getSelected() && selected)
            continue;
        auto root = subitem->get_root();
        auto item = subitem->get_item();
        auto style = subitem->getStyle();
        // For the rare occasion the user generates from a hole (no item)
        if (!root) {
            root = *items.begin();
            style = root->style;
        }
        if (!root) {
            g_warning("Can't generate itemless object in boolean-builder.");
            continue;
        }
        auto parent = cast<SPItem>(root->parent);
        if (!parent) {
            g_warning("Can't generate top-most object in boolean-builder.");
            continue;
        }

        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");
        repr->setAttribute("d", sp_svg_write_path(subitem->get_pathv() * parent->dt2i_affine()));
        repr->setAttribute("style", style->writeIfDiff(parent->style));

        // Images and clipped clones are re/clipped instead of path-constructs
        if ((is<SPImage>(item) || is<SPUse>(item)) && item->getId()) {
            if (is<SPImage>(item)) {
                // An image may have been contained without groups or layers with transforms
                // moving it to the defs would lose this information. So we add it in now.
                auto tr = i2anc_affine(item, parent);

                // Make a copy of the image when not replacing it
                if (!used_images.contains(item)) {
                    auto orig = item;
                    if (item->parent != defs && !replace) {
                        auto copy_repr = item->getRepr()->duplicate(xml_doc);
                        item = cast<SPItem>(defs->appendChildRepr(copy_repr));
                    }
                    item->setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(tr));
                    used_images[orig] = item;
                } else {
                    // Make sure the id we use below is the copy, or the original dependiong on replace
                    item = used_images[item];
                }
            }

            // Consume existing repr as the clipPath and replace with clone of image
            Geom::Affine clone_tr = Geom::identity();
            std::vector<Inkscape::XML::Node *> paths = {repr};
            std::string clip_id = SPClipPath::create(paths, doc);
            std::string href_id = std::string("#") + item->getId();

            if (is<SPUse>(item)) {
                href_id = item->getAttribute("xlink:href");
                clone_tr = i2anc_affine(item, parent);
                // Remove the original clone's transform from the new clip object
                repr->setAttribute("transform", sp_svg_transform_write(clone_tr.inverse()));
            }

            repr = xml_doc->createElement("svg:use");
            repr->setAttribute("x", "0");
            repr->setAttribute("y", "0");
            repr->setAttribute("xlink:href", href_id);
            repr->setAttribute("clip-path", std::string("url(#") + clip_id + ")");
            repr->setAttribute("transform", sp_svg_transform_write(clone_tr));
        }

        parent->getRepr()->addChild(repr, root->getRepr());
        ret.emplace_back(doc->getObjectByRepr(repr));
    }
    _work_items.clear();

    for (auto &[orig, item] : used_images) {
        // Images that are used in a fragment are moved,
        if (item->parent != defs && replace) {
            auto img_repr = item->getRepr();
            sp_repr_unparent(img_repr);
            defs->getRepr()->appendChild(img_repr);
        }
    }

    // Apart from the used images, everything else is to be deleted.
    if (replace) {
        for (auto item : items) {
            if (!used_images.contains(item)) {
                sp_object_ref(item, nullptr);
            }
        }
        for (auto item : items) {
            if (!used_images.contains(item)) {
                item->deleteObject();
                sp_object_unref(item, nullptr);
            }
        }
    }

    return ret;
}

void BooleanBuilder::undo()
{
    if (_undo.empty())
        return;

    // Cancel any task;
    task_cancel();

    // Shuffle the undo stack
    _redo.emplace_back(std::move(_work_items));
    _work_items = std::move(_undo.back());
    _undo.pop_back();

    // Redraw the screen items
    redraw_items();
}

void BooleanBuilder::redo()
{
    if (_redo.empty())
        return;

    // Cancel any task;
    task_cancel();

    // Shuffle the undo stack
    _undo.emplace_back(std::move(_work_items));
    _work_items = std::move(_redo.back());
    _redo.pop_back();

    // Redraw the screen items
    redraw_items();
}

} // namespace Inkscape
