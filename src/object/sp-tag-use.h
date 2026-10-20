// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <inkscape:tagref> implementation
 *
 * Authors:
 *   Theodore Janeczko
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SP_TAG_USE_H
#define SEEN_SP_TAG_USE_H

#include <memory>
#include <string>
#include <optional>

#include <sigc++/scoped_connection.h>

#include "sp-object.h"          // for SPObject
#include "util/cast.h"          // for tag_of
#include "xml/node.h"           // for Node

class SPDocument;
class SPItem;
class SPTagUseReference;
enum class SPAttr;
namespace Inkscape::XML {
class Node;
class Document;
} // namespace Inkscape::XML

class SPTagUse final : public SPObject {
    int tag() const override { return tag_of<decltype(*this)>; }

    // item built from the original's repr (the visible clone)
    // relative to the SPUse itself, it is treated as a child, similar to a grouped item relative to its group
    SPObject *child = nullptr;
    std::optional<std::string> href;

public:
    SPTagUse();
    ~SPTagUse() override;
    
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;
    Inkscape::XML::Node* write(Inkscape::XML::Document* doc, Inkscape::XML::Node* repr, unsigned flags) override;
    void release() override;
    
    virtual void href_changed(SPObject* old_ref, SPObject* ref);
    
    //virtual SPItem* unlink();
    virtual SPItem* get_original();
    virtual SPItem* root();

    // the reference to the original object
    std::unique_ptr<SPTagUseReference> ref;
    sigc::scoped_connection _changed_connection;
};

#endif // SEEN_SP_TAG_USE_H

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
