// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * An abstract gadget that implements a finite cache for a factory.
 */
/*
 * Authors:
 *    PBS <pbs3141@gmail.com>
 *
 * Copyright (C) 2022 PBS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_UTIL_CACHED_MAP_H
#define INKSCAPE_UTIL_CACHED_MAP_H

#include <unordered_map>
#include <deque>
#include <memory>
#include <algorithm>

namespace Inkscape {
namespace Util {

/**
 * A cached_map<Tk, Tv> is designed for use by a factory that takes as input keys of type Tk and
 * produces objects of type std::unique_ptr<Tv> in response. It allows such a factory to remember
 * a finite number of previously constructed objects for later re-use.
 *
 * Upon constructing an object v for key k for the first time, calling
 *
 *     my_ptr = my_cached_map.add(k, std::move(v));
 *
 * will add it to the cache, returning a std::shared_ptr<Tv> by which it can now be accessed.
 *
 * To re-use an object that might be in the cache, use
 *
 *     my_ptr = my_cached_map.lookup(k)
 *
 * When all copies of the shared_ptr my_ptr have expired, the object is marked as unused. However
 * it is not immediately deleted. As further objects are marked as unused, the oldest unused
 * objects are gradually deleted, with their number never exceeding the value max_cache_size.
 *
 * Note that the cache must not be destroyed while any shared pointers to any of its objects are
 * still active. This is in accord with its expected usage; if the factory loads objects from an
 * external library, then it should be safe to destroy the cache just before the library is
 * unloaded, as the objects should no longer be in use at that point anyway.
 */
template <typename Tk, typename Tv, typename Hash = std::hash<Tk>, typename Compare = std::equal_to<Tk>>
class cached_map
{
public:
    /**
     * Construct an empty cached_map.
     *
     * The optional max_cache_size argument specifies the maximum number of unused elements which
     * will be kept in memory.
     */
    cached_map(std::size_t max_cache_size = 32) : max_cache_size(max_cache_size) {}

    /**
     * Given a key and a unique_ptr to a value, inserts them into the map, or discards them if the
     * key is already present.
     *
     * Returns a non-null shared_ptr to the new value in the map corresponding to key.
     */
    auto add(Tk key, std::unique_ptr<Tv> value)
    {
        auto ret = map.emplace(std::move(key), std::move(value));
        return get_view(ret.first->second);
    }

    /**
     * Look up a key in the map.
     *
     * Returns a shared pointer to the corresponding value, or null if the key is not present.
     */
    auto lookup(Tk const &key) -> std::shared_ptr<Tv>
    {
        if (auto it = map.find(key); it != map.end()) {
            return get_view(it->second);
        } else {
            return {};
        }
    }

    void clear()
    {
        unused.clear();
        map.clear();
    }

private:
    struct Item
    {
        std::unique_ptr<Tv> value; // The unique_ptr owning the actual value.
        std::weak_ptr<Tv> view; // A non-owning shared_ptr view that is in use by the outside world.
        Item(decltype(value) value) : value(std::move(value)) {}
    };

    std::size_t const max_cache_size;
    std::unordered_map<Tk, Item, Hash, Compare> map;
    std::deque<Tv*> unused;

    auto get_view(Item &item)
    {
        if (auto view = item.view.lock()) {
            return view;
        } else {
            remove_unused(item.value.get());
            auto new_view = std::shared_ptr<Tv>(item.value.get(), [this] (Tv *value) {
                push_unused(value);
            });
            item.view = new_view;
            return new_view;
        }
    }

    void remove_unused(Tv *value)
    {
        auto it = std::find(unused.begin(), unused.end(), value);
        if (it != unused.end()) {
            unused.erase(it);
        }
    }

    void push_unused(Tv *value)
    {
        unused.emplace_back(value);
        if (unused.size() > max_cache_size) {
            pop_unused();
        }
    }

    void pop_unused()
    {
        auto value = unused.front();
        map.erase(std::find_if(map.begin(), map.end(), [value] (auto const &it) {
            return it.second.value.get() == value;
        }));
        unused.pop_front();
    }
};

} // namespace Util
} // namespace Inkscape

#endif // INKSCAPE_UTIL_CACHED_MAP_H
