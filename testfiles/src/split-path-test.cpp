// SPDX-License-Identifier: GPL-2.0-or-later

#include "io/split-path.h"

#include <gtest/gtest.h>

namespace Inkscape {

TEST(SplitPathTest, SplitPath)
{
#ifdef _WIN32
    {
        auto path = "";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::Empty);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{}));
        EXPECT_EQ(parts.join(), "");
    }
    {
        auto path = ".\\";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::RelativeCWD);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{}));
        EXPECT_EQ(parts.join(), ".");
    }
    {
        auto path = "\\files\\test.txt";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::RelativeCurrentDriveRoot);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"files", "test.txt"}));
        EXPECT_EQ(parts.join(), "\\files\\test.txt");
    }
    {
        auto path = "D:..\\stuff\\../..\\file.txt";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::RelativeDriveCWD);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"D:", "..", "..", "file.txt"}));
        EXPECT_EQ(parts.join(), "D:..\\..\\file.txt");
    }
    {
        auto path = "LPT2";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::LegacyDevice);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"LPT2"}));
        EXPECT_EQ(parts.join(), "LPT2");
    }
    {
        auto path = "LPT22.txt";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::RelativeCWD);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"LPT22.txt"}));
        EXPECT_EQ(parts.join(), ".\\LPT22.txt");
    }
    {
        auto path = "C:\\images\\..\\こんにちは/\\.\\\\file.svg";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::FullyQualified);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"C:", "こんにちは", "file.svg"}));
        EXPECT_EQ(parts.join(), "C:\\こんにちは\\file.svg");
    }
    {
        auto path = "\\\\192.168.0.100\\Files\\..\\hello.txt";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::UNC);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"192.168.0.100", "Files", "hello.txt"}));
        EXPECT_EQ(parts.join(), "\\\\192.168.0.100\\Files\\hello.txt");
    }
    {
        auto path = "\\\\.\\C:\\test.txt";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::Device);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"C:", "test.txt"}));
        EXPECT_EQ(parts.join(), "\\\\.\\C:\\test.txt");
    }
    {
        auto path = "\\\\.\\UNC\\127.0.0.1\\Misc/Items\\README.md";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::DeviceUNC);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"127.0.0.1", "Misc", "Items", "README.md"}));
        EXPECT_EQ(parts.join(), "\\\\.\\UNC\\127.0.0.1\\Misc\\Items\\README.md");
    }
    {
        auto path = "\\\\?\\C:\\..\\foo/test.txt";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::DeviceNormalized);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"C:", "..", "foo/test.txt"}));
        EXPECT_EQ(parts.join(), "\\\\?\\C:\\..\\foo/test.txt");
    }
    {
        auto path = "\\\\?\\UNC\\10.0.0.1\\MyShare\\asdf.svg";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::DeviceNormalizedUNC);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"10.0.0.1", "MyShare", "asdf.svg"}));
        EXPECT_EQ(parts.join(), "\\\\?\\UNC\\10.0.0.1\\MyShare\\asdf.svg");
    }
#else // no _WIN32
    {
        auto path = "";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::Empty);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{}));
        EXPECT_EQ(parts.join(), "");
    }
    {
        auto path = "/";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::Absolute);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{}));
        EXPECT_EQ(parts.join(), "/");
    }
    {
        auto path = "./";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::RelativeCWD);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{}));
        EXPECT_EQ(parts.join(), ".");
    }
    {
        auto path = "my file";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::RelativeCWD);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"my file"}));
        EXPECT_EQ(parts.join(), "./my file");
    }
    {
        auto path = "/home/user/../こんにちは/.//file.svg";
        auto parts = Inkscape::IO::split_path(path);
        EXPECT_EQ(parts.type, Inkscape::IO::PathType::Absolute);
        EXPECT_EQ(parts.data, (std::vector<std::string_view>{"home", "こんにちは", "file.svg"}));
        EXPECT_EQ(parts.join(), "/home/こんにちは/file.svg");
    }
#endif
}

} // namespace Inkscape
