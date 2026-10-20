// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_COLORS_GRADIENT_AVERAGER_H
#define INKSCAPE_COLORS_GRADIENT_AVERAGER_H

#include "manager.h"
#include "color.h"

namespace Inkscape::Colors {

/**
 * Accepts a sequence of gradient stops and computes the average colour of a gradient.
 *
 * It is assumed that the same (linear) colour space is used for both
 *
 *     1. Interpolation (generating the colours in between gradient stops)
 *     2. Blending (computing the average colour)
 *
 * For the more general case, see @a NonlinearGradientAverager.
 */
class LinearGradientAverager
{
public:
    /// @param space The space to perform gradient interpolation in (should be a linear space to make sense).
    explicit LinearGradientAverager(std::shared_ptr<Space::AnySpace> space = Manager::get().find(Space::Type::RGB))
        : _space{std::move(space)}
    {}

    /// @param pos The position of the gradient stop, which must be in [0, 1] and at least the previous stop.
    void addStop(double pos, Color col);

    /**
     * Complete the gradient, extending the last stop up to the end.
     * @return The average colour of the gradient.
     * @throws std::logic_error if there are no stops.
     */
    Color finish();

private:
    std::shared_ptr<Space::AnySpace> _space; ///< The interpolation and blending space
    std::vector<double> _accumulated; ///< Weighted sum of premultiplied values with alpha
    struct Stop
    {
        double pos;
        std::vector<double> values; ///< Premultiplied values with alpha
    };
    std::optional<Stop> _last; ///< The last gradient stop added
};

/**
 * Accepts a sequence of gradient stops and computes the average colour of a gradient,
 * allowing interpolation and blending in different colour spaces.
 */
class NonlinearGradientAverager
{
public:
    /**
     * @param interp_space The space to interpolate gradient stops in (can be any space).
     * @param blend_space The space to blend in (should be a linear space to make sense).
     * @param subdivisions How many subdivisions to use for piecewise linear approximation (higher = better).
     */
    explicit NonlinearGradientAverager(std::shared_ptr<Space::AnySpace> interp_space,
                                       std::shared_ptr<Space::AnySpace> blend_space,
                                       int subdivisions = 20)
        : _interp_space{std::move(interp_space)}
        , _averager{std::move(blend_space)}
        , _subdivisions{subdivisions}
    {}

    /// @param pos The position of the gradient stop, which must be in [0, 1] and at least the previous stop.
    void addStop(double pos, Color col);

    /**
     * Complete the gradient, extending the last stop up to the end.
     * @return The average colour of the gradient.
     * @throws std::logic_error if there are no stops.
     */
    Color finish() { return _averager.finish(); }

private:
    std::shared_ptr<Space::AnySpace> _interp_space;
    LinearGradientAverager _averager;
    int _subdivisions{};
    struct Stop
    {
        double pos;
        Colors::Color col; ///< In the interpolation space, with alpha
    };
    std::optional<Stop> _last; ///< The last gradient stop added
};

} // namespace Inkscape::Colors

#endif // INKSCAPE_COLORS_GRADIENT_AVERAGER_H
