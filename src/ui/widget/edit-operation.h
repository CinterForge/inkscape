// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 9/11/24.
//

#ifndef EDIT_OPERATION_H
#define EDIT_OPERATION_H

namespace Inkscape::UI {

// Common enum for basic editing operations

enum class EditOperation {
    // request to create a new item
    New,
    // request to delete selected item
    Delete,
    // request to change selected item
    Change,
    // request to import items
    Import,
    // request to export items
    Export,
    // change label
    Rename
};

} // namespace

#endif //EDIT_OPERATION_H
