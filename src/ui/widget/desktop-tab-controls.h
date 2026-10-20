// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Inkscape document tabs bar with surrounding controls.
 */

#ifndef INKSCAPE_UI_WIDGET_TAB_CONTROLS_H
#define INKSCAPE_UI_WIDGET_TAB_CONTROLS_H

#include <gtkmm/box.h>

namespace Gio { class Menu; }
namespace Gtk {
class Button;
class ScrolledWindow;
class Viewport;
}

class SPDesktop;
class SPDesktopWidget;

namespace Inkscape::UI::Widget {

class DesktopTabRow;

// Widget that holds document tabs with surrounding controls (like dropdown, left/right buttons).
class DesktopTabControls : public Gtk::Box
{
public:
    DesktopTabControls(SPDesktopWidget *desktop_widget);
    ~DesktopTabControls() override;

    void addTab(SPDesktop *desktop, int pos = -1);
    void removeTab(SPDesktop *desktop);
    void switchTab(SPDesktop *desktop);
    void refreshTitle(SPDesktop *desktop);

    int positionOfTab(SPDesktop *desktop) const;
    SPDesktop *tabAtPosition(int i) const;

private:
    std::unique_ptr<Inkscape::UI::Widget::DesktopTabRow> _tabs;
    Glib::RefPtr<Gio::Menu> _menu;
    Gtk::Button *_prevbtn;
    Gtk::Button *_nextbtn;
    Gtk::Viewport *_viewport;
    Gtk::ScrolledWindow *_scrollwin;
    sigc::scoped_connection _on_idle_scroll;

    class Instances;

    void _updateVisibility();
    void _rebuildMenu();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_TAB_CONTROLS_H
