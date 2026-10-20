// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Progress-splitter
 * Dynamically split a Progress into several sub-tasks.
 */
#ifndef INKSCAPE_ASYNC_PROGRESS_SPLITTER_H
#define INKSCAPE_ASYNC_PROGRESS_SPLITTER_H

#include <vector>
#include <optional>
#include "progress.h"

namespace Inkscape {
namespace Async {

/**
 * A RAII object for splitting a Progress into a dynamically-determined collection of sub-tasks.
 */
template <typename T, typename... S>
class ProgressSplitter
{
public:
    /// Construct a progress splitter for a given task.
    ProgressSplitter(Progress<T, S...> &parent) : parent(&parent) {}

    /// Add a SubProgress which makes progress \a amount.
    ProgressSplitter &add(std::optional<SubProgress<T, S...>> &progress, T amount)
    {
        entries.push_back({ &progress, amount });
        return *this;
    }

    /// Convenience method to enable a "fluent interface". Calls \a add if \a condition is true.
    ProgressSplitter &add_if(std::optional<SubProgress<T, S...>> &progress, T amount, bool condition)
    {
        return condition ? add(progress, amount) : *this;
    }

    /// Assign to each added SubProgress its portion of the total progress.
    ~ProgressSplitter() { apportion(); }

private:
    struct Entry
    {
        std::optional<SubProgress<T, S...>> *progress;
        T amount;
    };

    Progress<T, S...> *parent;
    std::vector<Entry> entries;

    void apportion() noexcept
    {
        if (entries.empty()) {
            return;
        }

        T total = 0;
        for (auto const &e : entries) {
            total += e.amount;
        }

        T from = 0;
        for (auto &e : entries) {
            e.progress->emplace(*parent, from / total, e.amount / total);
            from += e.amount;
        }
    }
};

} // namespace Async
} // namespace Inkscape

#endif // INKSCAPE_ASYNC_PROGRESS_SPLITTER_H
