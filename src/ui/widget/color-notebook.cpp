// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A notebook with RGB, CMYK, CMS, HSL, and Wheel pages
 *//*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Tomasz Boczkowski <penginsbacon@gmail.com> (c++-sification)
 *
 * Copyright (C) 2001-2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-notebook.h"

#undef SPCS_PREVIEW
#define noDUMP_CHANGE_INFO

#include <glibmm/i18n.h>
#include <gtkmm/stackswitcher.h>

#include "desktop.h"
#include "inkscape.h"
#include "colors/manager.h"
#include "colors/spaces/base.h"
#include "ui/dialog-events.h"
#include "ui/icon-loader.h"
#include "ui/pack.h"
#include "ui/tools/dropper-tool.h"
#include "ui/util.h"
#include "ui/widget/color-entry.h"
#include "ui/widget/color-page.h"
#include "ui/widget/generic/icon-combobox.h"

static constexpr int XPAD = 2;
static constexpr int YPAD = 1;

namespace Inkscape::UI::Widget {

ColorNotebook::ColorNotebook(std::shared_ptr<Colors::ColorSet> colors)
    : _colors(std::move(colors))
{
    set_name("ColorNotebook");

    _initUI();

    //_colors->signal_changed.connect(sigc::mem_fun(*this, &ColorNotebook::_onSelectedColorChanged));

    auto desktop = SP_ACTIVE_DESKTOP;
    _doc_replaced_connection = desktop->connectDocumentReplaced(sigc::hide<0>(sigc::mem_fun(*this, &ColorNotebook::setDocument)));
    setDocument(desktop->getDocument());
}

ColorNotebook::~ColorNotebook()
{
    if (_onetimepick)
        _onetimepick.disconnect();
    _doc_replaced_connection.disconnect();
    setDocument(nullptr);
}

void ColorNotebook::setDocument(SPDocument *document)
{
    _document = document;
    // XXX Watch for new icc spaces here using the profile tracker
}

void ColorNotebook::set_label(const Glib::ustring& label) {
    _label->set_markup(label);
}

void ColorNotebook::_initUI()
{
    guint row = 0;

    _book = Gtk::make_managed<Gtk::Stack>();
    _book->set_transition_type(Gtk::StackTransitionType::CROSSFADE);
    _book->set_transition_duration(130);
    _book->set_vhomogeneous(false);

    // mode selection switcher widget shows all buttons for color mode selection, side by side
    _switcher = Gtk::make_managed<Gtk::StackSwitcher>();
    _switcher->set_stack(*_book);
    // cannot leave it homogeneous - in some themes switcher gets very wide
    // TODO: GTK4: Figure out whether this is still needed / possible to do
    //_switcher->set_homogeneous(false);
    _switcher->set_halign(Gtk::Align::CENTER);
    attach(*_switcher, 0, row++, 2);

    _buttonbox = Gtk::make_managed<Gtk::Box>();

    // combo mode selection is compact and only shows one entry (active)
    _combo = Gtk::make_managed<IconComboBox>();
    // Important: add "regular" class to render non-symbolic color icons;
    // otherwise they will be rendered black&white
    _combo->add_css_class("regular");
    _combo->set_focusable(false);
    _combo->set_tooltip_text(_("Choose style of color selection"));

    // Add all universal (non-document icc profile) color spaces
    for (auto &space : Colors::Manager::get().spaces(Space::Traits::Picker)) {
        _addPageForSpace(space);
    }

    _label = Gtk::make_managed<Gtk::Label>();
    _label->set_visible();
    _label->set_xalign(0);
    _label->set_margin_end(XPAD);
    UI::pack_start(*_buttonbox, *_label, true, true);
    UI::pack_end(*_buttonbox, *_combo, false, false);
    _combo->signal_changed().connect([this](int id){ _setCurrentPage(id, false); });

    _buttonbox->set_margin_start(XPAD);
    _buttonbox->set_margin_end(XPAD);
    _buttonbox->set_margin_top(YPAD);
    _buttonbox->set_margin_bottom(YPAD);
    _buttonbox->set_hexpand();
    _buttonbox->set_valign(Gtk::Align::START);
    attach(*_buttonbox, 0, row, 2);

    row++;

    // book's margins chosen to line up ColorPage's widgets with our widgets
    _book->set_margin_top(3);
    _book->set_margin_bottom(3);
    _book->set_margin_start(2);
    _book->set_margin_end(2);
    _book->set_hexpand();
    _book->set_vexpand(false);
    attach(*_book, 0, row, 2, 1);

    // restore the last active page
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring page_name = prefs->getString("/colorselector/page", "");
    _setCurrentPage(getPageIndex(page_name), true);
    row++;

    _observer = prefs->createObserver("/colorselector/switcher", [this](const Preferences::Entry& new_value) {
        _switcher->set_visible(!new_value.getBool());
        _buttonbox->set_visible(new_value.getBool());
    });
    _observer->call();

    GtkWidget *rgbabox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    auto const rgbabox_box = GTK_BOX(rgbabox);

    /* Create color management icons */
    _colormanaged = sp_get_icon_image("color-management", GTK_ICON_SIZE_NORMAL);
    gtk_widget_set_tooltip_text(_colormanaged, _("Color Managed"));
    gtk_widget_set_sensitive(_colormanaged, false);
    gtk_box_append(rgbabox_box, _colormanaged);

    _outofgamut = sp_get_icon_image("out-of-gamut-icon", GTK_ICON_SIZE_NORMAL);
    gtk_widget_set_tooltip_text(_outofgamut, _("Out of gamut!"));
    gtk_widget_set_sensitive(_outofgamut, false);
    gtk_box_append(rgbabox_box, _outofgamut);

    _toomuchink = sp_get_icon_image("too-much-ink-icon", GTK_ICON_SIZE_NORMAL);
    gtk_widget_set_tooltip_text(_toomuchink, _("Too much ink!"));
    gtk_widget_set_sensitive(_toomuchink, false);
    gtk_box_append(rgbabox_box, _toomuchink);

    /* Color picker */
    _btn_picker = gtk_button_new();
    gtk_button_set_has_frame(GTK_BUTTON(_btn_picker), false);
    gtk_button_set_icon_name(GTK_BUTTON(_btn_picker), "color-picker");
    gtk_widget_set_tooltip_text(_btn_picker, _("Pick colors from image"));
    gtk_box_append(rgbabox_box, _btn_picker);
    g_signal_connect(G_OBJECT(_btn_picker), "clicked", G_CALLBACK(ColorNotebook::_onPickerClicked), this);

    auto const pick_under = Gtk::make_managed<Gtk::Button>();
    pick_under->set_has_frame(false);
    pick_under->set_action_name("app.chameleon-fill");
    pick_under->set_icon_name("color-picker-chameleon");
    pick_under->set_tooltip_text(_("Chameleon Fill"));
    gtk_box_append(rgbabox_box, GTK_WIDGET(pick_under->gobj()));

    /* Create RGB entry and color preview */
    _rgbal = gtk_label_new_with_mnemonic(_("RGB"));
    gtk_widget_set_halign(_rgbal, GTK_ALIGN_END);
    gtk_widget_set_hexpand(_rgbal, TRUE);
    gtk_box_append(rgbabox_box, _rgbal);

    auto const rgba_entry = Gtk::make_managed<ColorEntry>(_colors);
    rgba_entry->set_max_width_chars(8);
    auto const rgba_entry_widget = rgba_entry->Gtk::Widget::gobj();
    sp_dialog_defocus_on_enter(rgba_entry);
    gtk_box_append(rgbabox_box, rgba_entry_widget);
    gtk_label_set_mnemonic_widget(GTK_LABEL(_rgbal), rgba_entry_widget);

    // the "too much ink" icon is initially hidden
    gtk_widget_set_visible(_toomuchink, false);

    gtk_widget_set_margin_start(rgbabox, XPAD);
    gtk_widget_set_margin_end(rgbabox, XPAD);
    gtk_widget_set_margin_top(rgbabox, 8);
    gtk_widget_set_margin_bottom(rgbabox, YPAD);
    attach(*Glib::wrap(rgbabox), 0, row, 2, 1);

    // remember the page we switched to
    _book->property_visible_child_name().signal_changed().connect([this]() {
        // We don't want to remember auto cms selection
        Glib::ustring name = _book->get_visible_child_name();
        if (get_visible() && !name.empty() && name != "CMS") {
            Inkscape::Preferences *prefs = Inkscape::Preferences::get();
            prefs->setString("/colorselector/page", name);
        }
    });

#ifdef SPCS_PREVIEW
    _p = sp_color_preview_new(0xffffffff);
    gtk_widget_set_visible(_p, true);
    attach(*Glib::wrap(_p), 2, 3, row, row + 1, Gtk::FILL, Gtk::FILL, XPAD, YPAD);
#endif
}

void ColorNotebook::_onPickerClicked(GtkWidget * /*widget*/, ColorNotebook *colorbook)
{
    // Set the dropper into a "one click" mode, so it reverts to the previous tool after a click
    if (colorbook->_onetimepick) {
        colorbook->_onetimepick.disconnect();
    }
    else {
        Inkscape::UI::Tools::sp_toggle_dropper(SP_ACTIVE_DESKTOP);
        auto tool = dynamic_cast<Inkscape::UI::Tools::DropperTool *>(SP_ACTIVE_DESKTOP->getTool());
        if (tool) {
            colorbook->_onetimepick = tool->onetimepick_signal.connect([colorbook](Colors::Color const &color) {
                // Set color to color notebook here.
                colorbook->_colors->setAll(color);
            });
            // Close parent popover to release input grab
            UI::close_parent_popover(*colorbook);
        }
    }
}

/*void ColorNotebook::_onSelectedColorChanged() {
    _updateICCButtons();
}*/

/*void ColorNotebook::_updateICCButtons()
{
    if (!_document)
        return;

    // update color management icon
    // XXX auto space = _colors->getConstrainSpace();
    //bool is_cms = space->getDocument();
    //gtk_widget_set_sensitive(_colormanaged, is_cms);
    //gtk_widget_set_sensitive(_toomuchink, _selected_color->isOutOfGamut());
    //gtk_widget_set_sensitive(_outofgamut, _colors->average().isOverInked());


    gtk_widget_set_sensitive(_toomuchink, false);
    gtk_widget_set_sensitive(_outofgamut, false);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    auto page = prefs->getString("/colorselector/page");
    _setCurrentPage(getPageIndex(page), true);
}*/

int ColorNotebook::getPageIndex(const Glib::ustring &name)
{
    return getPageIndex(_book->get_child_by_name(name));
}

int ColorNotebook::getPageIndex(Gtk::Widget *widget)
{
    // Todo: (C++23) Use std::views::enumerate.
    int i = 0;
    for (auto const &child : UI::children(*_book)) {
        if (&child == widget) {
            return i;
        }
        i++;
    }
    return 0;
}

void ColorNotebook::_setCurrentPage(int i, bool sync_combo)
{
    auto page = get_nth_child(*_book, i);

    // page index could be outside the valid range if we manipulate visible color pickers;
    // default to the first page, so we show something
    if (!page) {
        page = _book->get_first_child();
    }

    _book->set_visible_child(*page);
    if (sync_combo) {
        _combo->set_active_by_id(i);
    }
}

void ColorNotebook::_addPageForSpace(std::shared_ptr<Colors::Space::AnySpace> space)
{
    auto selector_widget = Gtk::make_managed<ColorPage>(space, _colors);
    auto mode_name = space->getName();
    _book->add(*selector_widget, mode_name, mode_name);

    auto const n_pages = UI::get_n_children(*_book);
    auto const page_num = n_pages - 1;
    _combo->add_row(space->getIcon(), mode_name, page_num);

    auto prefs = Inkscape::Preferences::get();
    auto obs = prefs->createObserver(space->getPrefsPath() + "visible", [this,selector_widget,page_num](const Preferences::Entry& value) {
        _combo->set_row_visible(page_num, value.getBool());
        selector_widget->set_visible(value.getBool());
    });
    obs->call();
    _visibility_observers.emplace_back(std::move(obs));
}

void ColorNotebook::setCurrentColor(std::shared_ptr<Colors::ColorSet> colors)
{
    auto visible_child = _book->get_visible_child();
    if (auto current_page = dynamic_cast<ColorPage *>(visible_child)) {
        current_page->setCurrentColor(colors);
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
