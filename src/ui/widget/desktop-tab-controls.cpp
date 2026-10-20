// SPDX-License-Identifier: GPL-2.0-or-later

#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <glibmm/main.h>
#include <gtkmm/button.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/viewport.h>

#include "ui/widget/desktop-tab-controls.h"
#include "ui/widget/desktop-tab-row.h"
#include "ui/widget/desktop-widget.h"

namespace Inkscape::UI::Widget {

/// Global list of all DesktopTabControls instances. Its only purpose is to
/// coordinate when tab bars should be shown/hidden.
class DesktopTabControls::Instances
{
public:
    static Instances &get()
    {
        static Instances instance;
        return instance;
    }

    void add(DesktopTabControls *w)
    {
        _instances.push_back(w);
        if (_instances.size() > 1) {
            _updateVisibilityAll();
        }
    }

    void remove(DesktopTabControls *w)
    {
        _instances.erase(std::find(_instances.begin(), _instances.end(), w));
        if (_instances.size() <= 1) {
            _updateVisibilityAll();
        }
    }

    bool forceVisible() const { return _instances.size() > 1; }

private:
    Instances() = default;

    void _updateVisibilityAll()
    {
        for (auto w : _instances) {
            w->_updateVisibility();
        }
    }

    std::vector<DesktopTabControls *> _instances;
};

DesktopTabControls::DesktopTabControls(SPDesktopWidget *desktop_widget)
    : _tabs{std::make_unique<Inkscape::UI::Widget::DesktopTabRow>(desktop_widget)}
    , _menu{Gio::Menu::create()}
{
    set_name("DesktopTabControls");

    _prevbtn = Gtk::make_managed<Gtk::Button>();
    _prevbtn->set_has_frame(false);
    _prevbtn->set_icon_name("pan-left-symbolic");
    _prevbtn->signal_clicked().connect(sigc::mem_fun(*_tabs, &DesktopTabRow::switchToPrevDesktop));
    append(*_prevbtn);

    // Establish the widget hierarchy that css themes will expect - notebook > header > tabs > tab.
    // We set the "notebook" and "header" here, and TabsWidget underneath will be the "tabs".
    // Manually construct these with C, to easily add a css-name
    auto scrollwin_obj = g_object_new(GTK_TYPE_SCROLLED_WINDOW, "css-name", "notebook", NULL);
    _scrollwin = Gtk::manage(Glib::wrap(GTK_SCROLLED_WINDOW(scrollwin_obj)));

    auto viewport_obj = g_object_new(GTK_TYPE_VIEWPORT,
                                     "css-name", "header",
                                     "hadjustment", _scrollwin->get_hadjustment()->gobj(),
                                     "vadjustment", _scrollwin->get_vadjustment()->gobj(),
                                     NULL);
    _viewport = Gtk::manage(Glib::wrap(GTK_VIEWPORT(viewport_obj)));
    _viewport->add_css_class("top");

    _tabs->set_hexpand(true);
    _viewport->set_child(*_tabs);
    _scrollwin->set_child(*_viewport);
    // Use a horizontal scrollbar (so that we can scroll_to()), but keep it hidden
    _scrollwin->set_policy(Gtk::PolicyType::ALWAYS, Gtk::PolicyType::NEVER);
    _scrollwin->get_hscrollbar()->set_visible(false);
    append(*_scrollwin);

    _scrollwin->get_hadjustment()->signal_changed().connect(sigc::mem_fun(*this, &DesktopTabControls::_updateVisibility));

    _nextbtn = Gtk::make_managed<Gtk::Button>();
    _nextbtn->set_has_frame(false);
    _nextbtn->set_icon_name("pan-right-symbolic");
    _nextbtn->signal_clicked().connect(sigc::mem_fun(*_tabs, &DesktopTabRow::switchToNextDesktop));
    append(*_nextbtn);

    auto const menubtn = Gtk::make_managed<Gtk::MenuButton>();
    menubtn->set_has_frame(false);
    menubtn->set_halign(Gtk::Align::END);
    menubtn->set_valign(Gtk::Align::CENTER);
    menubtn->set_menu_model(_menu);
    menubtn->set_name("DesktopTabMenu");
    append(*menubtn);

    auto actiongroup = Gio::SimpleActionGroup::create();
    actiongroup->add_action_with_parameter("switch-to", Glib::VARIANT_TYPE_INT32, [this] (const Glib::VariantBase &val) {
        _tabs->switchToDesktop(tabAtPosition(val.get_dynamic<int>()));
    });
    insert_action_group("tab-controls", actiongroup);

    Instances::get().add(this);
    _updateVisibility();
}

DesktopTabControls::~DesktopTabControls()
{
    Instances::get().remove(this);
}

void DesktopTabControls::addTab(SPDesktop *desktop, int pos)
{
    _tabs->addTab(desktop, pos);
    _rebuildMenu();
    _updateVisibility();
}

void DesktopTabControls::removeTab(SPDesktop *desktop)
{
    _tabs->removeTab(desktop);
    _rebuildMenu();
    _updateVisibility();
}

void DesktopTabControls::switchTab(SPDesktop *desktop)
{
    _tabs->switchTab(desktop);

    auto pos = _tabs->positionOfTab(desktop);
    _prevbtn->set_sensitive(pos > 0);
    _nextbtn->set_sensitive(pos < _tabs->size() - 1);

    // Scroll to tab during idle, because during construction of all the widgets initially,
    // this doesn't work yet until the scroll ranges are settled.
    _on_idle_scroll = Glib::signal_idle().connect([this, pos] {
        if (auto widget = _tabs->widgetAtPosition(pos)) {
            _viewport->scroll_to(*widget);
        }
        return false;
    });
}

void DesktopTabControls::refreshTitle(SPDesktop *desktop)
{
    _tabs->refreshTitle(desktop);
    _rebuildMenu();
}

int DesktopTabControls::positionOfTab(SPDesktop *desktop) const
{
    return _tabs->positionOfTab(desktop);
}

SPDesktop *DesktopTabControls::tabAtPosition(int i) const
{
    return _tabs->tabAtPosition(i);
}

void DesktopTabControls::_updateVisibility()
{
    set_visible(_tabs->size() > 1 || Instances::get().forceVisible());

    auto adj = _scrollwin->get_hadjustment();
    auto scroll_has_hidden_tabs = adj->get_page_size() < adj->get_upper();
    auto show_buttons = _tabs->size() > 1 && scroll_has_hidden_tabs;
    _prevbtn->set_visible(show_buttons);
    _nextbtn->set_visible(show_buttons);
}

void DesktopTabControls::_rebuildMenu()
{
    // The menu uses indices as action parameters, so it's very sensitive to order changing.
    // Thus we rebuild from scratch when tabs are changed.
    _menu->remove_all();
    for (auto pos = 0; pos < _tabs->size(); pos++) {
        auto action = Glib::ustring::compose("tab-controls.switch-to(%1)", pos);
        _menu->append(_tabs->tabName(pos), action);
    }
}

} // namespace Inkscape::UI::Widget
