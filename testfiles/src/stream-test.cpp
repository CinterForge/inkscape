// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Stream IO tests
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2015-2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdio>
#include <gtest/gtest.h>
#include <string>

#include "io/stream/gzipstream.h"
#include "io/stream/inkscapestream.h"
#include "io/stream/stringstream.h"
#include "io/stream/uristream.h"
#include "io/stream/xsltstream.h"
#include "util/delete-with.h"

// names and path storage for other tests
auto const xmlpath = INKSCAPE_TESTS_DIR "/data/crystalegg.xml";
auto const xslpath = INKSCAPE_TESTS_DIR "/data/doc2html.xsl";

class MyFile
{
protected:
    std::string _filename;
    std::string _mode;

public:
    MyFile(std::string filename, char const *mode = "rb")
        : _filename(std::move(filename))
        , _mode(mode)
    {}

    FILE *open(char const *mode) const { return std::fopen(_filename.c_str(), mode); }

    operator FILE *() const { return open(_mode.c_str()); }

    std::string getContents() const
    {
        std::string buf;
        auto fp = Inkscape::Util::delete_with<std::fclose>(open("rb"));

        if (!fp) {
            ADD_FAILURE() << "failed to open " << _filename;
            exit(1);
        }

        for (int c; (c = std::fgetc(fp.get())) != EOF;) {
            buf.push_back(c);
        }

        return buf;
    }
};

class MyOutFile : public MyFile
{
public:
    MyOutFile(std::string filename)
        : MyFile("test_stream-out-" + filename, "wb")
    {}

    ~MyOutFile() { std::remove(_filename.c_str()); }
};

TEST(StreamTest, FileStreamCopy)
{
    auto inFile = MyFile(xmlpath);
    auto outFile = MyOutFile("streamtest.copy");
    {
        auto ins = Inkscape::IO::FileInputStream(inFile);
        auto outs = Inkscape::IO::FileOutputStream(outFile);
        pipeStream(ins, outs);
    }
    ASSERT_EQ(inFile.getContents(), outFile.getContents());
}

TEST(StreamTest, OutputStreamWriter)
{
    Inkscape::IO::StdOutputStream outs;
    Inkscape::IO::OutputStreamWriter writer(outs);
    writer << "Hello, world!  " << 123.45 << " times\n";
    writer.printf("There are %f quick brown foxes in %d states\n", 123.45, 88);
}

TEST(StreamTest, StdWriter)
{
    Inkscape::IO::StdWriter writer;
    writer << "Hello, world!  " << 123.45 << " times\n";
    writer.printf("There are %f quick brown foxes in %d states\n", 123.45, 88);
}

TEST(StreamTest, Xslt)
{
    // ######### XSLT Sheet ############
    auto xsltSheetFile = MyFile(xslpath);
    auto xsltSheetIns = Inkscape::IO::FileInputStream(xsltSheetFile);
    auto stylesheet = Inkscape::IO::XsltStyleSheet(xsltSheetIns);
    xsltSheetIns.close();
    auto sourceFile = MyFile(xmlpath);
    auto xmlIns = Inkscape::IO::FileInputStream(sourceFile);

    // ######### XSLT Input ############
    auto destFile = MyOutFile("test.html");
    auto xmlOuts = Inkscape::IO::FileOutputStream(destFile);
    auto xsltIns = Inkscape::IO::XsltInputStream(xmlIns, stylesheet);
    pipeStream(xsltIns, xmlOuts);
    xsltIns.close();
    xmlOuts.close();

    // ######### XSLT Output ############
    auto xmlIns2 = Inkscape::IO::FileInputStream(sourceFile);
    auto destFile2 = MyOutFile("test2.html");
    auto xmlOuts2 = Inkscape::IO::FileOutputStream(destFile2);
    auto xsltOuts = Inkscape::IO::XsltOutputStream(xmlOuts2, stylesheet);
    pipeStream(xmlIns2, xsltOuts);
    xmlIns2.close();
    xsltOuts.close();

    auto htmlContent = destFile.getContents();
    ASSERT_NE(htmlContent.find("<html"), std::string::npos);
    ASSERT_EQ(htmlContent, destFile2.getContents());
}

TEST(StreamTest, Gzip)
{
    auto sourceFile = MyFile(xmlpath);
    auto gzFile = MyOutFile("test.gz");
    auto destFile = MyOutFile("crystalegg2.xml");

    // ######### Gzip Output ############
    {
        auto sourceIns = Inkscape::IO::FileInputStream(sourceFile);
        auto gzOuts = Inkscape::IO::FileOutputStream(gzFile);
        auto gzipOuts = Inkscape::IO::GzipOutputStream(gzOuts);
        pipeStream(sourceIns, gzipOuts);
    }

    // ######### Gzip Input ############
    {
        auto gzIns = Inkscape::IO::FileInputStream(gzFile.open("rb"));
        auto destOuts = Inkscape::IO::FileOutputStream(destFile);
        auto gzipIns = Inkscape::IO::GzipInputStream(gzIns);
        pipeStream(gzipIns, destOuts);
    }

    ASSERT_EQ(sourceFile.getContents(), destFile.getContents());
}

TEST(StreamTest, GzipFExtraFComment)
{
    auto inFile = MyFile(INKSCAPE_TESTS_DIR "/data/example-FEXTRA-FCOMMENT.gz");
    auto inStream = Inkscape::IO::FileInputStream(inFile);
    auto inStreamGzip = Inkscape::IO::GzipInputStream(inStream);
    auto outStreamString = Inkscape::IO::StringOutputStream();
    pipeStream(inStreamGzip, outStreamString);
    ASSERT_EQ(outStreamString.getString(), "the content");
}
