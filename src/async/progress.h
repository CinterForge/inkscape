// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Progress
 * Interface for reporting progress and checking cancellation.
 */
#ifndef INKSCAPE_ASYNC_PROGRESS_H
#define INKSCAPE_ASYNC_PROGRESS_H

#include <chrono>

namespace Inkscape {
namespace Async {

class CancelledException {};

/**
 * An interface for tasks to report progress and check for cancellation.
 * Not supported:
 *  - Error reporting - use exceptions!
 *  - Thread-safety - overrides should provide this if needed using e.g. BackgroundProgress.
 */
template <typename... T>
class Progress
{
public:
    /// Report a progress value, returning false if cancelled.
    bool report(T const &... progress) { return _report(progress...); }

    /// Report a progress value, throwing CancelledException if cancelled.
    void report_or_throw(T const &... progress) { if (!_report(progress...)) throw CancelledException(); }

    /// Return whether not cancelled.
    bool keepgoing() const { return _keepgoing(); }

    /// Throw CancelledException if cancelled.
    void throw_if_cancelled() const { if (!_keepgoing()) throw CancelledException(); }

    /// Convenience function - same as check().
    operator bool() const { return _keepgoing(); }

protected:
    ~Progress() = default;
    virtual bool _keepgoing() const = 0;
    virtual bool _report(T const &... progress) = 0;
};

/**
 * A Progress object representing a sub-task of another Progress.
 */
template <typename T, typename... S>
class SubProgress final
    : public Progress<T, S...>
{
public:
    /// Construct a progress object for a sub-task.
    SubProgress(Progress<T, S...> &parent, T from, T amount)
    {
        if (auto p = dynamic_cast<SubProgress*>(&parent)) {
            _root = p->_root;
            _from = p->_from + p->_amount * from;
            _amount = p->_amount * amount;
        } else {
            _root = &parent;
            _from = from;
            _amount = amount;
        }
    }

private:
    Progress<T, S...> *_root;
    T _from, _amount;

    bool _keepgoing() const override { return _root->keepgoing(); }
    bool _report(T const &progress, S const &... aux) override { return _root->report(_from + _amount * progress, aux...); }
};

/**
 * A Progress object that throttles reports to a given step size.
 */
template <typename T, typename... S>
class ProgressStepThrottler final
    : public Progress<T, S...>
{
public:
    ProgressStepThrottler(Progress<T, S...> &parent, T step)
        : parent(&parent), step(step) {}

private:
    Progress<T, S...> *parent;
    T step;
    T last = 0;

    bool _keepgoing() const override { return parent->keepgoing(); }

    bool _report(T const &progress, S const &... aux) override
    {
        if (progress - last < step) {
            return parent->keepgoing();
        } else {
            last = progress;
            return parent->report(progress, aux...);
        }
    }
};

/**
 * A Progress object that throttles reports to a given time interval.
 */
template <typename... T>
class ProgressTimeThrottler final
    : public Progress<T...>
{
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;

public:
    using duration = clock::duration;

    ProgressTimeThrottler(Progress<T...> &parent, duration interval)
        : parent(&parent), interval(interval) {}

private:
    Progress<T...> *parent;
    duration interval;
    time_point last = clock::now();

    bool _keepgoing() const override { return parent->keepgoing(); }

    bool _report(T const &... progress) override
    {
        auto now = clock::now();
        if (now - last < interval) {
            return parent->keepgoing();
        } else {
            last = now;
            return parent->report(progress...);
        }
    }
};

/**
 * A dummy Progress object that never reports cancellation.
 */
template <typename... T>
class ProgressAlways final
    : public Progress<T...>
{
private:
    bool _keepgoing() const override { return true; }
    bool _report(T const &...) override { return true; }
};

} // namespace Async
} // namespace Inkscape

#endif // INKSCAPE_ASYNC_PROGRESS_H
