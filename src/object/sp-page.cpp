// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape pages implementation
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-page.h"
#include "page-manager.h"

#include <glibmm/i18n.h>

#include "attributes.h"
#include "document.h"

#include "display/control/canvas-page.h"
#include "object/object-set.h"
#include "util/units.h"

using Inkscape::DocumentUndo;

SPPage::SPPage()
    : SPObject()
{
    _canvas_item = new Inkscape::CanvasPage();
}

SPPage::~SPPage()
{
    delete _canvas_item;
    _canvas_item = nullptr;
}

void SPPage::build(SPDocument *document, Inkscape::XML::Node *repr)
{
    SPObject::build(document, repr);

    // SVG 2.0 Attributes
    this->readAttr(SPAttr::VIEWBOX);

    // Inkscape attributes
    this->readAttr(SPAttr::PAGE_MARGIN);
    this->readAttr(SPAttr::PAGE_BLEED);
    this->readAttr(SPAttr::INKSCAPE_LABEL);
    this->readAttr(SPAttr::PAGE_SIZE_NAME);

    /* Register, this creates a delayed signal for page-manager */
    document->addResource("page", this);
}

void SPPage::release()
{
    if (this->document) {
        // Unregister ourselves
        hidePage();
        this->document->removeResource("page", this);
    }

    SPObject::release();
}

