// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Algorithms::nearest_common_ancestor
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_ALGORITHMS_NEAREST_COMMON_ANCESTOR_H
#define SEEN_INKSCAPE_ALGORITHMS_NEAREST_COMMON_ANCESTOR_H

#include <iterator>
#include <functional>

namespace Inkscape {

namespace Algorithms {

/**
 * Time costs:
 *
 * The case of sharing a common successor is handled in O(1) time.
 *
 * If \a a is the nearest common ancestor, then runs in O(len(rest of b)) time.
 *
 * Otherwise, runs in O(len(a) + len(b)) time.
 */

template <typename ForwardIterator>
ForwardIterator nearest_common_ancestor(ForwardIterator a, ForwardIterator b, ForwardIterator end)
{
    if ( a == end || b == end ) {
        return end;
    }

    /* Handle in O(1) time the common cases of identical lists or tails. */
    {
        /* identical lists? */
        if ( a == b ) {
            return a;
        }

        /* identical tails? */
        ForwardIterator tail_a(a);
        ForwardIterator tail_b(b);
        if ( ++tail_a == ++tail_b ) {
            return tail_a;
        }
    }

    /* Build parallel lists of suffixes, ordered by increasing length. */

    ForwardIterator lists[2] = { a, b };
    std::vector<ForwardIterator> suffixes[2];

    for ( int i=0 ; i < 2 ; i++ ) {
        for ( ForwardIterator iter(lists[i]) ; iter != end ; ++iter ) {
            if ( iter == lists[1-i] ) {
                // the other list is a suffix of this one
                return lists[1-i];
            }
            suffixes[i].push_back(iter);
        }
    }

    /* Iterate in parallel through the lists of suffix lists from shortest to
     * longest, stopping before the first pair of suffixes that differs
     */

    ForwardIterator longest_common(end);

    while ( !suffixes[0].empty() && !suffixes[1].empty() &&
             suffixes[0].back() == suffixes[1].back() )
    {
        longest_common = suffixes[0].back();
        suffixes[0].pop_back();
        suffixes[1].pop_back();
    }

    return longest_common;
}

}

}

#endif /* !SEEN_INKSCAPE_ALGORITHMS_NEAREST_COMMON_ANCESTOR_H */

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
