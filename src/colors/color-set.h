// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A set of colors which can be modified together used for color pickers
 *//*
 * Copyright (C) 2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_COLOR_SET
#define SEEN_COLORS_COLOR_SET

#include <memory>
#include <optional>
#include <sigc++/signal.h>
#include <vector>

#include "colors/color.h"
#include <sigc++/scoped_connection.h>

namespace Inkscape::Colors {
class Color;
namespace Space {
class AnySpace;
class Components;
class Component;
} // namespace Space

using IdColors = std::vector<std::pair<std::string, Color>>;

class ColorSet
{
public:
    ColorSet(std::shared_ptr<Space::AnySpace> space = {}, std::optional<bool> alpha = {});

    // By default, disallow copy constructor
    ColorSet(ColorSet const &obj) = delete;

    IdColors::const_iterator begin() const { return std::begin(_colors); }
    IdColors::const_iterator end() const { return std::end(_colors); }
    IdColors::iterator begin() { return std::begin(_colors); }
    IdColors::iterator end() { return std::end(_colors); }

    // Signals allow the set to be tied into various interfaces
    bool isBlocked() const { return _blocked; }
    bool isGrabbed() const { return _grabbed; }

    sigc::signal<void()> signal_grabbed;
    sigc::signal<void()> signal_released;
    sigc::signal<void()> signal_changed;
    sigc::signal<void()> signal_cleared;

    void grab();
    void release();
    void block() { _blocked = true; }
    void unblock() { _blocked = false; }

    // Information about the color set's meta data
    Space::Components const &getComponents() const;
    std::shared_ptr<Space::AnySpace> const getSpaceConstraint() const { return _space_constraint; }
    std::optional<bool> const getAlphaConstraint() const { return _alpha_constraint; }

    // Set and get a single color (i.e. not a list of colors)
    bool set(Color const &color);
    std::optional<Color> get() const;
    void clear();

    // Control the list of colors by id
    std::optional<Color> get(std::string const &id) const;
    bool set(std::string id, Color const &color);

    // Change the colors in some way and send signals
    unsigned setAll(ColorSet const &other);
    unsigned setAll(Color const &other);
    unsigned setAll(Space::Component const &c, double value);
    std::vector<double> getAll(Space::Component const &c) const;

    // Set and get specific color components
    void setAverage(Space::Component const &c, double value);
    double getAverage(Space::Component const &c) const;
    Color getAverage() const;

    // Ask for information about the whole set of colors
    unsigned size() const;

    bool isEmpty() const { return _colors.empty(); }
    bool isSame() const;
    std::shared_ptr<Space::AnySpace> getBestSpace() const;

    bool isValid(const Space::Component& component) const;
private:
    bool _set(std::string const &id, Color const &color);
    void colors_changed();
    void colors_cleared();

    IdColors _colors;

    // Utility container for very fast id lookup
    std::unordered_map<std::string, size_t> _colors_index;

    // Constraints can only be set up at construction so are immutable.
    std::shared_ptr<Space::AnySpace> const _space_constraint;
    std::optional<bool> const _alpha_constraint;

    // States which are set during the lifetime of the set
    bool _grabbed = false;
    bool _blocked = false;
};

} // namespace Inkscape::Colors

#endif
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
