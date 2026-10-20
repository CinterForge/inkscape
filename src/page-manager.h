// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::PageManager - Multi-Page management.
 *
 * Copyright 2021 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_PAGE_MANAGER_H
#define SEEN_INKSCAPE_PAGE_MANAGER_H

#include <vector>
#include <optional>

#include "document.h"
#include "object/sp-namedview.h"
#include "svg/svg-bool.h"

class SPDesktop;
class SPPage;

namespace Inkscape {
class Selection;
class ObjectSet;
class CanvasPage;
namespace UI {
namespace Dialog {
class DocumentProperties;
}
} // namespace UI
namespace Colors {
class Color;
}

class PageManager
{
public:
    PageManager(SPDocument *document);
    ~PageManager();

    void deactivate();

    static bool move_objects();
    const std::vector<SPPage *> &getPages() const { return _pages; }
    std::vector<SPPage *> getPages(const std::string &pages, bool inverse) const;
    std::vector<SPPage *> getPages(std::set<unsigned int> indexes, bool inverse = false) const;

    void addPage(SPPage *page);
    void removePage(Inkscape::XML::Node *child);
    void reorderPages();

    // Returns None if no page selected
    SPPage *getSelected() const { return _selected_page; }
    SPPage *getPage(int index) const;
    SPPage *getPageAt(Geom::Point pos) const;
    SPPage *getFirstPage() const { return getPage(0); }
    SPPage *getLastPage() const { return getPage(_pages.size() - 1); }
    SPPage *getViewportPage() const;
    std::vector<SPPage *> getPagesFor(SPItem *item, bool contains) const;
    SPPage *getPageFor(SPItem *item, bool contains) const;
    Geom::OptRect getDesktopRect() const;
    bool hasPages() const { return !_pages.empty(); }
    int getPageCount() const { return _pages.size(); }
    int getPageIndex(const SPPage *page) const;
    int getSelectedPageIndex() const;
    Geom::Rect getSelectedPageRect() const;
    Geom::Point getSelectedPageOrigin() const;
    Geom::Translate getSelectedPageOriginAffine() const;
    Geom::Point nextPageLocation() const;
    SPPage *findPageAt(Geom::Point pos) const;

    void enablePages();
    void disablePages();
    void pagesChanged(SPPage *new_page);
    bool selectPage(SPPage *page);
    bool selectPage(SPItem *item, bool contains);
    bool selectPage(int index) { return selectPage(getPage(index)); }
    bool selectNextPage() { return selectPage(getSelectedPageIndex() + 1); }
    bool selectPrevPage() { return selectPage(getSelectedPageIndex() - 1); }
    bool hasNextPage() const { return getSelectedPageIndex() + 1 < _pages.size(); }
    bool hasPrevPage() const { return getSelectedPageIndex() - 1 >= 0; }

    Colors::Color const &getDefaultBackgroundColor() const { return background_color; }

    // TODO: move these functions out of here and into the Canvas or InkscapeWindow
    void zoomToPage(SPDesktop *desktop, SPPage *page, bool width_only = false);
    void zoomToSelectedPage(SPDesktop *desktop, bool width_only = false) { zoomToPage(desktop, _selected_page, width_only); };
    void centerToPage(SPDesktop *desktop, SPPage *page);
    void centerToSelectedPage(SPDesktop *desktop) { centerToPage(desktop, _selected_page); };

    SPPage *newPage();
    SPPage *newPage(double width, double height);
    SPPage *newPage(Geom::Rect rect, bool first_page = false);
    SPPage *newDesktopPage(Geom::Rect rect, bool first_page = false);
    SPPage *newDocumentPage(Geom::Rect rect, bool first_page = false);
    SPPage *duplicatePage();
    void deletePage(SPPage *page, bool contents = false);
    void deletePage(bool contents = false);
    void resizePage(double width, double height);
    void resizePage(SPPage *page, double width, double height);
    void scalePages(Geom::Scale const &scale);
    void rotatePage(int turns);
    void changeOrientation();
    void fitToSelection(ObjectSet *selection, bool add_margins = true);
    void fitToRect(Geom::OptRect box, SPPage *page, bool add_margins = false);

    bool subset(SPAttr key, const gchar *value);
    bool setDefaultAttributes(CanvasPage *item);
    bool showDefaultLabel() const { return label_style == "below"; }
    std::string getSizeLabel(SPPage *page = nullptr);
    std::string getSizeLabel(double width, double height);

    static void enablePages(SPDocument *document) { document->getPageManager().enablePages(); }
    static void disablePages(SPDocument *document) { document->getPageManager().disablePages(); }
    static SPPage *newPage(SPDocument *document) { return document->getPageManager().newPage(); }

    sigc::connection connectPageSelected(const sigc::slot<void (SPPage *)> &slot)
    {
        return _page_selected_signal.connect(slot);
    }
    /**
    * connectPageModified tracks changes to the selected page only.
    */
    sigc::connection connectPageModified(const sigc::slot<void (SPPage *)> &slot)
    {
        return _page_modified_signal.connect(slot);
    }
    sigc::connection connectPagesChanged(const sigc::slot<void (SPPage *)> &slot)
    {
        return _pages_changed_signal.connect(slot);
    }
    /**
    * connectSelectedPageOriginChanged notifies you if the result of getSelectedPageOrigin() may have changed.
    * A lot of conditions feed into this (page location, document-wide preferences, etc), so use this to track it.
    */
    sigc::connection connectSelectedPageOriginChanged(const sigc::slot<void ()> &slot)
    {
        return _selected_page_origin_changed_signal.connect(slot);
    }

    Colors::Color const &getBackgroundColor() const { return background_color; }
    Colors::Color const &getMarginColor() const { return margin_color; }
    Colors::Color const &getBleedColor() const { return bleed_color; }
    Colors::Color const &getBorderColor() const { return border_color; }
    Colors::Color const &getShadowColor() const;

    void movePages(Geom::Affine tr);
    std::vector<SPItem *> getOverlappingItems(SPDesktop *desktop, SPPage *page, bool hidden = true, bool in_bleed = false, bool in_layers = true);

protected:
    friend class Inkscape::UI::Dialog::DocumentProperties;

    // Default settings from sp-namedview
    SVGBool border_show;
    SVGBool border_on_top;
    SVGBool shadow_show;
    SVGBool checkerboard;

    std::string label_style = "default";

private:
    SPDocument *_document;
    SPPage *_selected_page = nullptr;
    std::vector<SPPage *> _pages;

    sigc::signal<void (SPPage *)> _page_selected_signal;
    sigc::signal<void (SPPage *)> _page_modified_signal;
    sigc::signal<void (SPPage *)> _pages_changed_signal;
    sigc::signal<void ()> _selected_page_origin_changed_signal;

    sigc::connection _page_modified_connection;
    sigc::scoped_connection _resources_changed;
    sigc::scoped_connection _doc_modified_conn;

    Colors::Color background_color;
    Colors::Color margin_color;
    Colors::Color bleed_color;
    Colors::Color border_color;
};

} // namespace Inkscape

#endif
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