void SPPage::set(SPAttr key, const gchar *value)
{
    switch (key) {
        case SPAttr::VIEWBOX:
            set_viewBox(value);
            break;
        case SPAttr::PAGE_MARGIN:
            this->margin.readOrUnset(value, document->getDocumentScale());
            break;
        case SPAttr::PAGE_BLEED:
            this->bleed.readOrUnset(value, document->getDocumentScale());
            break;
        case SPAttr::PAGE_SIZE_NAME:
            this->_size_label = value ? std::string(value) : "";
            break;
        default:
            SPObject::set(key, value);
            break;
    }
    update_relatives();
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void SPPage::position_changed(SPObject *prev)
{
    document->getPageManager().reorderPages();
}

/**
 * Update the percentage values of the svg boxes
 */
void SPPage::update_relatives()
{
    if (viewBox_set) {
        auto rect = getRect();
        if (this->margin)
            this->margin.update(12, 6, rect.width(), rect.height());
        if (this->bleed)
            this->bleed.update(12, 6, rect.width(), rect.height());
    }
}

/**
 * Returns true if the only aspect to this page is its size
 */
bool SPPage::isBarePage() const
{
    if (margin || bleed) {
        return false;
    }
    return true;
}

/**
 * Gets the rectangle in document units
 */
Geom::Rect SPPage::getRect() const
{
    return viewBox;
}

/**
 * Get the rectangle of the page, in desktop units
 */
Geom::Rect SPPage::getDesktopRect() const
{
    return getDocumentRect() * document->doc2dt();
}

/**
 * Get document rect, minus the margin amounts.
 */
Geom::Rect SPPage::getDocumentMargin() const
{
    auto rect = getRect();
    rect.setTop(rect.top() + margin.top().computed);
    rect.setLeft(rect.left() + margin.left().computed);
    rect.setBottom(rect.bottom() - margin.bottom().computed);
    rect.setRight(rect.right() - margin.right().computed);
    if (rect.hasZeroArea())
        return getDocumentRect(); // Cancel!
    return rect * document->getDocumentScale();
}

Geom::Rect SPPage::getDesktopMargin() const
{
    return getDocumentMargin() * document->doc2dt();
}

/**
 * Get page rect enlarged by bleed amounts
 */
Geom::Rect SPPage::getBleed() const
{
    auto rect = getRect();
    rect.setTop(rect.top() - bleed.top().computed);
    rect.setLeft(rect.left() - bleed.left().computed);
    rect.setBottom(rect.bottom() + bleed.bottom().computed);
    rect.setRight(rect.right() + bleed.right().computed);

    if (rect.hasZeroArea()) {
        return getRect();
    }
    return rect;
}

/**
 * Get document rect, plus the bleed amounts.
 */
Geom::Rect SPPage::getDocumentBleed() const
{
    return getBleed() * document->getDocumentScale();
}

Geom::Rect SPPage::getDesktopBleed() const
{
    return getDocumentBleed() * document->doc2dt();
}

/**
 * Get the rectangle of the page, scaled to the document.
 */
Geom::Rect SPPage::getDocumentRect() const
{
    return getRect() * document->getDocumentScale();
}

/**
 * Like getDesktopRect but returns a slightly shrunken rectangle
 * so interactions don't confuse the border with the object.
 */
Geom::Rect SPPage::getSensitiveRect() const
{
    auto rect = getDesktopRect();
    rect.expandBy(-0.1);
    return rect;
}

/**
 * Set the page rectangle in its native units.
 */
void SPPage::setRect(Geom::Rect rect)
{
    viewBox = rect;

    // always clear size label, toolbar is responsible for putting it back if needed.
    this->_size_label = "";

    // This is needed to update the xml
    this->updateRepr();

    // This eventually calls the ::update below while idle
    this->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Set the page rectangle in document coordinates.
 */
void SPPage::setDocumentRect(Geom::Rect rect, bool add_margins)
{
    rect *= document->getDocumentScale().inverse();
    if (add_margins) {
        // Add margins to rectangle.
        rect.setTop(rect.top() - margin.top().computed);
        rect.setLeft(rect.left() - margin.left().computed);
        rect.setBottom(rect.bottom() + margin.bottom().computed);
        rect.setRight(rect.right() + margin.right().computed);
    }
    setRect(rect);
}

/**
 * Set the page rectangle in desktop coordinates.
 */
void SPPage::setDesktopRect(Geom::Rect rect)
{
    setDocumentRect(rect * document->dt2doc());
}

/** @brief
 * Set just the height and width from a predefined size.
 * These dimensions are in document units, which happen to be the same
 * as desktop units, since pages are aligned to the coordinate axes.
 *
 * @param width The desired width in document/desktop units.
 * @param height The desired height in document/desktop units.
 */
void SPPage::setSize(double width, double height)
{
    auto rect = getDocumentRect();
    rect.setMax(rect.corner(0) + Geom::Point(width, height));
    setDocumentRect(rect);
}

/**
 * Set the page's margin
 */
void SPPage::setMargin(const std::string &value)
{
    this->margin.fromString(value, document->getDisplayUnit()->abbr, document->getDocumentScale());
    this->updateRepr();
}

/**
 * Set the page's bleed
 */
void SPPage::setBleed(const std::string &value)
{
    this->bleed.fromString(value, document->getDisplayUnit()->abbr, document->getDocumentScale());
    this->updateRepr();
}

/**
 * Get the margin side of the box.
 */
double SPPage::getMarginSide(int side)
{
    return this->margin.get((BoxSide)side);
}

/**
 * Set the margin at this side of the box in user units.
 */
void SPPage::setMarginSide(int side, double value, bool confine)
{
    if (confine && !margin) {
        this->margin.set(value, value, value, value);
    } else {
        this->margin.set((BoxSide)side, value, confine);
    }
    this->updateRepr();
}
/**
 * Set the margin at this side in display units.
 */
void SPPage::setMarginSide(int side, const std::string &value, bool confine)
{
    auto scale = document->getDocumentScale();
    auto unit = document->getDisplayUnit()->abbr;
    if (confine && !margin) {
        this->margin.fromString(value, unit, scale);
    } else {
        this->margin.fromString((BoxSide)side, value, unit, scale);
    }
    this->updateRepr();
}

std::string SPPage::getMarginLabel() const
{
    if (!margin || margin.isZero())
        return "";
    auto scale = document->getDocumentScale();
    auto unit = document->getDisplayUnit()->abbr;
    return margin.toString(unit, scale, 2);
}

std::string SPPage::getBleedLabel() const
{
    if (!bleed || bleed.isZero())
        return "";
    auto scale = document->getDocumentScale();
    auto unit = document->getDisplayUnit()->abbr;
    return bleed.toString(unit, scale, 2);
}

double SPPage::getBleedSide(int side)
{
    return this->bleed.get((BoxSide)side);
}

void SPPage::setBleedSide(int side, double value)
{
    this->bleed.set((BoxSide)side, value, false);
    this->updateRepr();
}

/**
 * Get the items which are ONLY on this page and don't overlap.
 *
 * This ignores layers so items in the same layer which are shared
 * between pages are not moved around or exported into pages they
 * shouldn't be.
 *
 * @param hidden - Return hidden items (default: true)
 * @param in_bleed - Use the bleed box instead of the page box
 * @param in_layers - Should layers be traversed to find items (default: true)
 */
std::vector<SPItem *> SPPage::getExclusiveItems(bool hidden, bool in_bleed, bool in_layers) const
{
    return document->getItemsInBox(0, in_bleed ? getDocumentBleed() : getDocumentRect(), hidden, true, true, false, in_layers);
}

/**
 * Like ExcludiveItems above but get all the items which are inside or overlapping.
 *
 * @param hidden - Return hidden items (default: true)
 * @param in_bleed - Use the bleed box instead of the page box
 * @param in_layers - Should layers be traversed to find items (default: true)
 */
std::vector<SPItem *> SPPage::getOverlappingItems(bool hidden, bool in_bleed, bool in_layers) const
{
    return document->getItemsPartiallyInBox(0, in_bleed ? getDocumentBleed() : getDocumentRect(), hidden, true, true, false, in_layers);
}

/**
 * Return true if this item is contained within the page boundary.
 *
 * @param item - The SPItem to check for page intersection or containment
 * @param contains - If true the test will be if the shape is entirely contained,
                     rather than just partly overlaps.
 * @param groups - If true, groups will be tested as a whole object instead of one
 *                 object at a time.
 */
bool SPPage::itemOnPage(SPItem const *item, bool contains, bool groups) const
{
    if (!groups && is<SPGroup>(item)) {
        return std::any_of(item->children.begin(),
                   item->children.end(),
                   [this](auto const &obj) {
                       auto const *as_item = cast<SPItem>(&obj);
                       return as_item && itemOnPage(as_item);
                   });
    }
    if (auto box = item->desktopGeometricBounds()) {
        if (contains) {
            return getDesktopRect().contains(*box);
        }
        return getDesktopRect().intersects(*box);
    }
    return false;
}

/**
 * Returns true if this page is the same as the viewport.
 */
bool SPPage::isViewportPage() const
{
    auto rect = document->preferredBounds();
    return _can_be_viewport && getDocumentRect().corner(0).floor() == rect->corner(0).floor();
}

/**
 * Shows the page in the given canvas item group.
 */
void SPPage::showPage(Inkscape::CanvasItemGroup *fg, Inkscape::CanvasItemGroup *bg)
{
    _canvas_item->add(getDesktopRect(), fg, bg);
    setDefaultAttributes();
    // The final steps are completed in an update cycle
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Sets the default attributes from the namedview.
 */
bool SPPage::setDefaultAttributes()
{
    if (document->getPageManager().setDefaultAttributes(_canvas_item)) {
        this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        return true;
    }
    return false;
}

/**
 * Set the selected high-light for this page.
 */
void SPPage::setSelected(bool sel)
{
    this->_canvas_item->is_selected = sel;
    this->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

/**
 * Returns the page number (order of pages) starting at 1
 */
int SPPage::getPageIndex() const
{
    return document->getPageManager().getPageIndex(this);
}

/**
 * Set this page to a new order in the page stack.
 *
 * @param index - Placement of page in the stack, starting at '0'
 * @param swap_page - Swap the rectangle position
 *
 * @returns true if page has been moved.
 */
bool SPPage::setPageIndex(int index, bool swap_page)
{
    int current = getPageIndex();

    if (current != index) {
        auto &page_manager = document->getPageManager();

        // The page we're going to be shifting to
        auto sibling = page_manager.getPage(index);

        // Insertions are done to the right of the sibling
        if (index < current) {
            index -= 1;
        }
        auto insert_after = page_manager.getPage(index);

        // We may have selected an index off the end, so attach it after the last page.
        if (!insert_after && index > 0) {
            insert_after = page_manager.getLastPage();
            sibling = nullptr; // disable swap
        }

        if (insert_after) {
            if (this == insert_after) {
                g_warning("Page is already at this index. Not moving.");
                return false;
            }
            // Attach after the given page
            getRepr()->parent()->changeOrder(getRepr(), insert_after->getRepr());
        } else {
            // Attach to before any existing page
            sibling = page_manager.getFirstPage();
            getRepr()->parent()->changeOrder(getRepr(), nullptr);
        }
        if (sibling && swap_page) {
            swapPage(sibling, true);
        }

        page_manager.selectPage(this);

        return true;
    }
    return false;
}

/**
 * Move the page by the given affine, in desktop units.
 *
 * @param translate - The positional translation to apply.
 * @param with_objects - Flag to request that connected objects also move.
 */
void SPPage::movePage(Geom::Affine translate, bool with_objects)
{
    if (translate.isTranslation()) {
        if (with_objects) {
            // Move each item that is overlapping this page too
            moveItems(translate, getOverlappingItems());
        }
        setDesktopRect(getDesktopRect() * translate);
    }
}

/**
 * Move the given items by the given translation in document units.
 *
 * @param translate - The movement to be applied
 * @param objects - a vector of SPItems to move
 */
void SPPage::moveItems(Geom::Affine translate, std::vector<SPItem *> const &items)
{
    if (items.empty()) {
        return;
    }
    Inkscape::ObjectSet set(items[0]->document);
    for (auto &item : items) {
        if (item->isLocked()) {
            continue;
        }
        set.add(item);
    }
    set.applyAffine(translate, true, false, true);
}

/**
 * Swap the locations of this page with another page (see movePage)
 *
 * @param other - The other page to swap with
 * @param with_objects - Should the page objects move too.
 */
void SPPage::swapPage(SPPage *other, bool with_objects)
{
    // Flip direction so we know which page will be the viewport
    if (other->isViewportPage()) {
        other->swapPage(this, with_objects);
        return;
    }

    // Swapping with the viewport page must be handled gracefully.
    if (this->isViewportPage()) {
        auto other_rect = other->getDesktopRect();
        auto new_rect = Geom::Rect(Geom::Point(0, 0),
            Geom::Point(other_rect.width(), other_rect.height()));
        this->document->fitToRect(new_rect, false);
    }

    // Prevent any resizing of these pages by the viewport when doc->ensureUpToDate()
    // is called by the move objects functionality that needs accurate bboxes.
    _can_be_viewport = false;
    other->_can_be_viewport = false;

    auto this_affine = Geom::Translate(getDesktopRect().corner(0));
    auto other_affine = Geom::Translate(other->getDesktopRect().corner(0));
    movePage(this_affine.inverse() * other_affine, with_objects);
    other->movePage(other_affine.inverse() * this_affine, with_objects);

    _can_be_viewport = true;
    other->_can_be_viewport = true;
}

void SPPage::update(SPCtx * /*ctx*/, unsigned int /*flags*/)
{
    // This is manual because this is not an SPItem, but it's own visual identity.
    auto lbl = label();
    char *alt = nullptr;
    if (document->getPageManager().showDefaultLabel()) {
        alt = g_strdup_printf("%d", getPagePosition());
    }
    _canvas_item->update(getDesktopRect(), getDesktopMargin(), getDesktopBleed(), lbl ? lbl : alt, false, document->yaxisdown());
    g_free(alt);
}

/**
 * Write out the page's data into its xml structure.
 */
Inkscape::XML::Node *SPPage::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags)
{
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:view");
    }

    write_viewBox(repr);
    repr->setAttributeOrRemoveIfEmpty("inkscape:margin", margin.write());
    repr->setAttributeOrRemoveIfEmpty("inkscape:bleed", bleed.write());
    repr->setAttributeOrRemoveIfEmpty("inkscape:page-size", _size_label);

    return SPObject::write(xml_doc, repr, flags);
}

void SPPage::setSizeLabel(std::string label)
{
    _size_label = label;
    // This is needed to update the xml
    this->updateRepr();
}

std::string SPPage::getDefaultLabel() const
{
    gchar *format = g_strdup_printf(_("Page %d"), getPagePosition());
    auto ret = std::string(format);
    g_free(format);
    return ret;
}

std::string SPPage::getLabel() const
{
    auto ret = label();
    if (!ret) {
        return getDefaultLabel();
    }
    return std::string(ret);
}

std::string SPPage::getSizeLabel() const
{
    return _size_label;
}

/**
 * Copy non-size attributes from the given page.
 */
void SPPage::copyFrom(SPPage *page)
{
    _size_label = page->_size_label;
    if (auto const &margin_box = page->getMarginBox()) {
        margin.read(margin_box.write(), document->getDocumentScale());
    }
    if (auto const &bleed_box = page->getBleedBox()) {
        bleed.read(bleed_box.write(), document->getDocumentScale());
    }
    updateRepr();
}

void SPPage::set_guides_visible(bool show) {
    _canvas_item->set_guides_visible(show);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
