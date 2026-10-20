// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <utility>

#include "xml/node.h"

namespace Inkscape {
std::pair<char const *, char const *> getHrefAttribute(XML::Node const &);
void setHrefAttribute(XML::Node &, Util::const_char_ptr);
} // namespace Inkscape
