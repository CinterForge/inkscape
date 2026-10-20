// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_PATTERN_EDITOR_H
#define INKSCAPE_PATTERN_EDITOR_H

#include <giomm/liststore.h>

// Simplistic filtered list store implementation
// It's a GIO ListStore plus custom filter function to control visibility of items

namespace Inkscape {

template<class T> class FilteredStore {
public:
    FilteredStore() {
        _store = Gio::ListStore<T>::create();
    }

    typedef Glib::RefPtr<T> TPtr;

    bool assign(const std::vector<TPtr>& items) {
        if (items == _items) return false; // not changed

        _items = items;
        apply_filter();
        return true; // store updated
    }

    void refresh() {
        apply_filter(true);
    }

    const std::vector<TPtr>& get_items() const {
        return _items;
    }

    void set_filter(const std::function<bool (const TPtr&)>& filter_callback) {
        _filter_callback = filter_callback;
    }

    void apply_filter(bool force_refresh = false) {
        // if there's a filter, run it
        std::vector<TPtr> visible;
        if (_filter_callback) {
            std::copy_if(_items.cbegin(), _items.cend(),
			 std::back_inserter(visible), std::ref(_filter_callback));
        }
        auto const &items = _filter_callback ? visible : _items;

        if (force_refresh) {
            update_store(items);
            return;
        }

        // compare store content with visible list items to avoid needless updates
        const auto n = _store->get_n_items();
        bool same = false;
        if (n == items.size()) {
            same = true;
            // compare each item
            for (int i = 0; i < n; ++i) {
                if (items[i] != _store->get_item(i)) {
                    same = false;
                    break;
                }
            }
        }

        if (same) return; // nothing to do

        update_store(items);
    }

    Glib::RefPtr<Gio::ListStore<T>> get_store() {
        return _store;
    }

private:
    void update_store(const std::vector<TPtr>& items) {
        _store->freeze_notify();
        _store->remove_all();
        for (auto&& t : items) {
            _store->append(t);
        }
        _store->thaw_notify();
    }

    Glib::RefPtr<Gio::ListStore<T>> _store;
    std::function<bool (const TPtr&)> _filter_callback;
    std::vector<TPtr> _items;
    std::vector<TPtr> _visible;
};

} // namespace Inkscape

#endif // INKSCAPE_PATTERN_EDITOR_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim:filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99:
