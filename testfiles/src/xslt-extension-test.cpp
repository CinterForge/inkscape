// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file @brief
 * Technical tests for the implementation of the XSLT extension.
 *
 * Authors:
 *   Rafał Siejakowski <rs@rs-math.net>
 *
 * @copyright
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "extension/dependency.h"
#include "extension/extension.h"
#include "extension/implementation/xslt.h"
#include "extension/prefdialog/widget.h"
#include "extension/processing-action.h"
#include "extension/timer.h"
#include "gc-anchored.h"
#include "testfiles/src/mocks/xml-document-mock.h"
#include "testfiles/src/mocks/xml-node-mock.h"

namespace Glib {
int file_open_tmp(std::string &, std::string const &)
{
    return 0;
}
} // namespace Glib

// We do not need a real SPDocument for this test
class SPDocument
{
public:
    ~SPDocument();

    static std::unique_ptr<SPDocument> createDoc(Inkscape::XML::Document *, char const *, char const *, char const *,
                                                 SPDocument * = nullptr);
    static void rebase(Inkscape::XML::Document *, bool keep_namedview = true);
};
SPDocument::~SPDocument() {}

std::unique_ptr<SPDocument> SPDocument::createDoc(Inkscape::XML::Document *, char const *, char const *, char const *,
                                                  SPDocument *)
{
    return {};
}

void SPDocument::rebase(Inkscape::XML::Document *, bool keep_namedview)
{
    return;
}

bool sp_repr_save_rebased_file(Inkscape::XML::Document *, gchar const *const, gchar const *, gchar const *,
                               gchar const *)
{
    return true;
}

namespace {
struct MockStatics
{
    MockStatics() { instance = this; }
    ~MockStatics() { instance = nullptr; }

    MOCK_CONST_METHOD0(sp_repr_do_read, Inkscape::XML::Document *());
    MOCK_CONST_METHOD0(sp_repr_read_file, Inkscape::XML::Document *());

    inline static MockStatics *instance = nullptr;
};
} // namespace

Inkscape::XML::Document *sp_repr_do_read(xmlDocPtr, gchar const *)
{
    return MockStatics::instance->sp_repr_do_read();
}

Inkscape::XML::Document *sp_repr_read_file(char const*, char const*, bool)
{
    return MockStatics::instance->sp_repr_read_file();
}

/* Mock implementation of the XML and XSLT library functions.
 * These implementation model the expected memory management conventions:
 * functions which in the real library return "owning pointers" are implemented
 * with `return new`, and the cleanup functions call `delete`.
 */
void xmlCleanupParser() {}

xsltStylesheetPtr xsltParseStylesheetDoc(xmlDocPtr take_ownership)
{
    delete take_ownership;
    return new xsltStylesheet{};
}

void xmlFreeDoc(xmlDocPtr cur)
{
    delete cur;
}
xmlDocPtr xmlParseFile(char const *)
{
    return new xmlDoc{};
}

extern "C" {
xmlDocPtr xsltApplyStylesheet(xsltStylesheetPtr, xmlDocPtr, char const **)
{
    return new xmlDoc;
}

void xsltFreeStylesheet(xsltStylesheetPtr style)
{
    delete style;
}

int xsltSaveResultToFilename(char const *, xmlDocPtr, xsltStylesheetPtr, int)
{
    return 1;
}
void xsltCleanupGlobals() {}
}

namespace Inkscape {

using namespace ::testing;

// Do not use the Garbage Collector in this test
void GC::Anchored::anchor() const {}
namespace Extension {
struct MockXSLTExtension : public Extension
{
    MockXSLTExtension()
        : Extension(nullptr, {}, nullptr)
    {
        EXPECT_CALL(mockRoot, firstChild()).WillRepeatedly(Return(&mockXSLTNode));
        EXPECT_CALL(mockXSLTNode, name()).WillRepeatedly(Return("extension:xslt"));
        EXPECT_CALL(mockXSLTNode, firstChild()).WillRepeatedly(Return(&mockFileNode));
        EXPECT_CALL(mockFileNode, name()).WillRepeatedly(Return("extension:file"));
        EXPECT_CALL(mockFileNode, firstChild()).WillRepeatedly(Return(&mockFileNameTextNode));
        EXPECT_CALL(mockFileNameTextNode, content()).WillRepeatedly(Return("non-existent-file"));
    }
    MOCK_METHOD0(m_loaded, bool());

    XML::Mock::Node mockRoot;
    XML::Mock::Node mockXSLTNode;
    XML::Mock::Node mockFileNode;
    XML::Mock::Node mockFileNameTextNode;
};

Extension::Extension(XML::Node *, ImplementationHolder, std::string *) {}

Extension::~Extension() {}

void Extension::paramListString(std::list<std::string> &) const {}
std::string Extension::get_dependency_location(char const *)
{
    return {};
}
bool Extension::check()
{
    return true;
}

void Extension::deactivate() {}

bool Extension::loaded()
{
    return static_cast<MockXSLTExtension *>(this)->m_loaded();
}
XML::Node *Extension::get_repr()
{
    return &static_cast<MockXSLTExtension *>(this)->mockRoot;
}

bool Extension::prefs()
{
    return false;
}

namespace Implementation {

std::unique_ptr<SPDocument> Implementation::new_from_template(Template *)
{
    return {};
}

std::unique_ptr<SPDocument> Implementation::open(Input *, char const *, bool)
{
    return {};
}

void Implementation::effect(Effect *, ExecutionEnv *, SPDesktop *, ImplementationDocumentCache *) {}
Gtk::Widget *Implementation::prefs_effect(Inkscape::Extension::Effect *, SPDesktop *, sigc::signal<void()> *,
                                          ImplementationDocumentCache *)
{
    return nullptr;
}

struct XSLTExtensionTest : public Test
{
    MockStatics mock;
};

TEST_F(XSLTExtensionTest, DoNotLeakMemoryOnSuccessfulLoad)
{
    MockXSLTExtension mockModule;

    XSLT testXslt;
    EXPECT_CALL(mockModule, m_loaded()).Times(1).WillRepeatedly(Return(false)).RetiresOnSaturation();
    testXslt.load(&mockModule);
    EXPECT_CALL(mockModule, m_loaded()).Times(AtLeast(1)).WillRepeatedly(Return(true));
    testXslt.unload(&mockModule);
}

TEST_F(XSLTExtensionTest, DoNotLeakMemoryOnFailedlLoad)
{
    MockXSLTExtension mockModule;

    XSLT testXslt;
    EXPECT_CALL(mockModule, m_loaded()).Times(AtLeast(1)).WillRepeatedly(Return(false)).RetiresOnSaturation();
    testXslt.load(&mockModule);
    testXslt.unload(&mockModule);
}

TEST_F(XSLTExtensionTest, DoNotLeakMemoryOnOpen)
{
    XML::Mock::Document mockDoc;
    XML::Mock::Node mockRoot;
    EXPECT_CALL(mock, sp_repr_do_read()).WillOnce(Return(&mockDoc));
    EXPECT_CALL(mockDoc.asNode(), root()).WillRepeatedly(Return(&mockRoot));
    EXPECT_CALL(mockRoot, name()).WillRepeatedly(Return("svg:svg"));

    XSLT().open(nullptr, "fake_filename", false);
}
} // namespace Implementation
} // namespace Extension
} // namespace Inkscape
