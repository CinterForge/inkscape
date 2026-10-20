// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors:
 * see git history
 * John Cliff <simarilius@yahoo.com>
 *
 * Copyright (C) 2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INK_STOCK_ITEMS_H
#define SEEN_INK_STOCK_ITEMS_H

#include <functional>
#include <memory>
#include <vector>
#include "libnrtype/font-factory.h"

class SPObject;
class SPDocument;

// Stock objects kept in documents with controlled life time
class StockPaintDocuments
    : public Inkscape::Util::EnableSingleton<StockPaintDocuments, Inkscape::Util::Depends<FontFactory>>
{
public:
    std::vector<SPDocument *> get_paint_documents(std::function<bool (SPDocument *)> const &filter);

private:
    StockPaintDocuments();
    friend class EnableSingleton;

    std::vector<std::unique_ptr<SPDocument>> documents;
};

SPObject *get_stock_item(char const *urn, bool stock = false, SPDocument* stock_doc = nullptr);

#endif // SEEN_INK_STOCK_ITEMS_H
