// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2018 Felipe Corrêa da Silva Sanches, Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_FONT_VARIATIONS_H
#define INKSCAPE_UI_WIDGET_FONT_VARIATIONS_H

#include <gtkmm/box.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/spinbutton.h>

#include "spinbutton.h"
#include "libnrtype/OpenTypeUtil.h"
#include "style.h"
#include "ui/operation-blocker.h"

namespace Inkscape::UI::Widget {

/**
 * A widget for a single axis: Label and Slider
 */
class FontVariationAxis : public Gtk::Box
{
public:
    FontVariationAxis(OTVarAxis const &axis, Glib::ustring tooltip);
    Glib::ustring get_name() { return name; }
    std::string get_tag()    { return tag; }
    Gtk::Label* get_label()  { return label; }
    double get_value()       { return edit->get_value(); }
    int get_precision()      { return precision; }
    Gtk::Scale* get_scale()  { return scale; }
    double get_def()         { return def; }
    SpinButton* get_editbox() { return edit; }
    void set_value(double value);

private:
    // Widgets
    Glib::ustring name;
    Glib::ustring tag; // Four character OpenType axis tag.
    Gtk::Label* label;
    Gtk::Scale* scale;
    SpinButton* edit = nullptr;

    int precision;
    double def = 0.0; // Default value

    // Signals
    sigc::signal<void ()> signal_changed;
};

/**
 * A widget for selecting font variations (OpenType Variations).
 */
class FontVariations : public Gtk::Box
{
public:
    /**
     * Constructor
     */
    FontVariations();

    /**
     * Update GUI.
     */
    void update(const Glib::ustring& font_spec);

#if false
    /**
     * Fill SPCSSAttr based on settings of buttons.
     */
    void fill_css( SPCSSAttr* css );

    /**
     * Get CSS String
     */
    Glib::ustring get_css_string();
#endif

    Glib::ustring get_pango_string(bool include_defaults = false) const;

    /**
     * Let others know that user has changed GUI settings.
     * (Used to enable 'Apply' and 'Default' buttons.)
     */
    sigc::connection connectChanged(sigc::slot<void ()> slot) {
        return _signal_changed.connect(slot);
    }

    // return true if there are some variations present
    bool variations_present() const;

    // provide access to label and spin button size groups
    Glib::RefPtr<Gtk::SizeGroup> get_size_group(int index);

    // construct temp UI with N axes and report its height
    int measure_height(int axis_count);

private:
    void build_ui(const std::vector<OTVarAxis>& axes);

    std::vector<FontVariationAxis*> _axes;
    Glib::RefPtr<Gtk::SizeGroup> _size_group;
    Glib::RefPtr<Gtk::SizeGroup> _size_group_edit;
    sigc::signal<void ()> _signal_changed;
    std::vector<OTVarAxis> _ot_axes;
    OperationBlocker _update;
};


}

#endif // INKSCAPE_UI_WIDGET_FONT_VARIATIONS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
