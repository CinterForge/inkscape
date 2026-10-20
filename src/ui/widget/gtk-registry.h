// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INKSCAPE_UI_WIDGET_GTK_REGISTRY_H
#define SEEN_INKSCAPE_UI_WIDGET_GTK_REGISTRY_H

#include <glibmm/wrap.h>
#include <gtkmm/widget.h>

namespace Gtk {
class Builder;
}

namespace Inkscape::UI::Widget {

// Add all custom widgets to the Gtk Builder registry so they can be
// Used from glade/ui xml files.
void register_all();

// helper class to handle Glib type registration details for custom widgets
template<class T, class Base> class BuildableWidget : public Base {
    static GType gtype;

    static Glib::ObjectBase* wrap_new(GObject* o) {
        auto obj = new T((typename Base::BaseObjectType*)o);
        return Gtk::manage(obj);
    }

protected:
    BuildableWidget() = default;
    BuildableWidget(typename Base::BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>&) : Base(cobject) {}

public:
    static void register_type() {
        if (gtype) return;

        auto dummy = T();
        gtype = G_OBJECT_TYPE(dummy.Gtk::Widget::gobj());
        Glib::wrap_register(gtype, wrap_new);
    }

    static GType get_gtype() {
        return gtype;
    }
};

template<class T, class Base>
GType BuildableWidget<T, Base>::gtype = 0;

} // namespace Dialog::UI::Widget

#endif // SEEN_INKSCAPE_UI_WIDGET_GTK_REGISTRY_H

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
