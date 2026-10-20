// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Node iterators
 *
 * Copyright (C) 2004 MenTaLguY
 */

#include "node-iterators.h"
#include "xml/node.h"

namespace Inkscape {
namespace XML {

Node const *NodeSiblingIteratorStrategy::next(Node const *node)
{
    return node ? node->next() : nullptr;
}

Node const *NodeParentIteratorStrategy::next(Node const *node)
{
    return node ? node->parent() : nullptr;
}

} // namespace XML
} // namespace Inkscape

// vim: expandtab:shiftwidth=4:softtabstop=4:fileencoding=utf-8:textwidth=99 :
