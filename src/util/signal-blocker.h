// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * RAII blocker for sigc++ signals.
 *
 * Authors:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2014 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_UTIL_SIGNAL_BLOCKER_H
#define SEEN_INKSCAPE_UTIL_SIGNAL_BLOCKER_H

/**
 * RAII blocker for sigc++ signals.
 */
template <typename T>
class SignalBlocker
{
public:
    /**
     * Creates a new instance that if the signal is currently unblocked will block
     * it until this instance is destructed and then will unblock it.
     */
    [[nodiscard]] explicit SignalBlocker(T &connection)
        : _connection{connection}
        , _was_blocked{_connection.blocked()}
    {
        if (!_was_blocked) {
            _connection.block();
        }
    }

    /**
     * Destructor that will unblock the signal if it was blocked initially by this
     * instance.
     */
    ~SignalBlocker()
    {
        if (!_was_blocked) {
            _connection.unblock();
        }
    }

    SignalBlocker(SignalBlocker const &) = delete;
    SignalBlocker &operator=(SignalBlocker const &) = delete;

private:
    T &_connection;
    bool const _was_blocked;
};

#endif // SEEN_INKSCAPE_UTIL_SIGNAL_BLOCKER_H

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
