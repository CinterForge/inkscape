// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Widgets::PageSelector - select and move to pages
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "page-selector.h"

#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <gtkmm/label.h>
#include <gtkmm/signallistitemfactory.h>

#include "desktop.h"
#include "ui/icon-names.h"
#include "ui/pack.h"

namespace Inkscape::UI::Widget {

PageSelector::PageSelector()
    : Gtk::Box(Gtk::Orientation::HORIZONTAL)
{
    set_name("PageSelector");

    _prev_button.set_image_from_icon_name(INKSCAPE_ICON("pan-start"), Gtk::IconSize::NORMAL);
    _prev_button.set_has_frame(false);
    _prev_button.set_tooltip_text(_("Move to previous page"));
    _prev_button.signal_clicked().connect(sigc::mem_fun(*this, &PageSelector::prevPage));

    _next_button.set_image_from_icon_name(INKSCAPE_ICON("pan-end"), Gtk::IconSize::NORMAL);
    _next_button.set_has_frame(false);
    _next_button.set_tooltip_text(_("Move to next page"));
    _next_button.signal_clicked().connect(sigc::mem_fun(*this, &PageSelector::nextPage));

    _dropdown.set_tooltip_text(_("Current page"));
    _dropdown.set_size_request(100, -1);

    auto expr = Gtk::ClosureExpression<Glib::ustring>::create(sigc::mem_fun(*this, &PageSelector::textForItem));
    _dropdown.set_expression(expr);
    _dropdown.set_search_match_mode(Gtk::StringFilter::MatchMode::SUBSTRING);
    _dropdown.set_enable_search(true);

    auto factory = Gtk::SignalListItemFactory::create();
    factory->signal_setup().connect(  sigc::mem_fun(*this, &PageSelector::setupPageItemCB));
    factory->signal_bind().connect(   sigc::mem_fun(*this, &PageSelector::bindPageItemCB));
    factory->signal_unbind().connect( sigc::mem_fun(*this, &PageSelector::unbindPageItemCB));
    _dropdown.set_factory(factory);

    _pageitem_liststore = Gio::ListStore<PageItem>::create();
    _dropdown.set_model(_pageitem_liststore);

    _dropdown.property_selected().signal_changed().connect(sigc::mem_fun(*this, &PageSelector::onDropDownChangedCB));

    UI::pack_start(*this, _prev_button, UI::PackOptions::expand_padding);
    UI::pack_start(*this, _dropdown, UI::PackOptions::expand_widget);
    UI::pack_start(*this, _next_button, UI::PackOptions::expand_padding);
}

PageSelector::~PageSelector()
{
    _doc_replaced_connection.disconnect();
    _selector_changed_connection.disconnect();
    setDocument(nullptr);
}

void PageSelector::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        _doc_replaced_connection.disconnect();
    }

    _desktop = desktop;
    setDocument(_desktop ? _desktop->getDocument() : nullptr);

    if (_desktop) {
        _doc_replaced_connection = _desktop->connectDocumentReplaced(sigc::hide<0>(sigc::mem_fun(*this, &PageSelector::setDocument)));
    }
}

void PageSelector::setDocument(SPDocument *document)
{
    if (_document) {
        _pages_changed_connection.disconnect();
        _page_selected_connection.disconnect();
    }

    _document = document;

    if (_document) {
        auto &page_manager = _document->getPageManager();
        _pages_changed_connection =
            page_manager.connectPagesChanged(sigc::mem_fun(*this, &PageSelector::pagesChanged));
        _page_selected_connection =
            page_manager.connectPageSelected(sigc::mem_fun(*this, &PageSelector::selectionChanged));
        pagesChanged(page_manager.getSelected());
    }
}

void PageSelector::pagesChanged(SPPage *new_page)
{
    auto scoped(_update.block());
    _selector_changed_connection.block();

    auto &page_manager = _document->getPageManager();

    // Destroy all existing pages in the model.
    _pageitem_liststore->splice(0, _pageitem_liststore->get_n_items(), {});

    // Hide myself when there's no pages (single page document)
    this->set_visible(page_manager.hasPages());

    // Add in pages, do not use getResourcelist("page") because the items
    // are not guaranteed to be in node order, they are in first-seen order.
    for (auto &page : page_manager.getPages()) {
        _pageitem_liststore->append(PageItem::create(page));
        if (page == new_page) {
            page_manager.selectPage(page);
        }
    }
    selectionChanged(page_manager.getSelected());

    _selector_changed_connection.unblock();
}

