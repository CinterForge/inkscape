// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UI_TOOLBAR_PENCIL_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_PENCIL_TOOLBAR_H

/**
 * @file
 * Pencil and pen toolbars
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "toolbar.h"
#include "ui/operation-blocker.h"

namespace Gtk {
class Builder;
class Button;
class ToggleButton;
} // namespace Gtk

namespace Inkscape {
namespace UI::Widget {
class DropDownList;
class SpinButton;
} // namespace UI::Widget
namespace XML { class Node; }
} // namespace Inkscape

namespace Inkscape::UI::Toolbar {

class PencilToolbar : public Toolbar
{
public:
    PencilToolbar(bool pencil_mode);
    ~PencilToolbar() override;

    void setDesktop(SPDesktop *desktop) override;

private:
    PencilToolbar(Glib::RefPtr<Gtk::Builder> const &builder, bool pencil_mode);

    using ValueChangedMemFun = void (PencilToolbar::*)();

    bool const _tool_is_pencil;
    std::vector<Gtk::ToggleButton *> _mode_buttons;
    Gtk::Button &_flatten_spiro_bspline_btn;

    Gtk::ToggleButton &_usepressure_btn;
    Gtk::Box &_minpressure_box;
    UI::Widget::SpinButton &_minpressure_item;
    Gtk::Box &_maxpressure_box;
    UI::Widget::SpinButton &_maxpressure_item;
    UI::Widget::DropDownList& _cap_item;
    Gtk::Box& _cap_box;
    UI::Widget::SpinButton &_tolerance_item;
    Gtk::ToggleButton &_simplify_btn;
    Gtk::Button &_flatten_simplify_btn;

    UI::Widget::DropDownList& _shape_item;
    Gtk::Box& _shape_box;
    Gtk::Box &_shapescale_box;
    UI::Widget::SpinButton &_shapescale_item;
    bool _set_shape = false;

    OperationBlocker _blocker;

    void add_powerstroke_cap(Glib::RefPtr<Gtk::Builder> const &builder);
    void add_shape_option(Glib::RefPtr<Gtk::Builder> const &builder);
    void setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value,
                                   ValueChangedMemFun value_changed_mem_fun);
    void hide_extra_widgets(Glib::RefPtr<Gtk::Builder> const &builder);
    void mode_changed(int mode);
    Glib::ustring freehand_tool_name() const;
    void minpressure_value_changed();
    void maxpressure_value_changed();
    void shapewidth_value_changed();
    void use_pencil_pressure();
    void tolerance_value_changed();
    void change_shape(int shape);
    void update_width_value(int shape);
    void change_cap(int cap);
    void simplify_lpe();
    void simplify_flatten();
    template <typename... T> void _flattenLPE();
};

} // namespace Inkscape::UI::Toolbar

#endif // INKSCAPE_UI_TOOLBAR_PENCIL_TOOLBAR_H
