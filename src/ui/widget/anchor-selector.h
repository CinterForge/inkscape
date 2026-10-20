// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * anchor-selector.h
 *
 *  Created on: Mar 22, 2012
 *      Author: denis
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef ANCHOR_SELECTOR_H
#define ANCHOR_SELECTOR_H

#include <array>
#include <gtkmm/box.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/grid.h>

namespace Glib {
class ustring;
} // namespace Glib

namespace Inkscape::UI::Widget {

class AnchorSelector final : public Gtk::Box
{
public:
    AnchorSelector();

    int getHorizontalAlignment() const { return _selection % 3; }
    int getVerticalAlignment  () const { return _selection / 3; }

    sigc::connection connectSelectionChanged(sigc::slot<void ()>);

    void setAlignment(int horizontal, int vertical);

private:
    std::array<Gtk::ToggleButton, 9> _buttons;
    int                _selection;
    Gtk::Grid          _container;

    sigc::signal<void ()> _selectionChanged;

    void setupButton(const Glib::ustring &icon, Gtk::ToggleButton &button);
    void btn_activated(int index);
};

} // namespace Inkscape::UI::Widget

#endif // ANCHOR_SELECTOR_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
