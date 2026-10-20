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
#ifndef SEEN_SP_COLOR_NOTEBOOK_H
#define SEEN_SP_COLOR_NOTEBOOK_H

#include <gtkmm/grid.h>         // for Grid

#include "colors/color-set.h"
#include "preferences.h"        // for PrefObserver

class SPDocument;

namespace Gtk {
class Box;
class Label;
class Stack;
class StackSwitcher;
} // namespace Gtk

namespace Inkscape::UI::Widget {

class IconComboBox;

class ColorNotebook
    : public Gtk::Grid
{
public:
    ColorNotebook(std::shared_ptr<Colors::ColorSet> color);
    ~ColorNotebook() override;

    void set_label(const Glib::ustring& label);
    void setCurrentColor(std::shared_ptr<Colors::ColorSet> colors);

protected:
    void _initUI();
    void _addPageForSpace(std::shared_ptr<Colors::Space::AnySpace> space);
    void setDocument(SPDocument *document);

    static void _onPickerClicked(GtkWidget *widget, ColorNotebook *colorbook);
    //virtual void _onSelectedColorChanged();
    int getPageIndex(const Glib::ustring &name);
    int getPageIndex(Gtk::Widget *widget);

    //void _updateICCButtons();
    void _setCurrentPage(int i, bool sync_combo);

    std::shared_ptr<Colors::ColorSet> _colors;
    unsigned long _entryId = 0;
    Gtk::Stack* _book = nullptr;
    Gtk::StackSwitcher* _switcher = nullptr;
    Gtk::Box* _buttonbox = nullptr;
    Gtk::Label* _label = nullptr;
    GtkWidget *_rgbal = nullptr; /* RGBA entry */
    GtkWidget *_outofgamut = nullptr;
    GtkWidget *_colormanaged = nullptr;
    GtkWidget *_toomuchink = nullptr;
    GtkWidget *_btn_picker = nullptr;
    GtkWidget *_p = nullptr; /* Color preview */
    sigc::connection _onetimepick;
    IconComboBox* _combo = nullptr;

private:
    PrefObserver _observer;
    std::vector<PrefObserver> _visibility_observers;

    SPDocument *_document = nullptr;
    sigc::connection _doc_replaced_connection;
};

} // namespace Inkscape::UI::Widget

#endif // SEEN_SP_COLOR_NOTEBOOK_H

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

