// SPDX-License-Identifier: GPL-2.0-or-later

#include "object-modified-tags.h"
#include <stdexcept>
#include "object/sp-object.h"

unsigned int get_next_object_modified_tag() {
    // next available tag
    static unsigned int next_tag = SP_OBJECT_USER_MODIFIED_TAG_1;

    auto tag = next_tag;
    if (tag > SP_OBJECT_USER_MODIFIED_TAG_8 || tag < SP_OBJECT_USER_MODIFIED_TAG_1) {
        // we ran out of available tags; see sp-object.h
        throw std::runtime_error("Object modified tags exhausted.");
    }
    next_tag <<= 1;
    return tag;
}
