// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_COLOR_H
#define SEEN_COLORS_COLOR_H

#include <memory>
#include <string>
#include <vector>

#include "colors/spaces/enum.h"
#include "utils.h"

namespace Inkscape::Colors {
namespace Space {
class AnySpace;
} // namespace Space

class Color final
{
public:
    Color(std::shared_ptr<Space::AnySpace> space, std::vector<double> colors);
    Color(Space::Type space_type, std::vector<double> values);
    explicit Color(uint32_t color, bool alpha = true);

    static std::optional<Color> parse(char const *value);
    static std::optional<Color> parse(std::string const &value);
    static std::optional<Color> ifValid(Space::Type space_type, std::vector<double> values);

    bool operator==(Color const &other) const;
    double operator[](unsigned int index) const { return get(index); }

    std::shared_ptr<Space::AnySpace> const &getSpace() const { return _space; }
    const std::vector<double> &getValues() const { return _values; }
    void setValues(std::vector<double> values);
    size_t size() const { return _values.size(); }

    double get(unsigned int index) const;
    bool set(unsigned int index, double value);
    bool set(Color const &other, bool keep_space = true);
    bool set(std::string const &parsable, bool keep_space = true);
    bool set(uint32_t rgba, bool opacity = true);

    bool hasOpacity() const;
    void enableOpacity(bool enabled);
    unsigned int getOpacityChannel() const;
    double getOpacity() const;
    double stealOpacity();
    bool setOpacity(double opacity);
    bool addOpacity(double opacity = 1.0) { return setOpacity(opacity * getOpacity()); }
    Color withOpacity(double opacity) const;

    unsigned int getPin(unsigned int channel) const;

    static constexpr double EPSILON = 1e-4;

    double difference(Color const &other) const;
    bool isClose(Color const &other, double epsilon = EPSILON) const;
    bool isSimilar(Color const &other, double epsilon = EPSILON) const;

    bool convert(Color const &other);
    bool convert(std::shared_ptr<Space::AnySpace> space);
    bool convert(Space::Type type);
    std::optional<Color> converted(Color const &other) const;
    std::optional<Color> converted(std::shared_ptr<Space::AnySpace> to_space) const;
    std::optional<Color> converted(Space::Type type) const;

    std::string toString(bool opacity = true) const;
    uint32_t toRGBA(double opacity = 1.0) const;
    uint32_t toARGB(double opacity = 1.0) const;
    uint32_t toABGR(double opacity = 1.0) const;

    std::string getName() const { return _name; }
    void setName(std::string name) { _name = std::move(name); }

    bool isOutOfGamut(std::shared_ptr<Space::AnySpace> other) const;
    bool isOverInked() const;

    void normalize();
    Color normalized() const;

    void compose(Color const &other);
    Color composed(Color const &other) const;

    void average(Color const &other, double pos = 0.5, unsigned int pin = 0);
    Color averaged(Color const &other, double pos = 0.5) const;

    void invert(unsigned int pin);
    void invert() { invert(getPin(getOpacityChannel())); }
    void jitter(double force, unsigned int pin = 0);

private:
    std::string _name;
    std::vector<double> _values;
    std::shared_ptr<Space::AnySpace> _space;

    template <typename Func>
    void _color_mutate_inplace(Color const &other, unsigned int pin, Func avgFunc);

    bool _isnear(std::vector<double> const &other, double epsilon = 0.001) const;
};

class ColorError : public std::exception
{
public:
    ColorError(std::string &&msg)
        : _msg(msg)
    {}
    char const *what() const noexcept override { return _msg.c_str(); }

private:
    std::string _msg;
};

} // namespace Inkscape::Colors

#endif // SEEN_COLORS_COLOR_H
