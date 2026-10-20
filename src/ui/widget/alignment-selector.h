// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * anchor-selector.h
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef ANCHOR_SELECTOR_H
#define ANCHOR_SELECTOR_H

#include <array>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/grid.h>

namespace Glib {
class ustring;
} // namespace Glib

namespace Inkscape::UI::Widget {

class AlignmentSelector final : public Gtk::Box
{
public:
    AlignmentSelector();

    sigc::connection connectAlignmentClicked(sigc::slot<void (int)>);

private:
    std::array<Gtk::Button, 9> _buttons;
    Gtk::Grid   _container;

    sigc::signal<void (int)> _alignmentClicked;

    void setupButton(const Glib::ustring &icon, Gtk::Button &button);
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
