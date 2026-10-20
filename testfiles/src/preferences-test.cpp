// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Unit tests for the Preferences object
 *//*
 * Authors:
 * see git history
 *
 * Copyright (C) 2016-2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "preferences.h"

#include <gtest/gtest.h>

#include "colors/color.h"

// test observer
class TestObserver : public Inkscape::Preferences::Observer {
public:
    TestObserver(Glib::ustring const &path) :
        Inkscape::Preferences::Observer(path),
        value(0) {}

    virtual void notify(Inkscape::Preferences::Entry const &val)
    {
        value = val.getInt();
        value_str = val.getString();
        value_set = val.isSet();
    }
    int value;
    std::string value_str;
    bool value_set;
};

class PreferencesTest : public ::testing::Test
{
public:
    Inkscape::Preferences *prefs = nullptr;

protected:
    void SetUp() override {
        prefs = Inkscape::Preferences::get();
    }

    void TearDown() override {
        prefs = nullptr;
        Inkscape::Preferences::unload();
    }
};

TEST_F(PreferencesTest, testStartingState)
{
    ASSERT_TRUE(prefs);
    ASSERT_EQ(prefs->isWritable(), true);
}

TEST_F(PreferencesTest, testRemove)
{
    prefs->setString("/test/hello", "foo");
    ASSERT_EQ(prefs->getString("/test/hello"), "foo");
    prefs->remove("/test/hello");
    ASSERT_EQ(prefs->getString("/test/hello", "default"), "default");
    // empty string is not the same as removed:
    prefs->setString("/test/hello", "");
    // repeated twice to also test caching
    for (int i = 0; i < 1; i++) {
        ASSERT_EQ(prefs->getString("/test/hello", "default"), "");
    }
}

TEST_F(PreferencesTest, testOverwrite)
{
    prefs->setInt("/test/intvalue", 123);
    ASSERT_EQ(prefs->getInt("/test/intvalue"), 123);
    prefs->setInt("/test/intvalue", 321);
    ASSERT_EQ(prefs->getInt("/test/intvalue"), 321);
}

TEST_F(PreferencesTest, testHasPref)
{
    ASSERT_FALSE(prefs->hasPref("/test/value"));
    prefs->setInt("/test/value", 5);
    ASSERT_TRUE(prefs->hasPref("/test/value"));
}

TEST_F(PreferencesTest, testBoolFormat)
{
    ASSERT_TRUE(prefs->getBool("/test/boolvalue", true));
    ASSERT_FALSE(prefs->getBool("/test/boolvalue", false));
    prefs->setBool("/test/boolvalue", true);
    ASSERT_TRUE(prefs->getBool("/test/boolvalue", false));
    prefs->setBool("/test/boolvalue", false);
    ASSERT_FALSE(prefs->getBool("/test/boolvalue", true));
}

TEST_F(PreferencesTest, testOptionalBool)
{
    ASSERT_FALSE(prefs->getOptionalBool("/test/opboolvalue"));
    prefs->setBool("/test/opboolvalue", false);
    ASSERT_TRUE(prefs->getOptionalBool("/test/opboolvalue"));
    ASSERT_FALSE(*prefs->getOptionalBool("/test/opboolvalue"));
    prefs->setBool("/test/opboolvalue", true);
    ASSERT_TRUE(prefs->getOptionalBool("/test/opboolvalue"));
    ASSERT_TRUE(*prefs->getOptionalBool("/test/opboolvalue"));
}

TEST_F(PreferencesTest, testIntFormat)
{
    // test to catch thousand separators (wrong locale applied)
    prefs->setInt("/test/intvalue", 1'000'000);
    ASSERT_EQ(prefs->getInt("/test/intvalue"), 1'000'000);
}

TEST_F(PreferencesTest, testUIntFormat)
{
    prefs->setUInt("/test/uintvalue", 1'000'000u);
    ASSERT_EQ(prefs->getUInt("/test/uintvalue"), 1'000'000u);
}

TEST_F(PreferencesTest, testDblPrecision)
{
    const double VAL = 9.123456789; // 10 digits
    prefs->setDouble("/test/dblvalue", VAL);
    auto ret = prefs->getDouble("/test/dblvalue");
    ASSERT_NEAR(VAL, ret, 1e-9);
}

TEST_F(PreferencesTest, testDefaultReturn)
{
    // repeated twice to also test negative caching
    for (int i = 0; i < 1; i++) {
        ASSERT_EQ(prefs->getInt("/this/path/does/not/exist", 123), 123);
    }
}

TEST_F(PreferencesTest, testLimitedReturn)
{
    prefs->setInt("/test/intvalue", 1000);

    // simple case
    ASSERT_EQ(prefs->getIntLimited("/test/intvalue", 123, 0, 500), 123);
    // the below may seem quirky but this behaviour is intended
    ASSERT_EQ(prefs->getIntLimited("/test/intvalue", 123, 1001, 5000), 123);
    // corner cases
    ASSERT_EQ(prefs->getIntLimited("/test/intvalue", 123, 0, 1000), 1000);
    ASSERT_EQ(prefs->getIntLimited("/test/intvalue", 123, 1000, 5000), 1000);
}

