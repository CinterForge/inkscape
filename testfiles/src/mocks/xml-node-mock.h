// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @brief @file
 * Runtime-controllable mock implementation of the XML::Node interface.
 *
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * @copyright
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_TESTING_MOCKS_XML_NODE_H
#define INKSCAPE_TESTING_MOCKS_XML_NODE_H

#include <gmock/gmock.h>

#include "xml/attribute-record.h"
#include "xml/node.h"

namespace Inkscape::XML::Mock {

struct Node : public XML::Node
{
    MOCK_CONST_METHOD0(type, NodeType());
    MOCK_CONST_METHOD0(name, char const *());
    MOCK_CONST_METHOD0(content, char const *());
    MOCK_CONST_METHOD1(attribute, char const *(char const *));
    MOCK_CONST_METHOD0(code, int());
    MOCK_CONST_METHOD0(position, unsigned());
    MOCK_CONST_METHOD0(childCount, unsigned());
    MOCK_CONST_METHOD0(attributeList, AttributeVector const &());
    MOCK_CONST_METHOD1(matchAttributeName, bool(char const *));
    MOCK_METHOD1(setPosition, void(int));
    MOCK_METHOD1(setContent, void(char const *));
    MOCK_METHOD2(setAttribute, void(Util::const_char_ptr, Util::const_char_ptr));
    MOCK_METHOD1(setCodeUnsafe, void(int));
    MOCK_METHOD0(document, XML::Document *());
    MOCK_CONST_METHOD0(document, XML::Document const *());
    MOCK_METHOD0(root, XML::Node *());
    MOCK_CONST_METHOD0(root, XML::Node const *());
    MOCK_METHOD0(parent, XML::Node *());
    MOCK_CONST_METHOD0(parent, XML::Node const *());
    MOCK_METHOD0(next, XML::Node *());
    MOCK_CONST_METHOD0(next, XML::Node const *());
    MOCK_METHOD0(prev, XML::Node *());
    MOCK_CONST_METHOD0(prev, XML::Node const *());
    MOCK_METHOD0(firstChild, XML::Node *());
    MOCK_CONST_METHOD0(firstChild, XML::Node const *());
    MOCK_METHOD0(lastChild, XML::Node *());
    MOCK_CONST_METHOD0(lastChild, XML::Node const *());
    MOCK_METHOD1(nthChild, XML::Node *(unsigned));
    MOCK_CONST_METHOD1(nthChild, XML::Node const *(unsigned));
    MOCK_CONST_METHOD1(duplicate, XML::Node *(XML::Document *));
    MOCK_METHOD2(addChild, void(XML::Node *, XML::Node *));
    MOCK_METHOD1(appendChild, void(XML::Node *));
    MOCK_METHOD1(removeChild, void(XML::Node *));
    MOCK_METHOD2(changeOrder, void(XML::Node *, XML::Node *));
    MOCK_METHOD2(cleanOriginal, void(XML::Node *, char const *));
    MOCK_METHOD3(equal, bool(XML::Node const *, bool, bool));
    MOCK_METHOD4(mergeFrom, void(XML::Node const *, char const *, bool, bool));
    MOCK_METHOD1(addObserver, void(XML::NodeObserver &));
    MOCK_METHOD1(removeObserver, void(XML::NodeObserver &));
    MOCK_METHOD1(synthesizeEvents, void(XML::NodeObserver &));
    MOCK_METHOD1(addSubtreeObserver, void(XML::NodeObserver &));
    MOCK_METHOD1(removeSubtreeObserver, void(XML::NodeObserver &));
    MOCK_METHOD1(recursivePrintTree, void(unsigned));
    MOCK_METHOD2(setAttributeImpl, void(char const *, char const *));
};
} // namespace Inkscape::XML::Mock
#endif // INKSCAPE_TESTING_MOCKS_XML_NODE_H