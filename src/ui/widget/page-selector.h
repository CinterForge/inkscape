// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::UI::Widget::PageSelector - page selector widget
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_WIDGETS_PAGE_SELECTOR
#define SEEN_INKSCAPE_WIDGETS_PAGE_SELECTOR

#include <giomm/liststore.h>
#include <glibmm/property.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/listitem.h>

#include "object/sp-page.h"
#include "ui/operation-blocker.h"

class SPDesktop;
class SPDocument;

namespace Inkscape::UI::Widget {

class PageSelector : public Gtk::Box
{
public:
    PageSelector();
    ~PageSelector() override;

    void setDesktop(SPDesktop *desktop);

private:
    class PageItem : public Glib::Object {
    public:
        static Glib::RefPtr<PageItem> create(SPPage* page)
        {
            return Glib::make_refptr_for_instance<PageItem>(new PageItem(page));
        }
        SPPage* getPage() const { return _page; }
        Glib::Property<Glib::ustring>& property_label() { return _property_label; }

    protected:
        PageItem(SPPage* page)
            : Glib::ObjectBase(typeid(PageItem))
            , _page(page)
            , _property_label(*this, "label", page ? page->getLabel() : " ")
        {
            if (_page) {
                _modified_connection = _page->connectModified(
                    [this](SPObject*, unsigned flags) {
                        if (flags & SP_OBJECT_MODIFIED_FLAG) {
                            _property_label = _page->getLabel();
                        }
                    });
            }
        }

        ~PageItem() override
        {
            _modified_connection.disconnect();
        }

    private:
        SPPage* _page = nullptr;
        Glib::Property<Glib::ustring> _property_label; // Only used to trigger signal on change.
        sigc::connection _modified_connection;
    };

    SPDesktop *_desktop = nullptr;
    SPDocument *_document = nullptr;

    Gtk::DropDown _dropdown;
    Gtk::Button _prev_button;
    Gtk::Button _next_button;

    Glib::RefPtr<Gio::ListStore<PageItem>> _pageitem_liststore;

    OperationBlocker _update;

    sigc::connection _selector_changed_connection;
    sigc::connection _pages_changed_connection;
    sigc::connection _page_selected_connection;
    sigc::connection _doc_replaced_connection;

    void setDocument(SPDocument *document);
    void pagesChanged(SPPage *new_page);
    void selectionChanged(SPPage *page);

    void nextPage();
    void prevPage();

    Glib::ustring markupForItem(const Glib::RefPtr<Glib::ObjectBase>& item) const;
    Glib::ustring textForItem(const Glib::RefPtr<Glib::ObjectBase>& item) const;

    void setupPageItemCB(  const Glib::RefPtr<Gtk::ListItem>& list_item);
    void renderPageItem(   const Glib::RefPtr<Gtk::ListItem>& list_item);
    void bindPageItemCB(   const Glib::RefPtr<Gtk::ListItem>& list_item);
    void unbindPageItemCB( const Glib::RefPtr<Gtk::ListItem>& list_item);
    void onDropDownChangedCB();
};

} // namespace Inkscape::UI::Widget

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
