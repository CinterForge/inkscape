// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Test extract_uri
 */
/*
 * Authors:
 *   Thomas Holder
 *
 * Copyright (C) 2018 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "util/uri.h"
#include <gtest/gtest.h>

TEST(ExtractUriTest, valid)
{
    ASSERT_EQ(extract_uri("url(#foo)"), "#foo");
    ASSERT_EQ(extract_uri("url( \t #foo \t )"), "#foo");
    ASSERT_EQ(extract_uri("url( '#foo' )"), "#foo");
    ASSERT_EQ(extract_uri("url('url(foo)')"), "url(foo)");
    ASSERT_EQ(extract_uri("url(\"foo(url)\")"), "foo(url)");
    ASSERT_EQ(extract_uri("url()bar"), "");
    ASSERT_EQ(extract_uri("url( )bar"), "");
    ASSERT_EQ(extract_uri("url(a b)"), "a b");
}

TEST(ExtractUriTest, legacy)
{
    ASSERT_EQ(extract_uri("url (foo)"), "foo");
}

TEST(ExtractUriTest, invalid)
{
    ASSERT_EQ(extract_uri("#foo"), "");
    ASSERT_EQ(extract_uri(" url(foo)"), "");
    ASSERT_EQ(extract_uri("url(#foo"), "");
    ASSERT_EQ(extract_uri("url('#foo'"), "");
    ASSERT_EQ(extract_uri("url('#foo)"), "");
    ASSERT_EQ(extract_uri("url #foo)"), "");
}

static char const *extract_end(char const *s)
{
    char const *end = nullptr;
    extract_uri(s, &end);
    return end;
}

TEST(ExtractUriTest, endptr)
{
    ASSERT_STREQ(extract_end(""), nullptr);
    ASSERT_STREQ(extract_end("url(invalid"), nullptr);
    ASSERT_STREQ(extract_end("url('invalid)"), nullptr);
    ASSERT_STREQ(extract_end("url(valid)"), "");
    ASSERT_STREQ(extract_end("url(valid)foo"), "foo");
    ASSERT_STREQ(extract_end("url('valid')bar"), "bar");
    ASSERT_STREQ(extract_end("url(  'valid'  )bar"), "bar");
    ASSERT_STREQ(extract_end("url(  valid  ) bar "), " bar ");
    ASSERT_STREQ(extract_end("url()bar"), "bar");
    ASSERT_STREQ(extract_end("url( )bar"), "bar");
}

TEST(ExtractUriTest, data_uri)
{
    // Adobe mime-type missing image
    {
        char const *data = "base64,ADOBE";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::RASTER);
        ASSERT_STREQ(now, "ADOBE");
    }
    // Data: already consumed
    {
        char const *data = "image/jpeg;base64,TRUE";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::RASTER);
        ASSERT_STREQ(now, "TRUE");
    }
    // Regular data uri
    {
        char const *data = "data:image/jpeg;base64,TRUE";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::RASTER);
        ASSERT_STREQ(now, "TRUE");
    }
    {
        char const *data = "data:image/svg+xml;base64,TRUE";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::SVG);
        ASSERT_STREQ(now, "TRUE");
    }
    {
        char const *data = "data:text/plain;base64,FALSE";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::NONE);
        ASSERT_STREQ(now, "FALSE");
    }
    {
        char const *data = "DaTa:iMaGe/pNg;bAsE64,IrReGuLaR";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::RASTER);
        ASSERT_STREQ(now, "IrReGuLaR");
    }
    {
        char const *data = "http://example.com/foo.png";
        auto [now, type] = extract_uri_data(data);
        ASSERT_TRUE(now);
        ASSERT_EQ(type, Base64Data::NONE);
        ASSERT_STREQ(now, "");
    }
}

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
