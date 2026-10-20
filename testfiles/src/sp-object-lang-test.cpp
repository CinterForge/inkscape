// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Test handling of lang attribute.
 *
 * Authors:
 *   Alvin Wong
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <doc-per-case-test.h>
#include <glibmm/main.h>
#include <gtest/gtest.h>

#include "attributes.h"
#include "document.h"
#include "inkscape.h"
#include "object/sp-glyph.h"
#include "object/sp-root.h"
#include "object/sp-text.h"
#include "object/sp-tspan.h"

using namespace Inkscape;

namespace Glib {

void PrintTo(const ustring &value, std::ostream *os)
{
    *os << "\"" << value << "\"";
}

} // namespace Glib

class SPObjectLangTest : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        Inkscape::Application::create(false);

        main_loop = Glib::MainLoop::create();
    }

    static void TearDownTestSuite() { main_loop.reset(); }

    void SetUp() override
    {
        std::string docString = R"A(
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
  <g id="layer1">
    <text
       id="text1"
       xml:space="preserve"
      ><tspan
         id="tspan11"
         xml:lang="zh-Hant-TW"
        >遍</tspan><tspan
         id="tspan12"
         lang="zh-Hant-HK"
        >遍</tspan><tspan
         id="tspan13"
         xml:lang="zh-Hans"
         lang="zh-Hant"
        >遍</tspan><tspan
         id="tspan14"
         lang="zh-Hant"
         xml:lang="zh-Hans"
        >遍</tspan><tspan
         id="tspan15"
        >遍</tspan></text>
    <text
       id="text2"
       xml:space="preserve"
       xml:lang="ja"
      ><tspan
         id="tspan21"
         xml:lang="zh"
        >遍</tspan><tspan
         id="tspan22"
        >遍</tspan><tspan
         id="tspan23"
         xml:lang=""
        >遍</tspan></text>
  </g>
  <g id="layer2" xml:lang="ko">
    <text
       id="text3"
       xml:space="preserve"
      ><tspan
         id="tspan31"
        >遍</tspan></text>
    <text
       id="text4"
       xml:space="preserve"
       xml:lang="jp"
      ><tspan
         id="tspan41"
        >遍</tspan></text>
  </g>
</svg>
        )A";
        doc = SPDocument::createNewDocFromMem(docString);

        ASSERT_NE(doc, nullptr);

        root = doc->getRoot();
        ASSERT_NE(root, nullptr);
        ASSERT_NE(root->getRepr(), nullptr);
        ASSERT_TRUE(root->hasChildren());

        layer1 = doc->getObjectById("layer1");
        ASSERT_NE(layer1, nullptr);
        text1 = cast<SPText>(doc->getObjectById("text1"));
        ASSERT_NE(text1, nullptr);
        tspan11 = cast<SPTSpan>(doc->getObjectById("tspan11"));
        ASSERT_NE(tspan11, nullptr);
        tspan12 = cast<SPTSpan>(doc->getObjectById("tspan12"));
        ASSERT_NE(tspan12, nullptr);
        tspan13 = cast<SPTSpan>(doc->getObjectById("tspan13"));
        ASSERT_NE(tspan13, nullptr);
        tspan14 = cast<SPTSpan>(doc->getObjectById("tspan14"));
        ASSERT_NE(tspan14, nullptr);
        tspan15 = cast<SPTSpan>(doc->getObjectById("tspan15"));
        ASSERT_NE(tspan15, nullptr);
        text2 = cast<SPText>(doc->getObjectById("text2"));
        ASSERT_NE(text2, nullptr);
        tspan21 = cast<SPTSpan>(doc->getObjectById("tspan21"));
        ASSERT_NE(tspan21, nullptr);
        tspan22 = cast<SPTSpan>(doc->getObjectById("tspan22"));
        ASSERT_NE(tspan22, nullptr);
        tspan23 = cast<SPTSpan>(doc->getObjectById("tspan23"));
        ASSERT_NE(tspan23, nullptr);
        layer2 = doc->getObjectById("layer2");
        ASSERT_NE(layer2, nullptr);
        text3 = cast<SPText>(doc->getObjectById("text3"));
        ASSERT_NE(text3, nullptr);
        tspan31 = cast<SPTSpan>(doc->getObjectById("tspan31"));
        ASSERT_NE(tspan31, nullptr);
        text4 = cast<SPText>(doc->getObjectById("text4"));
        ASSERT_NE(text4, nullptr);
        tspan41 = cast<SPTSpan>(doc->getObjectById("tspan41"));
        ASSERT_NE(tspan41, nullptr);

        // Run the initial update, so that we can properly check update propagation in tests.
        iterate_main_loop();
    }

    void TearDown() override { doc.reset(); }

    ~SPObjectLangTest() override = default;

