// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef INKSCAPE_SP_STYLE_ELEM_H
#define INKSCAPE_SP_STYLE_ELEM_H

#include <memory>
#include <vector>
#include "3rdparty/libcroco/src/cr-statement.h"

#include "media.h"
#include "sp-object.h"

#include "xml/node-observer.h"

class SPStyleElem;

class SPStyleElemNodeObserver : public Inkscape::XML::NodeObserver
{
    friend class SPStyleElem;
    ~SPStyleElemNodeObserver() override = default; // can only exist as a direct base of SPStyleElem

    void notifyChildAdded(Inkscape::XML::Node &, Inkscape::XML::Node &, Inkscape::XML::Node *) final;
    void notifyChildRemoved(Inkscape::XML::Node &, Inkscape::XML::Node &, Inkscape::XML::Node *) final;
    void notifyChildOrderChanged(Inkscape::XML::Node &, Inkscape::XML::Node &, Inkscape::XML::Node *, Inkscape::XML::Node *) final;
};

class SPStyleElemTextNodeObserver : public Inkscape::XML::NodeObserver
{
    friend class SPStyleElem;
    ~SPStyleElemTextNodeObserver() override = default; // can only exist as a direct base of SPStyleElem

    void notifyContentChanged(Inkscape::XML::Node &, Inkscape::Util::ptr_shared, Inkscape::Util::ptr_shared) final;
};

class SPStyleElem final
    : public SPObject
    , private SPStyleElemNodeObserver
    , private SPStyleElemTextNodeObserver
{
public:
    SPStyleElem();
    ~SPStyleElem() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    // Container for the libcroco style sheet instance created on load.
    CRStyleSheet *style_sheet{nullptr};

    Media media;
    bool is_css{false};

    std::vector<std::unique_ptr<SPStyle>> get_styles() const;

    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void set(SPAttr key, char const *value) override;
    void read_content() override;
    void release() override;

    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned int flags) override;

private:
    SPStyleElemNodeObserver &nodeObserver() { return *this; }
    SPStyleElemTextNodeObserver &textNodeObserver() { return *this; }

    // for static_casts
    friend SPStyleElemNodeObserver;
    friend SPStyleElemTextNodeObserver;
};

#endif /* !INKSCAPE_SP_STYLE_ELEM_H */

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
