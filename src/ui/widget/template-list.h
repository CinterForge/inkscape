// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef WIDGET_TEMPLATE_LIST_H
#define WIDGET_TEMPLATE_LIST_H

#include <giomm/liststore.h>
#include <gtkmm/boolfilter.h>
#include <gtkmm/gridview.h>
#include <gtkmm/stack.h>

#include "extension/template.h"
#include "ui/iconview-item-factory.h"

namespace Gdk {
class Pixbuf;
} // namespace Gdk

namespace Gtk {
class Builder;
class IconView;
class ListStore;
} // namespace Gtk

class SPDocument;

namespace Inkscape::UI::Widget {

class TemplateList final : public Gtk::Stack
{
public:
    TemplateList() = default;
    TemplateList(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    ~TemplateList() override = default;

    enum AddPage { All, Custom };
    void init(Extension::TemplateShow mode, AddPage add_page, bool allow_unselect = false);
    void reset_selection(Gtk::Widget* current_page = nullptr);
    bool has_selected_preset();
    bool has_selected_new_template();
    std::shared_ptr<Extension::TemplatePreset> get_selected_preset(Gtk::Widget* current_page = nullptr);
    SPDocument *new_document(Gtk::Widget* current_page = nullptr);
    void show_page(const Glib::ustring& name);
    const std::vector<Glib::ustring>& get_categories() const { return _categories; }
    Glib::ustring get_category_label(Glib::ustring const &cat) const;
    void filter(Glib::ustring search);
    void focus();

    sigc::connection connectItemSelected(const sigc::slot<void (int)> &slot) { return _item_selected_signal.connect(slot); }
    sigc::connection connectItemActivated(const sigc::slot<void ()> &slot) { return _item_activated_signal.connect(slot); }
    sigc::signal<void(const Glib::ustring&)> signal_switch_page();

private:
    struct TemplateItem;
    Glib::RefPtr<TemplateItem> get_selected_item(Gtk::Widget* current_page = nullptr);
    Glib::RefPtr<Gio::ListStore<TemplateItem>> generate_category(std::string const &label, bool allow_unselect);
    Cairo::RefPtr<Cairo::ImageSurface> icon_to_pixbuf(std::string const &name, int scale);
    Gtk::GridView *get_iconview(Gtk::Widget *widget);
    bool is_item_visible(const Glib::RefPtr<Glib::ObjectBase>& item, const Glib::ustring& search) const;
    void refilter(Glib::ustring search);

    sigc::signal<void (int)> _item_selected_signal;
    sigc::signal<void ()> _item_activated_signal;
    std::vector<std::unique_ptr<IconViewItemFactory>> _factory;
    sigc::signal<void (const Glib::ustring&)> _signal_switch_page;
    std::vector<Glib::ustring> _categories;
    Glib::RefPtr<Gtk::BoolFilter> _filter;
    Glib::ustring _search_term;
};

} // namespace Inkscape::UI::Widget

#endif // WIDGET_TEMPLATE_LIST_H

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
