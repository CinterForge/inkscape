// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIDGETGROUP_H
#define WIDGETGROUP_H
#include <cassert>

namespace Inkscape::UI::Widget {

class WidgetGroup {
public:
    void add(Gtk::Widget* widget) {
        assert(widget);
        _widgets.push_back(widget);
    }

    void add(const WidgetGroup& group) {
        _widgets.reserve(_widgets.size() + group._widgets.size());
        _widgets.insert(_widgets.end(), group._widgets.begin(), group._widgets.end());
    }

    void set_visible(bool show) {
        for_each([=](auto w) {
            if (w->get_visible() != show) w->set_visible(show);
        });
    }

    void set_sensitive(bool enabled) {
        for_each([=](auto w) {
            if (w->get_sensitive() != enabled) w->set_sensitive(enabled);
        });
    }

    template <typename F>
    void for_each(F&& f) {
        for (auto w : _widgets) {
            f(w);
        }
    }

    Gtk::Widget* operator [] (int index) const {
        return _widgets.at(index);
    }

    bool empty() const { return _widgets.empty(); }

    size_t size() const { return _widgets.size(); }
private:
    // non-owning pointers to widgets
    std::vector<Gtk::Widget*> _widgets;
};

}

#endif //WIDGETGROUP_H
