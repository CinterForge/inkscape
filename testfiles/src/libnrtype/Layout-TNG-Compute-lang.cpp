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

#include <cstdio>
#include <glib.h>
#include <gtest/gtest.h>
#include <memory>

#include "inkscape.h"
#include "libnrtype/Layout-TNG-Compute.h"
#include "object/sp-object.h"

#define DEBUG_PANGO_ITEMIZATION

using namespace Inkscape;
using namespace Inkscape::Text;

struct PangoDeleter
{
    void operator()(PangoAttrList *p) const { pango_attr_list_unref(p); }
    void operator()(PangoFontDescription *p) const { pango_font_description_free(p); }
    void operator()(char *p) const { g_free(p); }
};

template <typename T>
using pango_ptr = std::unique_ptr<T, PangoDeleter>;

static char const *get_script_name(GUnicodeScript s)
{
    GEnumClass *class_;
    GEnumValue *value;
    char const *nick;

    class_ = (GEnumClass *)g_type_class_ref(g_unicode_script_get_type());
    value = g_enum_get_value(class_, s);
    nick = value->value_nick;
    g_type_class_unref(class_);
    return nick;
}

static void print_attribute(PangoAttribute *attr)
{
    pango_ptr<PangoAttrList> l{pango_attr_list_new()};
    pango_attr_list_insert(l.get(), pango_attribute_copy(attr));
    pango_ptr<char[]> s{pango_attr_list_to_string(l.get())};
    printf("%s", s.get());
}

class LibnrtypeLayoutTNGComputeTest : public ::testing::Test
{
public:
    static void SetUpTestSuite() { Inkscape::Application::create(false); }

    ~LibnrtypeLayoutTNGComputeTest() override = default;

protected:
    void testLangAttribute();
};

TEST_F(LibnrtypeLayoutTNGComputeTest, TestLangAttribute)
{
    testLangAttribute();
}

void LibnrtypeLayoutTNGComputeTest::testLangAttribute()
{
    SPObject object1;
    SPObject object2;
    object2.setLanguage("en");
    SPObject object3;
    SPObject object4;
    object4.setLanguage("zh-Hant-HK");
    SPObject object5;
    Glib::ustring text1 = "text1";
    Glib::ustring text2 = "text2";
    Glib::ustring text3 = "text3";
    Glib::ustring text4 = "漢字";
    Glib::ustring text5 = "text5";

    Text::Layout layout;
    layout.wrap_mode = Inkscape::Text::Layout::WRAP_NONE;
    layout.appendText(text1, object1.style, &object1);
    layout.appendText(text2, object2.style, &object2);
    layout.appendText(text3, object3.style, &object3);
    layout.appendText(text4, object3.style, &object4);
    layout.appendText(text5, object3.style, &object5);

    Layout::Calculator calc = Layout::Calculator(&layout);

    // HACK: Call `Calculator::calculate` to run all the logic required before testing
    // `Calculator::_buildPangoItemizationForPara`
    ASSERT_TRUE(calc.calculate());

    Layout::Calculator::ParagraphInfo para{};
    para.first_input_index = 0;
    calc._buildPangoItemizationForPara(&para);

    // Print Pango items for debugging
    for (Layout::Calculator::PangoItemInfo const &pango_item : para.pango_items) {
        PangoItem *item = pango_item.item;

        pango_ptr<PangoFontDescription> desc{pango_font_describe(item->analysis.font)};
        pango_ptr<char[]> font_name{pango_font_description_to_string(desc.get())};

        printf("Item: %d (+%d)\n", item->offset, item->length);
        printf("Font: %s\n", font_name.get());
        printf("Script: %s\n", get_script_name((GUnicodeScript)item->analysis.script));
        printf("Lang: %s\n", pango_language_to_string(item->analysis.language));
        printf("Bidi: %d\n", item->analysis.level);
        printf("Attrs: ");
        for (GSList *a = item->analysis.extra_attrs; a; a = a->next) {
            PangoAttribute *attr = (PangoAttribute *)a->data;
            print_attribute(attr);
            printf(",");
        }
        printf("\nChars: %d\n\n", item->num_chars);
    }

    ASSERT_EQ(para.pango_items.size(), 5);

    EXPECT_EQ(para.pango_items[0].item->offset, 0);
    EXPECT_EQ(para.pango_items[0].item->length, 5);
    EXPECT_EQ(para.pango_items[0].item->analysis.script, G_UNICODE_SCRIPT_LATIN);
    EXPECT_STREQ(pango_language_to_string(para.pango_items[0].item->analysis.language), "und");

    EXPECT_EQ(para.pango_items[1].item->offset, 5);
    EXPECT_EQ(para.pango_items[1].item->length, 5);
    EXPECT_EQ(para.pango_items[1].item->analysis.script, G_UNICODE_SCRIPT_LATIN);
    EXPECT_STREQ(pango_language_to_string(para.pango_items[1].item->analysis.language), "en");

    EXPECT_EQ(para.pango_items[2].item->offset, 10);
    EXPECT_EQ(para.pango_items[2].item->length, 5);
    EXPECT_EQ(para.pango_items[2].item->analysis.script, G_UNICODE_SCRIPT_LATIN);
    EXPECT_STREQ(pango_language_to_string(para.pango_items[2].item->analysis.language), "und");

    EXPECT_EQ(para.pango_items[3].item->offset, 15);
    EXPECT_EQ(para.pango_items[3].item->length, 6);
    EXPECT_EQ(para.pango_items[3].item->analysis.script, G_UNICODE_SCRIPT_HAN);
    EXPECT_STREQ(pango_language_to_string(para.pango_items[3].item->analysis.language), "zh-hant-hk");

    EXPECT_EQ(para.pango_items[4].item->offset, 21);
    EXPECT_EQ(para.pango_items[4].item->length, 5);
    EXPECT_EQ(para.pango_items[4].item->analysis.script, G_UNICODE_SCRIPT_LATIN);
    EXPECT_STREQ(pango_language_to_string(para.pango_items[4].item->analysis.language), "und");
}