TEST_F(PreferencesTest, testColor)
{
    const auto blue = Inkscape::Colors::Color::parse("blue").value();
    prefs->setColor("/test/colorvalue", blue);
    ASSERT_EQ(prefs->getColor("/test/colorvalue", "green"), blue);

    const auto red = Inkscape::Colors::Color::parse("red").value();
    prefs->setColor("/test/colorvalue", "red");
    ASSERT_EQ(prefs->getColor("/test/colorvalue", "green"), red);
}

TEST_F(PreferencesTest, testColorReadsUint)
{
    prefs->setUInt("/test/colorvalue", 2882400153);
    ASSERT_EQ(prefs->getColor("/test/colorvalue", "green").toString(), "#abcdef99");
}

TEST_F(PreferencesTest, testColorDefaultReturn)
{
    const auto green = Inkscape::Colors::Color::parse("green").value();
    ASSERT_EQ(prefs->getColor("/test/colorvalueNonExistent", "green"), green);
}

TEST_F(PreferencesTest, testIsValidBool)
{
    prefs->setBool("/test/boolvalue", true);
    ASSERT_TRUE(prefs->getEntry("/test/boolvalue").isValidBool());
    prefs->setString("/test/boolvalue", "invalid");
    ASSERT_FALSE(prefs->getEntry("/test/boolvalue").isValidBool());
}

TEST_F(PreferencesTest, testIsValidInt)
{
    prefs->setInt("/test/intvalue", 123);
    ASSERT_TRUE(prefs->getEntry("/test/intvalue").isValidInt());
    prefs->setString("/test/intvalue", "invalid");
    ASSERT_FALSE(prefs->getEntry("/test/intvalue").isValidInt());
    prefs->setString("/test/intvalue", "2147483647");
    ASSERT_TRUE(prefs->getEntry("/test/intvalue").isValidInt());
    prefs->setString("/test/intvalue", "2147483648");
    ASSERT_FALSE(prefs->getEntry("/test/intvalue").isValidInt());
    prefs->setString("/test/intvalue", "-2147483648");
    ASSERT_TRUE(prefs->getEntry("/test/intvalue").isValidInt());
    prefs->setString("/test/intvalue", "-2147483649");
    ASSERT_FALSE(prefs->getEntry("/test/intvalue").isValidInt());
}

TEST_F(PreferencesTest, testIsValidUInt)
{
    prefs->setUInt("/test/uintvalue", 123u);
    ASSERT_TRUE(prefs->getEntry("/test/uintvalue").isValidUInt());
    prefs->setString("/test/uintvalue", "-123");
    ASSERT_FALSE(prefs->getEntry("/test/uintvalue").isValidUInt());
    prefs->setString("/test/uintvalue", "4294967295");
    ASSERT_TRUE(prefs->getEntry("/test/uintvalue").isValidUInt());
    prefs->setString("/test/uintvalue", "4294967296");
    ASSERT_FALSE(prefs->getEntry("/test/uintvalue").isValidUInt());
    prefs->setString("/test/uintvalue", "-4294967296");
    ASSERT_FALSE(prefs->getEntry("/test/uintvalue").isValidUInt());
}

TEST_F(PreferencesTest, testIsValidDouble)
{
    prefs->setDouble("/test/doublevalue", 123.456);
    ASSERT_TRUE(prefs->getEntry("/test/doublevalue").isValidDouble());
    prefs->setString("/test/doublevalue", "invalid");
    ASSERT_FALSE(prefs->getEntry("/test/doublevalue").isValidDouble());
}

TEST_F(PreferencesTest, testIsValidColor)
{
    prefs->setColor("/test/colorvalue", Inkscape::Colors::Color::parse("blue").value());
    ASSERT_TRUE(prefs->getEntry("/test/colorvalue").isValidColor());
    prefs->setString("/test/colorvalue", "#2E3436ff");
    ASSERT_TRUE(prefs->getEntry("/test/colorvalue").isValidColor());
    prefs->setString("/test/colorvalue", "2882400153");
    ASSERT_TRUE(prefs->getEntry("/test/colorvalue").isValidColor());

    prefs->setString("/test/colorvalue", "pixels");
    ASSERT_FALSE(prefs->getEntry("/test/colorvalue").isValidColor());
}

TEST_F(PreferencesTest, testKeyObserverNotification)
{
    Glib::ustring const path = "/some/random/path";
    TestObserver obs("/some/random");
    obs.value = 1;
    prefs->setInt(path, 5);
    ASSERT_EQ(obs.value, 1); // no notifications sent before adding

    prefs->addObserver(obs);
    prefs->setInt(path, 10);
    ASSERT_EQ(obs.value, 10);
    ASSERT_TRUE(obs.value_set);
    prefs->setInt("/some/other/random/path", 42);
    ASSERT_EQ(obs.value, 10); // value should not change

    prefs->removeObserver(obs);
    prefs->setInt(path, 15);
    ASSERT_EQ(obs.value, 10); // no notifications sent after removal
}

