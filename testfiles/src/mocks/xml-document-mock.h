// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @brief @file
 * Runtime-controllable mock implementation of the XML::Document interface.
 *
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * @copyright
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_TESTING_MOCKS_XML_DOCUMENT_H
#define INKSCAPE_TESTING_MOCKS_XML_DOCUMENT_H

#include <gmock/gmock.h>

#include "testfiles/src/mocks/xml-node-mock.h"
#include "xml/document.h"

namespace Inkscape::XML::Mock {

struct Document
    : Mock::Node
    , XML::Document
{
    Mock::Node &asNode() { return *this; }

    XML::Node *root() override { return Mock::Node::root(); }
    XML::Node const *root() const override { return Mock::Node::root(); }
    NodeType type() const override { return Mock::Node::type(); }
    char const *name() const override { return Mock::Node::name(); }

    int code() const override { return Mock::Node::code(); }
    unsigned position() const override { return Mock::Node::position(); }
    unsigned childCount() const override { return Mock::Node::childCount(); }
    char const *content() const override { return Mock::Node::content(); }
    char const *attribute(char const *key) const override { return Mock::Node::attribute(key); }
    AttributeVector const &attributeList() const override { return Mock::Node::attributeList(); }
    bool matchAttributeName(char const *partial_name) const override
    {
        return Mock::Node::matchAttributeName(partial_name);
    }
    void setPosition(int pos) override { return Mock::Node::setPosition(pos); }
    void setContent(char const *value) override { return Mock::Node::setContent(value); }
    void setCodeUnsafe(int code) override { return Mock::Node::setCodeUnsafe(code); }
    XML::Document *document() override { return Mock::Node::document(); }
    XML::Document const *document() const override { return Mock::Node::document(); }
    XML::Node *parent() override { return Mock::Node::parent(); }
    XML::Node const *parent() const override { return Mock::Node::parent(); }
    XML::Node *next() override { return Mock::Node::next(); }
    XML::Node const *next() const override { return Mock::Node::next(); }
    XML::Node *prev() override { return Mock::Node::prev(); }
    XML::Node const *prev() const override { return Mock::Node::prev(); }
    XML::Node *firstChild() override { return Mock::Node::firstChild(); }
    XML::Node const *firstChild() const override { return Mock::Node::firstChild(); }
    XML::Node *lastChild() override { return Mock::Node::lastChild(); }
    XML::Node const *lastChild() const override { return Mock::Node::lastChild(); }
    XML::Node *nthChild(unsigned index) override { return Mock::Node::nthChild(index); }
    XML::Node const *nthChild(unsigned index) const override { return Mock::Node::nthChild(index); }
    void addChild(XML::Node *child, XML::Node *after) override { return Mock::Node::addChild(child, after); }
    void appendChild(XML::Node *child) override { return Mock::Node::appendChild(child); }
    void removeChild(XML::Node *child) override { return Mock::Node::removeChild(child); }
    void changeOrder(XML::Node *child, XML::Node *after) override { return Mock::Node::changeOrder(child, after); }
    void cleanOriginal(XML::Node *src, char const *key) override { return Mock::Node::cleanOriginal(src, key); }
    bool equal(XML::Node const *other, bool recursive, bool skip_ids = false) override
    {
        return Mock::Node::equal(other, recursive, skip_ids);
    }
    void mergeFrom(XML::Node const *src, char const *key, bool extension = false, bool clean = false) override
    {
        return Mock::Node::mergeFrom(src, key, extension, clean);
    }
    void addObserver(NodeObserver &observer) override { return Mock::Node::addObserver(observer); }
    void removeObserver(NodeObserver &observer) override { return Mock::Node::removeObserver(observer); }
    void synthesizeEvents(NodeObserver &observer) override { return Mock::Node::synthesizeEvents(observer); }
    void addSubtreeObserver(NodeObserver &observer) override { return Mock::Node::addSubtreeObserver(observer); }
    void removeSubtreeObserver(NodeObserver &observer) override { return Mock::Node::removeSubtreeObserver(observer); }
    void recursivePrintTree(unsigned level) override { return Mock::Node::recursivePrintTree(level); }
    void setAttributeImpl(char const *key, char const *value) override
    {
        return Mock::Node::setAttributeImpl(key, value);
    }

    MOCK_METHOD0(inTransaction, bool());
    MOCK_METHOD0(beginTransaction, void());
    MOCK_METHOD0(rollback, void());
    MOCK_METHOD0(commit, void());
    MOCK_METHOD0(commitUndoable, Event *());
    MOCK_METHOD1(createElement, XML::Node *(char const *));
    MOCK_METHOD1(createTextNode, XML::Node *(char const *));
    MOCK_METHOD2(createTextNode, XML::Node *(char const *, bool));
    MOCK_METHOD1(createComment, XML::Node *(char const *));
    MOCK_METHOD2(createPI, XML::Node *(char const *, char const *));
    MOCK_CONST_METHOD1(duplicate, XML::Document *(XML::Document *));
    MOCK_METHOD0(logger, NodeObserver *());
};
} // namespace Inkscape::XML::Mock
#endif // INKSCAPE_TESTING_MOCKS_XML_DOCUMENT_H
