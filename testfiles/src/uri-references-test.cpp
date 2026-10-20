// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Unit tests for URIReference
 * @file
 * Test URI Reference from src/object/
 */

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "document.h"
#include "inkscape.h"
#include "inkscape-application.h"
#include "object/sp-root.h"
#include "object/uri-references.h"

using namespace std::literals;

class URIReferenceTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        if (!Inkscape::Application::exists()) {
            Inkscape::Application::create(false);
        }
    }

protected:
    std::unique_ptr<SPDocument> doc;
    SPObject *root = nullptr;

    void SetUp() override {
        constexpr auto svg_data = R"A(
            <svg xmlns="http://www.w3.org/2000/svg" id="root">
              <rect id="rect1" />
            </svg>
        )A"sv;

        auto span = std::span<char const>(svg_data.data(), svg_data.size());
        doc = SPDocument::createNewDocFromMem(span, "uri-ref-test.svg");

        ASSERT_NE(doc, nullptr) << "Failed to create test document";

        root = doc->getRoot();
        ASSERT_NE(root, nullptr);
    }
};

/*
* Helper: Intercepts GLib warnings and turns them into GTest failures
*/
void FailOnWarning(const gchar *log_domain, GLogLevelFlags log_level,
                   const gchar *message, gpointer user_data) {
    // Record a failure, DO NOT abort/crash the program
    ADD_FAILURE() << "Unexpected Warning: " << message;
}

/*
* Helper: If it matches "Malformed URI", it marks success.
* If it's something else it records a FAILURE.
*/
void ExpectMalformedOrFail(const gchar *log_domain, GLogLevelFlags log_level,
                           const gchar *message, gpointer user_data) {
    bool* saw_expected = static_cast<bool*>(user_data);

    if (std::string(message).find("Malformed URI") != std::string::npos) {
        *saw_expected = true;
    } else {
        // It was the WRONG/NO warning. Fail the test.
        ADD_FAILURE() << "Unexpected Warning received: " << message;
    }
}

/**
 * Test Case: Internal Links
 * Expectation: Returns TRUE and finds the object.
 */
TEST_F(URIReferenceTest, AcceptsInternalLinks)
{
    Inkscape::URIReference ref(root);

    bool result = ref.try_attach("#rect1");
    EXPECT_TRUE(result) << "try_attach should return true for valid internal ID";

    if (ref.isAttached()) {
        EXPECT_STREQ(ref.getObject()->getId(), "rect1");
    }
}

/**
 * Test Case: Web Links
 * Expectation: Returns FALSE (did not attach), but NO console warning.
 */
TEST_F(URIReferenceTest, SilencesUnsupportedURI)
{
    guint handler_id = g_log_set_handler(nullptr, G_LOG_LEVEL_WARNING, FailOnWarning, nullptr);

    Inkscape::URIReference ref(root);

    // HTTP
    bool result = ref.try_attach("http://inkscape.example/path/to/file.svg");
    EXPECT_FALSE(result) << "try_attach should return false silently for http";

    // HTTPS
    bool result_https = ref.try_attach("https://inkscape.example/path/to/file.svg");
    EXPECT_FALSE(result_https) << "try_attach should return false silently for https";

    g_log_remove_handler(nullptr, handler_id);
}

/**
 * Test Case: Malformed URIs
 * Expectation: Returns FALSE and REPORTS a console warning.
 */
TEST_F(URIReferenceTest, WarnsOnMalformed)
{
    Inkscape::URIReference ref(root);

    bool saw_expected = false;
    guint handler_id = g_log_set_handler(nullptr, G_LOG_LEVEL_WARNING, ExpectMalformedOrFail, &saw_expected);

    ref.try_attach("#xpointer(id(broken");
    g_log_remove_handler(nullptr, handler_id);

    EXPECT_TRUE(saw_expected) << "Test finished but expected 'Malformed URI' warning was never seen.";
}