protected:
    /**
     * @brief Run one iteration of the main loop.
     * Call this to wait for any parent->child propagation of the lang property.
     */
    static void iterate_main_loop()
    {
        // Iterate until no events are left
        while (main_loop->get_context()->iteration(false)) {
        }
    }

private:
    static Glib::RefPtr<Glib::MainLoop> main_loop;

protected:
    std::unique_ptr<SPDocument> doc;

    SPRoot *root;
    SPObject *layer1;
    SPText *text1;
    SPTSpan *tspan11;
    SPTSpan *tspan12;
    SPTSpan *tspan13;
    SPTSpan *tspan14;
    SPTSpan *tspan15;
    SPText *text2;
    SPTSpan *tspan21;
    SPTSpan *tspan22;
    SPTSpan *tspan23;
    SPObject *layer2;
    SPText *text3;
    SPTSpan *tspan31;
    SPText *text4;
    SPTSpan *tspan41;
};

Glib::RefPtr<Glib::MainLoop> SPObjectLangTest::main_loop{};

TEST_F(SPObjectLangTest, LangAttrStatic)
{
    // no lang
    EXPECT_STREQ(root->getLanguage(), "");
    EXPECT_EQ(root->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(layer1->getLanguage(), "");
    EXPECT_EQ(layer1->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(text1->getLanguage(), "");
    EXPECT_EQ(text1->getLangAttribute(), std::nullopt);

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    // `xml:lang` takes precedence over `lang` (`xml:lang` in front)
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // `xml:lang` takes precedence over `lang` (`lang` in front)
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    // no lang
    EXPECT_STREQ(tspan15->getLanguage(), "");
    EXPECT_EQ(tspan15->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text2->getLanguage(), "ja");
    EXPECT_EQ(text2->getLangAttribute(), "ja");

    // not inheriting from parent
    EXPECT_STREQ(tspan21->getLanguage(), "zh");
    EXPECT_EQ(tspan21->getLangAttribute(), "zh");

    // inheriting from parent
    EXPECT_STREQ(tspan22->getLanguage(), "ja");
    EXPECT_EQ(tspan22->getLangAttribute(), std::nullopt);

    // not inheriting from parent (empty string is still valid for lang)
    EXPECT_STREQ(tspan23->getLanguage(), "");
    EXPECT_EQ(tspan23->getLangAttribute(), "");

    // lang on non-text element
    EXPECT_STREQ(layer2->getLanguage(), "ko");
    EXPECT_EQ(layer2->getLangAttribute(), "ko");

    // inheriting from parent
    EXPECT_STREQ(text3->getLanguage(), "ko");
    EXPECT_EQ(text3->getLangAttribute(), std::nullopt);

    // inheriting from grandparent
    EXPECT_STREQ(tspan31->getLanguage(), "ko");
    EXPECT_EQ(tspan31->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text4->getLanguage(), "jp");
    EXPECT_EQ(text4->getLangAttribute(), "jp");

    // inheriting from parent
    EXPECT_STREQ(tspan41->getLanguage(), "jp");
    EXPECT_EQ(tspan41->getLangAttribute(), std::nullopt);
}

TEST_F(SPObjectLangTest, LangAttrDynamicAddToRoot)
{
    // Test adding lang to root:

    root->set(SPAttr::XML_LANG, "en");

    iterate_main_loop();

    // set explicitly
    EXPECT_STREQ(root->getLanguage(), "en");
    EXPECT_EQ(root->getLangAttribute(), "en");

    // inheriting from parent
    EXPECT_STREQ(layer1->getLanguage(), "en");
    EXPECT_EQ(layer1->getLangAttribute(), std::nullopt);

    // inheriting from grandparent
    EXPECT_STREQ(text1->getLanguage(), "en");
    EXPECT_EQ(text1->getLangAttribute(), std::nullopt);

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    // `xml:lang` takes precedence over `lang` (`xml:lang` in front)
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // `xml:lang` takes precedence over `lang` (`lang` in front)
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    // inheriting from great-grandparent
    EXPECT_STREQ(tspan15->getLanguage(), "en");
    EXPECT_EQ(tspan15->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text2->getLanguage(), "ja");
    EXPECT_EQ(text2->getLangAttribute(), "ja");

    // not inheriting from parent
    EXPECT_STREQ(tspan21->getLanguage(), "zh");
    EXPECT_EQ(tspan21->getLangAttribute(), "zh");

    // inheriting from parent
    EXPECT_STREQ(tspan22->getLanguage(), "ja");
    EXPECT_EQ(tspan22->getLangAttribute(), std::nullopt);

    // not inheriting from parent (empty string is still valid for lang)
    EXPECT_STREQ(tspan23->getLanguage(), "");
    EXPECT_EQ(tspan23->getLangAttribute(), "");

    // lang on non-text element
    EXPECT_STREQ(layer2->getLanguage(), "ko");
    EXPECT_EQ(layer2->getLangAttribute(), "ko");

    // inheriting from parent
    EXPECT_STREQ(text3->getLanguage(), "ko");
    EXPECT_EQ(text3->getLangAttribute(), std::nullopt);

    // inheriting from grandparent
    EXPECT_STREQ(tspan31->getLanguage(), "ko");
    EXPECT_EQ(tspan31->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text4->getLanguage(), "jp");
    EXPECT_EQ(text4->getLangAttribute(), "jp");

    // inheriting from parent
    EXPECT_STREQ(tspan41->getLanguage(), "jp");
    EXPECT_EQ(tspan41->getLangAttribute(), std::nullopt);
}

TEST_F(SPObjectLangTest, LangAttrDynamicAddToMidLevel)
{
    // Test adding lang to a mid-level element:

    text1->set(SPAttr::XML_LANG, "en");

    iterate_main_loop();

    // no lang
    EXPECT_STREQ(root->getLanguage(), "");
    EXPECT_EQ(root->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(layer1->getLanguage(), "");
    EXPECT_EQ(layer1->getLangAttribute(), std::nullopt);

    // set explicitly
    EXPECT_STREQ(text1->getLanguage(), "en");
    EXPECT_EQ(text1->getLangAttribute(), "en");

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    // `xml:lang` takes precedence over `lang` (`xml:lang` in front)
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // `xml:lang` takes precedence over `lang` (`lang` in front)
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    // inheriting from parent
    EXPECT_STREQ(tspan15->getLanguage(), "en");
    EXPECT_EQ(tspan15->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text2->getLanguage(), "ja");
    EXPECT_EQ(text2->getLangAttribute(), "ja");

    // not inheriting from parent
    EXPECT_STREQ(tspan21->getLanguage(), "zh");
    EXPECT_EQ(tspan21->getLangAttribute(), "zh");

    // inheriting from parent
    EXPECT_STREQ(tspan22->getLanguage(), "ja");
    EXPECT_EQ(tspan22->getLangAttribute(), std::nullopt);

    // not inheriting from parent (empty string is still valid for lang)
    EXPECT_STREQ(tspan23->getLanguage(), "");
    EXPECT_EQ(tspan23->getLangAttribute(), "");

    // lang on non-text element
    EXPECT_STREQ(layer2->getLanguage(), "ko");
    EXPECT_EQ(layer2->getLangAttribute(), "ko");

    // inheriting from parent
    EXPECT_STREQ(text3->getLanguage(), "ko");
    EXPECT_EQ(text3->getLangAttribute(), std::nullopt);

    // inheriting from grandparent
    EXPECT_STREQ(tspan31->getLanguage(), "ko");
    EXPECT_EQ(tspan31->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text4->getLanguage(), "jp");
    EXPECT_EQ(text4->getLangAttribute(), "jp");

    // inheriting from parent
    EXPECT_STREQ(tspan41->getLanguage(), "jp");
    EXPECT_EQ(tspan41->getLangAttribute(), std::nullopt);
}

TEST_F(SPObjectLangTest, LangAttrDynamicRemoveFromMidLevel)
{
    // Test removing lang from a mid-level element:

    text2->set(SPAttr::XML_LANG, nullptr);

    iterate_main_loop();

    // no lang
    EXPECT_STREQ(root->getLanguage(), "");
    EXPECT_EQ(root->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(layer1->getLanguage(), "");
    EXPECT_EQ(layer1->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(text1->getLanguage(), "");
    EXPECT_EQ(text1->getLangAttribute(), std::nullopt);

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    // `xml:lang` takes precedence over `lang` (`xml:lang` in front)
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // `xml:lang` takes precedence over `lang` (`lang` in front)
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    // no lang
    EXPECT_STREQ(tspan15->getLanguage(), "");
    EXPECT_EQ(tspan15->getLangAttribute(), std::nullopt);

    // removed
    EXPECT_STREQ(text2->getLanguage(), "");
    EXPECT_EQ(text2->getLangAttribute(), std::nullopt);

    // not inheriting from parent
    EXPECT_STREQ(tspan21->getLanguage(), "zh");
    EXPECT_EQ(tspan21->getLangAttribute(), "zh");

    // inheriting from parent
    EXPECT_STREQ(tspan22->getLanguage(), "");
    EXPECT_EQ(tspan22->getLangAttribute(), std::nullopt);

    // not inheriting from parent (empty string is still valid for lang)
    EXPECT_STREQ(tspan23->getLanguage(), "");
    EXPECT_EQ(tspan23->getLangAttribute(), "");

    // lang on non-text element
    EXPECT_STREQ(layer2->getLanguage(), "ko");
    EXPECT_EQ(layer2->getLangAttribute(), "ko");

    // inheriting from parent
    EXPECT_STREQ(text3->getLanguage(), "ko");
    EXPECT_EQ(text3->getLangAttribute(), std::nullopt);

    // inheriting from grandparent
    EXPECT_STREQ(tspan31->getLanguage(), "ko");
    EXPECT_EQ(tspan31->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text4->getLanguage(), "jp");
    EXPECT_EQ(text4->getLangAttribute(), "jp");

    // inheriting from parent
    EXPECT_STREQ(tspan41->getLanguage(), "jp");
    EXPECT_EQ(tspan41->getLangAttribute(), std::nullopt);
}

TEST_F(SPObjectLangTest, LangAttrDynamicRemoveFromMidLevel2)
{
    // Test removing lang from a mid-level element (with parent lang):

    text4->set(SPAttr::XML_LANG, nullptr);

    iterate_main_loop();

    // no lang
    EXPECT_STREQ(root->getLanguage(), "");
    EXPECT_EQ(root->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(layer1->getLanguage(), "");
    EXPECT_EQ(layer1->getLangAttribute(), std::nullopt);

    // no lang
    EXPECT_STREQ(text1->getLanguage(), "");
    EXPECT_EQ(text1->getLangAttribute(), std::nullopt);

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    // `xml:lang` takes precedence over `lang` (`xml:lang` in front)
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // `xml:lang` takes precedence over `lang` (`lang` in front)
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    // no lang
    EXPECT_STREQ(tspan15->getLanguage(), "");
    EXPECT_EQ(tspan15->getLangAttribute(), std::nullopt);

    // lang on text element
    EXPECT_STREQ(text2->getLanguage(), "ja");
    EXPECT_EQ(text2->getLangAttribute(), "ja");

    // not inheriting from parent
    EXPECT_STREQ(tspan21->getLanguage(), "zh");
    EXPECT_EQ(tspan21->getLangAttribute(), "zh");

    // inheriting from parent
    EXPECT_STREQ(tspan22->getLanguage(), "ja");
    EXPECT_EQ(tspan22->getLangAttribute(), std::nullopt);

    // not inheriting from parent (empty string is still valid for lang)
    EXPECT_STREQ(tspan23->getLanguage(), "");
    EXPECT_EQ(tspan23->getLangAttribute(), "");

    // lang on non-text element
    EXPECT_STREQ(layer2->getLanguage(), "ko");
    EXPECT_EQ(layer2->getLangAttribute(), "ko");

    // inheriting from parent
    EXPECT_STREQ(text3->getLanguage(), "ko");
    EXPECT_EQ(text3->getLangAttribute(), std::nullopt);

    // removed
    EXPECT_STREQ(tspan31->getLanguage(), "ko");
    EXPECT_EQ(tspan31->getLangAttribute(), std::nullopt);

    // inheriting from parent
    EXPECT_STREQ(text4->getLanguage(), "ko");
    EXPECT_EQ(text4->getLangAttribute(), std::nullopt);

    // inheriting from grandparent
    EXPECT_STREQ(tspan41->getLanguage(), "ko");
    EXPECT_EQ(tspan41->getLangAttribute(), std::nullopt);
}

TEST_F(SPObjectLangTest, LangAttrDynamicLangAndXmlLang)
{
    // Test interaction between `lang` and `xml:lang`:

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    tspan11->set(SPAttr::LANG, "en");

    iterate_main_loop();

    // `xml:lang` exists, `lang` is ignored
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // -

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    tspan12->set(SPAttr::XML_LANG, "en");

    iterate_main_loop();

    // `lang` is overridden by `xml:lang`
    EXPECT_STREQ(tspan12->getLanguage(), "en");
    EXPECT_EQ(tspan12->getLangAttribute(), "en");

    // -

    // `xml:lang` takes precedence over `lang`
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    tspan13->set(SPAttr::LANG, nullptr);

    iterate_main_loop();

    // `xml:lang` still valid
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // -

    // `xml:lang` takes precedence over `lang`
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    tspan14->set(SPAttr::XML_LANG, nullptr);

    iterate_main_loop();

    // `lang` now applies
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hant");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hant");
}

TEST_F(SPObjectLangTest, NodeReparentLangUpdate)
{
    // Test lang change after reparenting elements:

    // Reparent text1 to tree with xml:lang="ko":
    XML::Node *const repr = text1->getRepr();
    layer1->getRepr()->removeChild(repr);
    layer2->getRepr()->addChild(repr, nullptr);
    text1 = cast<SPText>(layer2->get_child_by_repr(repr));
    ASSERT_NE(text1, nullptr);
    tspan11 = cast<SPTSpan>(doc->getObjectById("tspan11"));
    ASSERT_NE(tspan11, nullptr);
    tspan12 = cast<SPTSpan>(doc->getObjectById("tspan12"));
    ASSERT_NE(tspan12, nullptr);
    tspan13 = cast<SPTSpan>(doc->getObjectById("tspan13"));
    ASSERT_NE(tspan13, nullptr);
    tspan14 = cast<SPTSpan>(doc->getObjectById("tspan14"));
    ASSERT_NE(tspan14, nullptr);
    tspan15 = cast<SPTSpan>(doc->getObjectById("tspan15"));
    ASSERT_NE(tspan15, nullptr);

    // inheriting from new parent
    EXPECT_STREQ(text1->getLanguage(), "ko");
    EXPECT_EQ(text1->getLangAttribute(), std::nullopt);

    // `xml:lang`
    EXPECT_STREQ(tspan11->getLanguage(), "zh-Hant-TW");
    EXPECT_EQ(tspan11->getLangAttribute(), "zh-Hant-TW");

    // `lang`
    EXPECT_STREQ(tspan12->getLanguage(), "zh-Hant-HK");
    EXPECT_EQ(tspan12->getLangAttribute(), "zh-Hant-HK");

    // `xml:lang` takes precedence over `lang` (`xml:lang` in front)
    EXPECT_STREQ(tspan13->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan13->getLangAttribute(), "zh-Hans");

    // `xml:lang` takes precedence over `lang` (`lang` in front)
    EXPECT_STREQ(tspan14->getLanguage(), "zh-Hans");
    EXPECT_EQ(tspan14->getLangAttribute(), "zh-Hans");

    // inheriting from new grandparent
    EXPECT_STREQ(tspan15->getLanguage(), "ko");
    EXPECT_EQ(tspan15->getLangAttribute(), std::nullopt);
}

TEST_F(SPObjectLangTest, SPObjectUpdateReprLangOutput)
{
    // Test updating the object repr:

    EXPECT_STREQ(tspan11->getRepr()->attribute("xml:lang"), "zh-Hant-TW");
    EXPECT_STREQ(tspan11->getRepr()->attribute("lang"), nullptr);

    tspan11->setLanguage("en");
    tspan11->updateRepr();
    EXPECT_STREQ(tspan11->getRepr()->attribute("xml:lang"), "en");
    EXPECT_STREQ(tspan11->getRepr()->attribute("lang"), nullptr);

    // -

    EXPECT_STREQ(tspan12->getRepr()->attribute("xml:lang"), nullptr);
    EXPECT_STREQ(tspan12->getRepr()->attribute("lang"), "zh-Hant-HK");

    tspan12->setLanguage("en");
    tspan12->updateRepr();
    EXPECT_STREQ(tspan12->getRepr()->attribute("xml:lang"), "en");
    EXPECT_STREQ(tspan12->getRepr()->attribute("lang"), "zh-Hant-HK");

    // -

    EXPECT_STREQ(tspan13->getRepr()->attribute("xml:lang"), "zh-Hans");
    EXPECT_STREQ(tspan13->getRepr()->attribute("lang"), "zh-Hant");

    tspan13->setLanguage(std::nullopt);
    tspan13->updateRepr();
    EXPECT_STREQ(tspan13->getRepr()->attribute("xml:lang"), nullptr);
    EXPECT_STREQ(tspan13->getRepr()->attribute("lang"), nullptr);
}

TEST_F(SPObjectLangTest, SPGlyph)
{
    // Test that the `lang` attribute of <glyph>/SPGlyph is not touched:

    XML::Node *repr = doc->getReprDoc()->createElement("svg:glyph");
    ASSERT_NE(repr, nullptr);
    repr->setAttribute("lang", "en,zh");
    repr->setAttribute("xml:lang", "ja");

    root->getRepr()->addChild(repr, nullptr);
    SPGlyph *glyph = cast<SPGlyph>(root->get_child_by_repr(repr));
    ASSERT_NE(glyph, nullptr);

    EXPECT_STREQ(glyph->lang, "en,zh");
    EXPECT_EQ(glyph->getLangAttribute(), "ja");

    glyph->setLanguage(std::nullopt);
    glyph->updateRepr();
    EXPECT_STREQ(repr->attribute("xml:lang"), nullptr);
    EXPECT_STREQ(repr->attribute("lang"), "en,zh");

    repr->setAttribute("lang", "zh,en");
    EXPECT_STREQ(glyph->lang, "zh,en");
    EXPECT_EQ(glyph->getLangAttribute(), std::nullopt);

    repr->setAttribute("xml:lang", "ja");
    EXPECT_STREQ(glyph->lang, "zh,en");
    EXPECT_EQ(glyph->getLangAttribute(), "ja");

    repr->removeAttribute("xml:lang");
    EXPECT_STREQ(glyph->lang, "zh,en");
    EXPECT_EQ(glyph->getLangAttribute(), std::nullopt);
}