/// Test PreferencesObserver when pref value is added / emptied / removed
TEST_F(PreferencesTest, testKeyObserverNotificationAddRemove)
{
    Glib::ustring const path = "/some/random/path";
    prefs->setInt("/some/random/whatever", 42);

    // Set up observer
    TestObserver obs("/some/random");
    prefs->addObserver(obs);

    // value is added (set for the first time)
    prefs->setInt(path, 10);
    ASSERT_EQ(obs.value, 10);
    ASSERT_TRUE(obs.value_set);

    // set to empty string --> observer should still receive a valid (but empty) entry
    prefs->setString(path, "");
    ASSERT_EQ(obs.value_str, "");
    ASSERT_EQ(obs.value, 0); // fallback value for int
    ASSERT_TRUE(obs.value_set);

    // remove preference --> observer should still receive a non-existing entry (isValid==false)
    prefs->remove(path);
    ASSERT_FALSE(obs.value_set);

    // Remove key and then set again.
    // In this case the observer may stop working.
    // This limitation is documented in Preferences::addObserver.
    prefs->remove("/some/random");
    obs.value = 1234;
    prefs->setInt(path, 15);
    // Ideal result:
    // ASSERT_EQ(obs.value, 15);
    // ASSERT_TRUE(obs.value_valid);
    // Due to the above limitations: Observer is never notified
    ASSERT_EQ(obs.value, 1234);

    prefs->removeObserver(obs);
}

TEST_F(PreferencesTest, testEntryObserverNotificationAddRemove)
{
    Glib::ustring const path = "/some/random/path";
    prefs->setInt(path, 2);

    TestObserver obs(path);
    obs.value = 1;
    prefs->setInt(path, 5);
    ASSERT_EQ(obs.value, 1); // no notifications sent before adding

    prefs->addObserver(obs);
    prefs->setInt(path, 10);
    ASSERT_TRUE(obs.value_set);
    ASSERT_EQ(obs.value, 10);

    // empty string (not the same as removed)
    prefs->setString(path, "");
    ASSERT_TRUE(obs.value_set);
    ASSERT_EQ(obs.value_str, "");
    ASSERT_EQ(obs.value, 0); // fallback value for int conversion

    prefs->setInt(path, 15);
    ASSERT_EQ(obs.value, 15);

    prefs->remove(path);
    ASSERT_FALSE(obs.value_set);

    // Note: Here we are re-adding a removed preference.
    // The observer still works, but would also be allowed to fail, see Preferences::addObserver.
    prefs->setInt(path, 20);
    ASSERT_EQ(obs.value, 20);

    prefs->removeObserver(obs);
    prefs->setInt(path, 25);
    ASSERT_EQ(obs.value, 20); // no notifications sent after removal
}

TEST_F(PreferencesTest, testEntryObserverNotification)
{
    Glib::ustring const path = "/some/random/path";
    TestObserver obs(path);
    obs.value = 1;
    prefs->setInt(path, 5);
    ASSERT_EQ(obs.value, 1); // no notifications sent before adding

    prefs->addObserver(obs);
    prefs->setInt(path, 10);
    ASSERT_EQ(obs.value, 10);

    // test that filtering works properly
    prefs->setInt("/some/random/value", 1234);
    ASSERT_EQ(obs.value, 10);
    prefs->setInt("/some/randomvalue", 1234);
    ASSERT_EQ(obs.value, 10);
    prefs->setInt("/some/random/path2", 1234);
    ASSERT_EQ(obs.value, 10);

    prefs->removeObserver(obs);
    prefs->setInt(path, 15);
    ASSERT_EQ(obs.value, 10); // no notifications sent after removal
}

TEST_F(PreferencesTest, testPreferencesEntryMethods)
{
    prefs->setInt("/test/prefentry", 100);
    Inkscape::Preferences::Entry val = prefs->getEntry("/test/prefentry");
    ASSERT_TRUE(val.isSet());
    ASSERT_EQ(val.getPath(), "/test/prefentry");
    ASSERT_EQ(val.getEntryName(), "prefentry");
    ASSERT_EQ(val.getInt(), 100);
}

TEST_F(PreferencesTest, testTemporaryPreferences)
{
    auto pref = "/test/prefentry";
    prefs->setInt(pref, 100);
    ASSERT_EQ(prefs->getInt(pref), 100);
    {
        auto transaction = prefs->temporaryPreferences();
        prefs->setInt(pref, 200);
        ASSERT_EQ(prefs->getInt(pref), 200);
        {
            auto sub_transaction = prefs->temporaryPreferences();
            prefs->setInt(pref, 300);
            ASSERT_EQ(prefs->getInt(pref), 300);
        }
        // This doesn't change because only one guard can exist in the stack at one time.
        ASSERT_EQ(prefs->getInt(pref), 300);
    }
    ASSERT_EQ(prefs->getInt(pref), 100);
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