void PageSelector::selectionChanged(SPPage *page)
{
    auto scoped(_update.block());
    _selector_changed_connection.block();

    _next_button.set_sensitive(_document->getPageManager().hasNextPage());
    _prev_button.set_sensitive(_document->getPageManager().hasPrevPage());

    auto item = _dropdown.get_selected_item();
    if (auto pageitem = std::dynamic_pointer_cast<PageItem>(item)) {
        if (pageitem->getPage() != page) {
            for (guint i = 0; i < _pageitem_liststore->get_n_items(); ++i) {
                if (_pageitem_liststore->get_item(i)->getPage() == page) {
                    _dropdown.set_selected(i);
                    break;
                }
            }
        }
    }

    _selector_changed_connection.unblock();
}

void PageSelector::nextPage()
{
    if (_document->getPageManager().selectNextPage()) {
        _document->getPageManager().centerToSelectedPage(_desktop);
    }
}

void PageSelector::prevPage()
{
    if (_document->getPageManager().selectPrevPage()) {
        _document->getPageManager().centerToSelectedPage(_desktop);
    }
}

// Used for search expressions
Glib::ustring PageSelector::textForItem(const Glib::RefPtr<Glib::ObjectBase>& item) const
{
    auto pageitem = std::dynamic_pointer_cast<PageItem>(item);
    auto page = pageitem ? pageitem->getPage() : nullptr;
    if (page && page->getRepr()) {
        int page_num = page->getPagePosition();
        if (auto text = page->label()) {
            return Glib::ustring::compose("%1. %2", page_num, text);
        } else {
            return page->getDefaultLabel();
        }
    } else {
        return "⚠️";
    }
}

// Used for visible label
Glib::ustring PageSelector::markupForItem(const Glib::RefPtr<Glib::ObjectBase>& item) const
{
    auto pageitem = std::dynamic_pointer_cast<PageItem>(item);
    auto page = pageitem ? pageitem->getPage() : nullptr;
    if (page && page->getRepr()) {
        int page_num = page->getPagePosition();
        if (auto text = page->label()) {
            auto escaped_text = Glib::Markup::escape_text(text);
            return Glib::ustring::compose("<span size=\"smaller\"><tt>%1.</tt>%2</span>", page_num, escaped_text);
        } else {
            return Glib::ustring::compose("<span size=\"smaller\"><i>%1</i></span>", page->getDefaultLabel());
        }
    } else {
        return "⚠️";
    }
}

void PageSelector::setupPageItemCB(const Glib::RefPtr<Gtk::ListItem>& list_item) {
    auto label = Gtk::make_managed<Gtk::Label>();
    label->set_halign(Gtk::Align::START);
    label->set_max_width_chars(10);
    label->set_ellipsize(Pango::EllipsizeMode::END);
    list_item->set_child(*label);
}


void PageSelector::renderPageItem(const Glib::RefPtr<Gtk::ListItem>& list_item) {
    auto item = list_item->get_item();
    if (auto label = dynamic_cast<Gtk::Label*>(list_item->get_child())) {
        label->set_markup(markupForItem(item));
    }
}

void PageSelector::bindPageItemCB(const Glib::RefPtr<Gtk::ListItem>& list_item) {
    auto item = list_item->get_item();
    if (auto pageitem = std::dynamic_pointer_cast<PageItem>(item)) {
        renderPageItem(list_item);

        // Re-render whenever item's label property changes.
        auto connection = pageitem->property_label().get_proxy().signal_changed().connect(
            [this, list_item]() {
                renderPageItem(list_item);
            });

        list_item->set_data("label-connection", new sigc::connection(connection),
                            [](gpointer p) { delete static_cast<sigc::connection*>(p);
                            });
    }
}

void PageSelector::unbindPageItemCB(const Glib::RefPtr<Gtk::ListItem>& list_item) {
    if (auto connection = static_cast<sigc::connection*>(list_item->get_data("label-connection"))) {
        connection->disconnect();
    }
    list_item->set_data("label-connection", nullptr);
}

void PageSelector::onDropDownChangedCB() {

    if (_update.pending()) {
        return;
    }

    const unsigned index = _dropdown.get_selected();
    if (index == GTK_INVALID_LIST_POSITION ||
        index >= _pageitem_liststore->get_n_items()) {
        return;
    }
    auto pageitem = _pageitem_liststore->get_item(index);
    auto page = pageitem->getPage();
    if (page && _document->getPageManager().selectPage(page)) {
        _document->getPageManager().centerToSelectedPage(_desktop);
    }
}

} // namespace Inkscape::UI::Widget

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
