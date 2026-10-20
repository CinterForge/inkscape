// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_UTIL_STATIC_DOC_H
#define INKSCAPE_UTIL_STATIC_DOC_H

#include <memory>

#include "document.h"
#include "libnrtype/font-factory.h"
#include "statics.h"

namespace Inkscape::Util {

/**
 * Wrapper for a static SPDocument to ensure it is destroyed early enough.
 *
 * SPDocuments cannot outlive FontFactory which in turn cannot outlive the end of main().
 * Because of these unusual lifetime requirements, managing a static SPDocument requires
 * some extra work, which can be done by replacing this
 *
 *     static std::unique_ptr<SPDocument> doc = create_doc();
 *
 * with this
 *
 *     SPDocument *doc = cache_static_doc([] { return create_doc(); });
 *
 */
template <typename F, auto = [] {}>
SPDocument *cache_static_doc(F &&f)
{
    struct DocHolder : EnableSingleton<DocHolder, Depends<FontFactory>>
    {
        std::unique_ptr<SPDocument> doc;

        DocHolder(F &&f)
            : doc{std::forward<F>(f)()}
        {}
    };

    return DocHolder::get(std::forward<F>(f)).doc.get();
}

} // namespace Inkscape::Util

#endif // INKSCAPE_UTIL_STATIC_DOC_H
