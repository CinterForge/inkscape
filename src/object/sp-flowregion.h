// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_ITEM_FLOWREGION_H
#define SEEN_SP_ITEM_FLOWREGION_H

#include "livarot/Shape.h"
#include "sp-item.h"

class SPFlowregion final : public SPItem
{
public:
    SPFlowregion() = default;
    ~SPFlowregion() override = default;
    int tag() const override { return tag_of<decltype(*this)>; }

    std::vector<std::unique_ptr<Shape>> computed;

    void updateComputed();

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;
    void remove_child(Inkscape::XML::Node *child) override;
    void update(SPCtx *ctx, unsigned int flags) override;
    void modified(guint flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr,
                               unsigned int flags) override;
    const char *typeName() const override;
    const char *displayName() const override;
};

class SPFlowregionExclude final : public SPItem
{
public:
    SPFlowregionExclude() = default;
    ~SPFlowregionExclude() override = default;
    int tag() const override { return tag_of<decltype(*this)>; }

    void child_added(Inkscape::XML::Node *child, Inkscape::XML::Node *ref) override;
    void remove_child(Inkscape::XML::Node *child) override;
    void update(SPCtx *ctx, unsigned int flags) override;
    void modified(guint flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr,
                               unsigned int flags) override;
    const char *typeName() const override;
    const char *displayName() const override;
    Shape *getComputed() { return _computed.get(); }

private:
    std::unique_ptr<Shape> _computed;
    void _updateComputed();
};

#endif
