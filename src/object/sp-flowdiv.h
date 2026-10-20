// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_ITEM_FLOWDIV_H
#define SEEN_SP_ITEM_FLOWDIV_H

/*
 */

#include "sp-object.h"
#include "sp-item.h"

// these 3 are derivatives of SPItem to get the automatic style handling
class SPFlowdiv final : public SPItem {
public:
	SPFlowdiv();
	~SPFlowdiv() override;
    int tag() const override { return tag_of<decltype(*this)>; }

protected:
	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttr key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

class SPFlowtspan final : public SPItem {
public:
	SPFlowtspan();
	~SPFlowtspan() override;
    int tag() const override { return tag_of<decltype(*this)>; }

protected:
	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttr key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

class SPFlowpara final : public SPItem {
public:
	SPFlowpara();
	~SPFlowpara() override;
    int tag() const override { return tag_of<decltype(*this)>; }

protected:
	void build(SPDocument *document, Inkscape::XML::Node *repr) override;
	void release() override;
	void update(SPCtx* ctx, unsigned int flags) override;
	void modified(unsigned int flags) override;

	void set(SPAttr key, char const* value) override;
	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

// these do not need any style
class SPFlowline final : public SPObject {
public:
	SPFlowline();
	~SPFlowline() override;
    int tag() const override { return tag_of<decltype(*this)>; }

protected:
	void release() override;
	void modified(unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

class SPFlowregionbreak final : public SPObject {
public:
	SPFlowregionbreak();
	~SPFlowregionbreak() override;
    int tag() const override { return tag_of<decltype(*this)>; }

protected:
	void release() override;
	void modified(unsigned int flags) override;

	Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};

#endif
