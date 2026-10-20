// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEEN_INKSCAPE_UI_WIDGET_ICONCOMBOBOX
#define SEEN_INKSCAPE_UI_WIDGET_ICONCOMBOBOX

#include <giomm/liststore.h>
#include <gtkmm/boolfilter.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>
#include <2geom/point.h>

namespace Gtk {
class Builder;
}

namespace Inkscape::UI::Widget {

class IconComboBox : public Gtk::DropDown
{
public:
    // Items in a store used by IconComboBox
    struct ListItem;
    // What to show in a closed dropdown:
    enum HeaderType { ImageLabel, ImageOnly, LabelOnly };
    // Custom drop down list with icons/images and labels
    IconComboBox(bool use_icons = true, HeaderType header = ImageLabel);
    // Ditto, but uses supplied store
    IconComboBox(Glib::RefPtr<Gio::ListStore<ListItem>> store, bool use_icons = true, HeaderType header = ImageLabel);
    IconComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, Glib::RefPtr<Gio::ListStore<ListItem>> store = {}, bool use_icons = true, HeaderType header = ImageLabel);
    ~IconComboBox() override;

    // helper methods to populate store
    void add_row(Glib::ustring const &icon_name, Glib::ustring const &label, int id);
    void add_row(const Glib::ustring& icon_name, const Glib::ustring& full_name, const Glib::ustring& short_name, int id);
    void add_row(Cairo::RefPtr<Cairo::Surface> image, const Glib::ustring& label, int id);
    // Set selected (aka active) item
    void set_active_by_id(int id);
    // Show/hide an item in store
    void set_row_visible(int id, bool visible = true, bool refilter_items = true);
    // Get selected item's ID
    int get_active_row_id() const;
    std::shared_ptr<IconComboBox::ListItem> current_item();
    // return signal to selection change event; it reports ID of current item
    sigc::signal<void (int)>& signal_changed();
    // get default image size
    static int get_default_image_size() { return 16; }
    // establish logical image size for all pictures in the list
    void set_image_size(Geom::Point size) { _image_size = size; }
    Geom::Point get_image_size() const { return _image_size; }
    // add frame
    void set_has_frame(bool frame);
    // apply filter to show items after adding them
    void refilter();

private:
    void construct(Glib::RefPtr<Gio::ListStore<ListItem>> store, bool use_icons, HeaderType header);
    bool is_item_visible(const Glib::RefPtr<Glib::ObjectBase>& item) const;
    std::pair<std::shared_ptr<IconComboBox::ListItem>, int> find_by_id(int id, bool visible_only);

    Geom::Point _image_size{16, 16};
    Glib::RefPtr<Gtk::SignalListItemFactory> _factory;
    Glib::RefPtr<Gtk::SignalListItemFactory> _compact_factory;
    Glib::RefPtr<Gtk::FilterListModel> _filtered_model;
    Glib::RefPtr<Gtk::SingleSelection> _selection_model;
    Glib::RefPtr<Gtk::BoolFilter> _filter;
    Glib::RefPtr<Gio::ListStore<IconComboBox::ListItem>> _store;
    sigc::signal<void (int)> _signal_current_changed = sigc::signal<void (int)>();
};

struct IconComboBox::ListItem : Glib::Object {
    int id;                     // user-defined ID or index
    Glib::ustring label;        // label to show after icon/image
    Glib::ustring short_name;   // short name to use in a header (closed dropdown) when header type was 'LabelOnly'
    Glib::ustring icon;         // icon's name to load if icons have been enabled
    Glib::RefPtr<Gdk::Texture> image;  // image to present instead of icon if icons are disabled
    bool is_visible = true;     // true if item is to be visible/included
    std::string uid;            // user-defined unique ID (optional)
    void* data = nullptr;       // user-defined data (optional)

    static Glib::RefPtr<ListItem> create(
        int id,
        Glib::ustring label,
        Glib::RefPtr<Gdk::Texture> image
    ) {
        return create(id, label, {}, {}, image);
    }
    static Glib::RefPtr<ListItem> create(
        int id,
        Glib::ustring label,
        Glib::ustring short_name,
        Glib::ustring icon,
        Glib::RefPtr<Gdk::Texture> image
    ) {
        auto item = Glib::make_refptr_for_instance<ListItem>(new ListItem());
        item->id = id;
        item->label = label;
        item->short_name = short_name;
        item->icon = icon;
        item->image = image;
        return item;
    }
private:
    ListItem() {}
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_INKSCAPE_UI_WIDGET_ICONCOMBOBOX

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
