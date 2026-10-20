// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests migrated from cxxtest
 *
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2024 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include <src/document.h>
#include <src/inkscape.h>
#include <src/object/sp-root.h>
#include <src/object/sp-object.h>

using namespace Inkscape;
using namespace Inkscape::XML;
using Nature = SPObject::LinkedObjectNature;
using namespace std::literals;

class ObjectLinksTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        Inkscape::Application::create(false);
    }

    void SetUp() override {
        constexpr auto docString = R"A(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg version="1.1" id="svg2" width="245" height="110" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns="http://www.w3.org/2000/svg">
  <g id="holder" style="fill:#a51d2d">
    <rect style="stroke:none;stroke-width:7.62315;stroke-linecap:round;stroke-linejoin:round" id="blueberry" width="50" height="50" x="10" y="10" />
    <use x="0" y="0" xlink:href="#blueberry" id="banana" transform="translate(60)" style="fill:#008000" />
    <use x="0" y="0" xlink:href="#banana" id="peach" transform="translate(60)" style="fill:#ff0000" />
    <text xml:space="preserve" style="fill:#a51d2d;" id="pathtext" transform="translate(5,105)"><textPath xlink:href="#textpath" startOffset="50%" id="subtext" style="font-size:11px;font-family:'Noto Sans';">Text from the blue path</textPath></text>
    <path style="fill:none;stroke:#1a5fb4;stroke-width:1;" d="M 20.493281,-6.8198204 C 44.533623,-28.1299 82.044808,-31.874126 109.5958,-15.089731 c 18.83597,10.2521826 40.69713,14.53112164 61.50635,8.1113336" id="textpath" />
    <text xml:space="preserve" style="font-size:6px;white-space:pre;shape-inside:url(#blueberry);fill:#3d3846;" x="200" y="10" id="boxedtext" transform="translate(177)"><tspan x="35" y="57.763855" id="tspan5" style="font-size:6px;font-family:'Noto Sans';">This text should flow into the rectangle and should continue to flow after the cropping function is completed</tspan></text>
    <a id="boat" href="#linked_to"><rect id="linked_from"/></a>
    <rect id="linked_to"/>
  </g>
</svg>)A"sv;
        doc = SPDocument::createNewDocFromMem(docString);

        ASSERT_TRUE(doc);
        ASSERT_TRUE(doc->getRoot());
    }

    std::vector<SPObject *> getObjects(std::vector<std::string> const &lst) {
        std::vector<SPObject *> ret;
        for (auto &id : lst) {
            ret.push_back(doc->getObjectById(id));
        }
        return ret;
    }

    std::unique_ptr<SPDocument> doc;
};

