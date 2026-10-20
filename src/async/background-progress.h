// SPDX-License-Identifier: GPL-2.0-or-later
/** \file Background-progress
 * A Progress object that reports progress thread-safely over a Channel.
 */
#ifndef INKSCAPE_ASYNC_BACKGROUND_PROGRESS_H
#define INKSCAPE_ASYNC_BACKGROUND_PROGRESS_H

#include <functional>
#include "channel.h"
#include "progress.h"

namespace Inkscape {
namespace Async {

template <typename... T>
class BackgroundProgress final
    : public Progress<T...>
{
public:
    /**
     * Construct a Progress object which becomes cancelled as soon as \a channel is closed,
     * and reports progress by calling \a onprogress over \a channel.
     *
     * The result can only be used within the lifetime of \a channel.
     */
    BackgroundProgress(Channel::Source &channel, std::function<void(T...)> &onprogress)
        : channel(&channel)
        , onprogress(std::move(onprogress)) {}

private:
    Channel::Source *channel;
    std::function<void(T...)> onprogress;

    bool _keepgoing() const override { return channel->operator bool(); }
    bool _report(T const &... progress) override { return channel->run(std::bind(onprogress, progress...)); }
};

} // namespace Async
} // namespace Inkscape

#endif // INKSCAPE_ASYNC_BACKGROUND_PROGRESS_H
