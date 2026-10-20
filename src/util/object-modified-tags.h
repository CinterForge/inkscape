// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OBJECT_MODIFIED_TAGS_H
#define OBJECT_MODIFIED_TAGS_H

// Request next unique tag for marking content changes in DocumentUndo::done().
// Those tags can be requested by dialogs and used in selectionModified() to avoid updates.
//
unsigned int get_next_object_modified_tag();

#endif