::testing::AssertionResult ObjectIdsEq(int i, std::set<std::string> const& a, std::set<std::string> const& b) {
    std::set<std::string> delta;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
                        std::inserter(delta, delta.begin()));
    std::set_difference(b.begin(), b.end(), a.begin(), a.end(),
                        std::inserter(delta, delta.begin()));

    if (delta.size()) {
        std::ostringstream oo;
        for (auto id : delta) {
            if (std::find(a.begin(), a.end(), id) == a.end()) {
                oo << i << ". unexpected linked object '" << id << "' found.\n";
            } else {
                oo << i << ". expected linked object '" << id << "' not found.\n";
            }
        }
        return ::testing::AssertionFailure() << oo.str();
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult ObjectListEq(int i, std::vector<SPObject *> const& a, std::set<std::string> const& b) {
    std::set<std::string> a_ids;
    for (auto obj : a) {
        a_ids.insert(obj->getId());
    }
    return ObjectIdsEq(i, a_ids, b);
}


TEST_F(ObjectLinksTest, getLinked)
{
    std::vector<std::pair<std::pair<std::string, Nature>, std::set<std::string>>> tests = {
        // clang-format off
        // Groups don't directly link to anything (nonrecursive)
        {{"holder",    Nature::ANY}, {}},

        // A clone is forward linked to it's source, and backwards linked to anything cloning it
        {{"banana",    Nature::DEPENDENT},    {"peach"}},
        {{"banana",    Nature::ANY},          {"blueberry", "peach"}},
        {{"banana",    Nature::DEPENDENCY},   {"blueberry"}},

        // A clone of a clone is forward linked to it's source
        {{"peach",     Nature::DEPENDENT},    {}},
        {{"peach",     Nature::ANY},          {"banana"}},
        {{"peach",     Nature::DEPENDENCY},   {"banana"}},

        // A shape used for clones and flowed text is back linked
        {{"blueberry", Nature::DEPENDENT},    {"banana", "boxedtext"}},
        {{"blueberry", Nature::ANY},          {"banana", "boxedtext"}},
        {{"blueberry", Nature::DEPENDENCY},   {}},

        // Text flowed into a shape has a forward link to that shape
        {{"boxedtext", Nature::DEPENDENT},    {}},
        {{"boxedtext", Nature::ANY},          {"blueberry"}},
        {{"boxedtext", Nature::DEPENDENCY},   {"blueberry"}},

        // A shape used to shape text has back links to the text
        {{"textpath",  Nature::DEPENDENT},    {"subtext"}},
        {{"textpath",  Nature::ANY},          {"subtext"}},
        {{"textpath",  Nature::DEPENDENCY},   {}},

        // Text on a path has a forward link to it's shape
        {{"pathtext",  Nature::DEPENDENT},    {}},
        {{"pathtext",  Nature::ANY},          {"textpath"}},
        {{"pathtext",  Nature::DEPENDENCY},   {"textpath"}},

        // Anchor tags are linked correctly
        {{"linked_to",   Nature::DEPENDENT},  {"boat"}},
        {{"linked_to",   Nature::DEPENDENCY}, {}},
        {{"boat",        Nature::DEPENDENT},  {}},
        {{"boat",        Nature::DEPENDENCY}, {"linked_to"}}
        // clang-format on
    };
    auto i = 0;
    for (auto &test : tests) {
        auto obj = doc->getObjectById(test.first.first);
        ASSERT_TRUE(obj);

        auto objects = obj->getLinked(test.first.second);
        ASSERT_TRUE(ObjectListEq(i, objects, test.second));
        i++;
    }
}

TEST_F(ObjectLinksTest, getLinkedRecursive)
{
    std::vector<std::pair<std::pair<std::string, Nature>, std::set<std::string>>> tests = {
        // clang-format off
        // Groups link to everything via recursion
        {{"holder", Nature::DEPENDENT},  {"peach", "banana", "boxedtext", "subtext", "boat"}},
        {{"holder", Nature::ANY},        {"blueberry", "peach", "banana", "boxedtext", "subtext", "textpath", "boat", "linked_to"}},
        {{"holder", Nature::DEPENDENCY}, {"blueberry", "banana", "textpath", "linked_to"}},

        // A clone is forward linked to it's source, and backwards linked to anything cloning it
        {{"banana", Nature::DEPENDENT},  {"peach"}},
        {{"banana", Nature::ANY},        {"blueberry", "peach", "boxedtext", "banana"}},
        {{"banana", Nature::DEPENDENCY}, {"blueberry",}},

        {{"peach",  Nature::DEPENDENCY}, {"banana", "blueberry"}}
        // clang-format on
    };
    auto i = 0;
    for (auto &test : tests) {
        auto obj = doc->getObjectById(test.first.first);
        ASSERT_TRUE(obj);

        std::vector<SPObject *> objects;
        obj->getLinkedRecursive(objects, test.first.second);
        ASSERT_TRUE(ObjectListEq(i, objects, test.second));
        i++;
    }
}

TEST_F(ObjectLinksTest, cropToObject)
{
    doc->getRoot()->cropToObjects({doc->getObjectById("peach")});

    ASSERT_TRUE(doc->getObjectById("peach"));
    ASSERT_TRUE(doc->getObjectById("blueberry"));
    ASSERT_TRUE(doc->getObjectById("banana"));
    ASSERT_FALSE(doc->getObjectById("nothing"));
    ASSERT_FALSE(doc->getObjectById("pathtext"));
    ASSERT_FALSE(doc->getObjectById("textpath"));
    ASSERT_FALSE(doc->getObjectById("boxedtext"));
}
