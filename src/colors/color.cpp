
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/color.h"

#include <cassert>
#include <cmath>
#include <utility>

#include "colors/manager.h"
#include "spaces/base.h"
#include "spaces/components.h"

namespace Inkscape::Colors {
namespace {

template <typename T>
static decltype(auto) assert_nonnull(T &&t)
{
    assert(t);
    return std::forward<T>(t);
}

} // namespace

/**
 * Create a color, given the type of the space and the values to store.
 *
 * @arg space_type - The type of the color space these values are in.
 * @arg values - A vector of numbers usually between 0.0 and 1.0 per channel
 *               which will be moved to the new Color object.
 */
Color::Color(Space::Type space_type, std::vector<double> values)
    : Color(assert_nonnull(Manager::get().find(space_type)), std::move(values))
{}

/**
 * Compatibility layer for making blind RGB colors
 */
Color::Color(uint32_t rgba, bool opacity)
    : Color(Space::Type::RGB, rgba_to_values(rgba, opacity))
{}

/**
 * Construct a color in the given color space.
 *
 * @arg space - The color space these channel values exist in.
 * @arg colors - Each channel in the color space must have a value between 0.0 and 1.0
 *               an extra value may be appended to indicate the opacity to support CSS
 *               formatted opacity parsing but is not expected to be written by Inkscape
 *               when being generated.
 */
Color::Color(std::shared_ptr<Space::AnySpace> space, std::vector<double> colors)
    : _values(std::move(colors))
    , _space(std::move(space))
{
    assert(_space->isValidData(_values));
}

/**
 * Return true if the two colors are the same.
 *
 * The color space AND the values must be the same. But the name doesn't not
 *  have to be the same in both colors.
 */
bool Color::operator==(Color const &other) const
{
    // TODO: Adjust epsilon value to ignore roundtrip conversion rounding errors,
    // but still keep high precision of color comparison.
    //
    // I arrived at this value empirically. If it is too big, then sometimes changes (by the user) are not picked up,
    // and no refresh happens. If it is too small, then roundtrip conversions will also trigger changes, while they ideally should not.
    // It's all a bit fragile.
    return _space == other._space && _isnear(other._values, 0.00001);
}

/**
 * Get a single channel from this color.
 */
double Color::get(unsigned index) const
{
    assert(index <= getOpacityChannel());
    if (index < _values.size()) {
        return _values[index];
    } else {
        return 1.0;
    }
}

/**
 * Format the color as a css string and return it.
 *
 * @arg opacity - If set to false the opacity will be ignored, even if present.
 *
 * Note: Choose the color space for your color carefully before printing. If you
 * are outputting to a CSS version that only supports RGB hex codes, then convert
 * the color to that color space before printing.
 */
std::string Color::toString(bool opacity) const
{
    return _space->toString(_values, opacity);
}

/**
 * Return an sRGB conversion of the color in RGBA int32 format.
 *
 * @args opacity - optional opacity to be mixed into any existing
 *                 opacity in this color.
 */
uint32_t Color::toRGBA(double opacity) const
{
    return _space->toRGBA(_values, opacity);
}

/**
 * Return the RGBA int32 as an ARGB format number.
 */
uint32_t Color::toARGB(double opacity) const
{
    auto value = toRGBA(opacity);
    return (value >> 8) | ((value & 0xff) << 24);
}

/**
 * Return the RGBA int32 as an ABGR format color.
 */
uint32_t Color::toABGR(double opacity) const
{
    auto value = toRGBA(opacity);
    return (value << 24) | ((value << 8) & 0x00ff0000) | ((value >> 8) & 0x0000ff00) | (value >> 24);
}

/**
 * Convert to the same format as the other color.
 *
 * @arg other - Another color to copy the space and opacity from.
 */
bool Color::convert(Color const &other)
{
    if (convert(other._space)) {
        enableOpacity(other.hasOpacity());
        return true;
    }
    return false;
}

/**
 * Convert this color into a different color space.
 *
 * @arg to_space - The target space to convert the color values to
 */
bool Color::convert(std::shared_ptr<Space::AnySpace> to_space)
{
    if (!to_space || !to_space->hasValidCmsProfile()) {
        return false;
    }

    if (_space != to_space) {
        _space->convert(_values, to_space);
        _space = std::move(to_space);
        assert(_space->isValidData(_values));
    }
    _name = "";

    return true;
}

/**
 * Convert this color into the first matched color space of the given type.
 */
bool Color::convert(Space::Type type)
{
    if (auto space = Manager::get().find(type)) {
        return convert(space);
    }
    return false;
}

/**
 * Return a copy of this color converted to the same format as the other color.
 */
std::optional<Color> Color::converted(Color const &other) const
{
    Color copy = *this;
    if (copy.convert(other)) {
        return copy;
    }
    return {};
}

/**
 * Convert a copy of this color into a different color space.
 */
std::optional<Color> Color::converted(std::shared_ptr<Space::AnySpace> to_space) const
{
    Color copy = *this;
    if (copy.convert(std::move(to_space))) {
        return copy;
    }
    return {};
}

/**
 * Convert a copy of this color into the first matching color space type.
 *
 * if the space is not available in this color manager, an empty color is returned.
 */
std::optional<Color> Color::converted(Space::Type type) const
{
    Color copy = *this;
    if (copy.convert(type)) {
        return copy;
    }
    return {};
}

/**
 * Set the channels directly without checking if the space is correct.
 *
 * @arg values - A vector of doubles, one value between 0.0 and 1.0 for
 *               each channel. An extra channel can be included for opacity.
 */
void Color::setValues(std::vector<double> values)
{
    _name = "";
    _values = std::move(values);
    assert(_space->isValidData(_values));
}

/**
 * Set this color to the values from another color.
 *
 * @arg other - The other color which is a source for the values.
 *              if the other color is from an unknown color space which
 *              has never been seen before, it will cause an error.
 * @arg keep_space - If true, this color's color-space will stay the same
 *                   and the new values will be converted. This includes
 *                   discarding opacity if this color didn't have opacity.
 *
 * @returns true if the new value if different from the old value
 */
bool Color::set(Color const &other, bool keep_space)
{
    if (keep_space) {
        auto prev_space = _space;
        auto prev_values = _values;
        bool prev_opacity = hasOpacity();

        if (set(other, false)) {
            // Convert back to the previous space if needed.
            convert(prev_space);
            enableOpacity(prev_opacity);
            // Return true if the converted result is different
            return !_isnear(prev_values);
        }
    } else if (*this != other) {
        _space = other._space;
        _values = other._values;
        _name = other._name;
        return true;
    }
    return false;
}

/**
 * Set this color by parsing the given string. If there's a parser error
 * it will not change the existing color.
 *
 * @arg parsable - A string with a typical css color value.
 * @arg keep_space - If true, the existing space will stay the same (see previous Color::set)
 *
 * @returns true if the new value if different from the old value
 */
bool Color::set(std::string const &parsable, bool keep_space)
{
    if (auto color = Color::parse(parsable)) {
        return set(*color, keep_space);
    }
    return false;
}

/**
 * Returns true if the values are near to the other values
 */
bool Color::_isnear(std::vector<double> const &other, double epsilon) const
{
    bool is_near = _values.size() == other.size();
    for (size_t i = 0; is_near && i < _values.size(); i++) {
        is_near &= std::abs(_values[i] - other[i]) < epsilon;
    }
    return is_near;
}

/**
 * Create an optional color if value is valid.
 */
std::optional<Color> Color::parse(char const *value)
{
    if (!value) {
        return {};
    }
    return parse(std::string(value));
}

/**
 * Create an optional color, if possible, from the given string.
 */
std::optional<Color> Color::parse(std::string const &value)
{
    Space::Type space_type;
    std::string cms_name;
    std::vector<double> values;
    std::vector<double> fallback;
    if (Parsers::get().parse(value, space_type, cms_name, values, fallback)) {
        return ifValid(space_type, std::move(values));
    }
    // Couldn't be parsed as a color at all
    return {};
}

/**
 * Construct a color from the space type and values, if the values are valid
 */
std::optional<Color> Color::ifValid(Space::Type space_type, std::vector<double> values)
{
    if (auto space = Manager::get().find(space_type)) {
        if (space->isValidData(values)) {
            return std::make_optional<Color>(std::move(space), std::move(values));
        }
    }
    // Invalid color data, return empty optional
    return {};
}

/**
 * Set a specific channel in the color.
 *
 * @arg index - The channel/component index to set
 * @arg value - The new value to set it to.
 *
 * @returns true if the new value if different from the old value
 */
bool Color::set(unsigned int index, double value)
{
    assert(index <= getOpacityChannel());
    if (index == _values.size()) {
        _values.push_back(1.0);
    }
    auto const changed = std::abs(_values[index] - value) >= 0.001;
    _values[index] = value;
    return changed;
}

/**
 * Set this color from an RGBA unsigned int.
 *
 * @arg rgba - The RGBA color encoded as a single unsigned integer, 8bpc
 * @arg opacity - True if the opacity (Alpha) should be stored too.
 *
 * @returns true if the new value if different from the old value
 */
bool Color::set(uint32_t rgba, bool opacity)
{
    if (*_space != Space::Type::RGB) {
        // Ensure we are in RGB
        _space = assert_nonnull(Manager::get().find(Space::Type::RGB));
    } else if (rgba == toRGBA(opacity)) {
        return false; // nothing to do.
    }
    _name = "";
    _values = std::move(rgba_to_values(rgba, opacity));
    return true;
}

/**
 * Enables or disables the opacity channel.
 */
void Color::enableOpacity(bool enable)
{
    auto const has_opacity = hasOpacity();
    if (enable && !has_opacity) {
        _values.push_back(1.0);
    } else if (!enable && has_opacity) {
        _values.pop_back();
    }
}

/**
 * Returns true if there is an opacity channel in this color.
 */
bool Color::hasOpacity() const
{
    return _values.size() > getOpacityChannel();
}

/**
 * Get the opacity in this color, if it's stored. Returns 1.0 if no
 * opacity exists in this color or 0.0 if this color is empty.
 */
double Color::getOpacity() const
{
    return hasOpacity() ? _values.back() : 1.0;
}

/**
 * Get the opacity, and remove it from this color. This is useful when setting a color
 * into an svg css property that has it's own opacity property but you aren't ready to
 * create a string (see toString(false) for that use)
 */
double Color::stealOpacity()
{
    auto ret = getOpacity();
    enableOpacity(false);
    return ret;
}

/**
 * Get the opacity channel index
 */
unsigned int Color::getOpacityChannel() const
{
    return _space->getComponentCount();
}

/**
 * Return the pin number (pow2) of the channel index to pin
 * that channel in a mutation.
 */
unsigned int Color::getPin(unsigned int channel) const
{
    return 1 << channel;
}

/**
 * Set the opacity of this color object.
 */
bool Color::setOpacity(double opacity)
{
    if (hasOpacity()) {
        if (opacity == _values.back()) {
            return false;
        }
        _values.back() = opacity;
    } else {
        _values.emplace_back(opacity);
    }
    return true;
}

/**
 * Make a copy and add the given opacity on top.
 */
Color Color::withOpacity(double opacity) const
{
    Color copy = *this;
    copy.addOpacity(opacity);
    return copy;
}

/**
 * Make sure the values for this color are within acceptable ranges.
 */
void Color::normalize()
{
    for (auto const &comp : _space->getComponents(hasOpacity())) {
        _values[comp.index] = comp.normalize(_values[comp.index]);
    }
}

/**
 * Return a normalized copy of this color so the values are within acceptable ranges.
 */
Color Color::normalized() const
{
    Color copy = *this;
    copy.normalize();
    return copy;
}

/**
 * Invert the color for each channel.
 *
 * @arg pin - Bit field, which channels should not change if not specified
 *            the opacity pin is used as this is the most reasonable default.
 */
void Color::invert(unsigned int pin)
{
    for (unsigned int i = 0; i < _values.size(); i++) {
        if (pin & (1 << i)) {
            continue;
        }
        _values[i] = 1.0 - _values[i];
    }
}

/**
 * Jitter the color for each channel.
 *
 @ @arg force - The amount of jitter to add to each channel.
 * @arg pin   - Bit field, which channels should not change (see invert).
 */
void Color::jitter(double force, unsigned int pin)
{
    for (unsigned int i = 0; i < _values.size(); i++) {
        if (pin & (1 << i)) {
            continue;
        }
        // Random number between -0.5 and 0.5 times the force.
        double r = (static_cast<double>(std::rand()) / RAND_MAX - 0.5);
        _values[i] += r * force;
    }
    normalize();
}

/**
 * Put the other color on top of this color, mixing the two according to the alpha.
 *
 * @arg other - The other color to compose with this one.
 */
void Color::compose(Color const &other)
{
    auto alpha = other.getOpacity();
    _color_mutate_inplace(other, getPin(getOpacityChannel()),
                          [alpha](auto &value, auto otherValue) { value = value * (1.0 - alpha) + otherValue * alpha; });
    setOpacity(1.0 - (1.0 - getOpacity()) * (1.0 - alpha));
}

/**
 * Return the composition of this color, plus the other color on top.
 *
 * @arg other - The other color to compose with this one.
 */
Color Color::composed(Color const &other) const
{
    Color copy = *this;
    copy.compose(other);
    return copy;
}

/*
 * Modify this color to be the average between two colors, modifying the first.
 *
 * @arg other  - The other color to average with
 * @arg pos    - The position (i.e. t) between the two colors.
 * @pin pin    - Bit field, which channels should not change (see invert)
 */
void Color::average(Color const &other, double pos, unsigned int pin)
{
    _color_mutate_inplace(other, pin,
                          [pos](auto &value, auto otherValue) { value = value * (1 - pos) + otherValue * pos; });
}

/**
 * Return the average between this and another color.
 *
 * @arg other - The other color to average with
 * @arg pos - The weighting to give each color
 */
Color Color::averaged(Color const &other, double pos) const
{
    Color copy = *this;
    copy.average(other, pos);
    return copy;
}

/**
 * Get the mean square difference between this color and another.
 */
double Color::difference(Color const &other) const
{
    double ret = 0.0;
    if (auto copy = other.converted(*this)) {
        for (unsigned int i = 0; i < _values.size(); i++) {
            ret += std::pow(_values[i] - (*copy)[i], 2);
        }
    }
    return ret;
}

/**
 * Find out if a color is a close match to another color of the same type.
 *
 * @returns true if the colors are the same structure (space, opacity)
 *          and have values which are no more than epison apart.
 */
bool Color::isClose(Color const &other, double epsilon) const
{
    bool match = _space == other._space && _values.size() == other._values.size();
    for (unsigned int i = 0; match && i < _values.size(); i++) {
        match &= (std::fabs(_values[i] - other._values[i]) < epsilon);
    }
    return match;
}

/**
 * Find out if a color is similar to another color, converting it
 * first if it's a different type. If one has opacity and the other does not
 * it always returns false.
 *
 * @returns true if the colors are similar when converted to the same space.
 */
bool Color::isSimilar(Color const &other, double epsilon) const
{
    if (other._space != _space) {
        if (auto copy = other.converted(_space)) {
            return isClose(*copy, epsilon);
        }
        return false; // bad color conversion
    }
    return isClose(other, epsilon);
}

/**
 * Return true if this color would be out of gamut when converted to another space.
 *
 * @arg other - The other color space to compare against.
 */
bool Color::isOutOfGamut(std::shared_ptr<Space::AnySpace> other) const
{
    return _space->outOfGamut(_values, other);
}

/**
 * Return true if this color would be considered over-inked.
 */
bool Color::isOverInked() const
{
    return _space->overInk(_values);
}

// Color-color in-place modification template
template <typename Func>
void Color::_color_mutate_inplace(Color const &other, unsigned int pin, Func func)
{
    // Convert the other's space and opacity if it's different
    if (other._space != _space || other.hasOpacity() != hasOpacity()) {
        if (auto copy = other.converted(*this)) {
            return _color_mutate_inplace(*copy, pin, func);
        }
        return; // Bad conversion
    }

    // Both are good, so average each channel
    for (unsigned int i = 0; i < _values.size(); i++) {
        if (pin & (1 << i)) {
            continue;
        }
        func(_values[i], other[i]);
    }
}

}; // namespace Inkscape::Colors

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
