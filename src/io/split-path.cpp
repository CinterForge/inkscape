// SPDX-License-Identifier: GPL-2.0-or-later

#include "split-path.h"

#include <algorithm>
#include <ranges>
#include <string_view>
#include <vector>

namespace Inkscape::IO {

using std::operator""sv;
using Range = std::ranges::subrange<char const *>;

void action_relative(Range r, std::vector<std::string_view> &parts)
{
    if (parts.empty() || parts.back() == ".."sv) {
        parts.emplace_back(r.begin(), std::ranges::distance(r)); // preserve '..'
    } else {
        parts.pop_back(); // apply '..'
    }
}

void action_absolute(Range _r, std::vector<std::string_view> &parts)
{
    if (!parts.empty()) {
        parts.pop_back(); // apply '..'
    }
}

void action_noop(Range _r, std::vector<std::string_view> &parts) {}

#ifdef _WIN32

void action_fully_qualified(Range _r, std::vector<std::string_view> &parts)
{
    if (parts.size() > 1) { // preserve drive letter
        parts.pop_back();   // apply '..'
    }
}

void action_unc(Range _r, std::vector<std::string_view> &parts)
{
    if (parts.size() > 2) { // preserve server and share
        parts.pop_back();   // apply '..'
    }
}

void action_relative_drive_root(Range r, std::vector<std::string_view> &parts)
{
    if (parts.size() <= 1 || parts.back() == ".."sv) {
        parts.emplace_back(r.begin(), std::ranges::distance(r)); // preserve '..'
    } else {
        parts.pop_back(); // apply '..'
    }
}

void (*SPLIT_PATH_PARENT_ACTIONS[11])(Range, std::vector<std::string_view> &) = {
    action_noop,
    action_noop,
    action_noop,
    action_noop,
    action_unc,
    action_unc,
    action_fully_qualified,
    action_fully_qualified,
    action_relative_drive_root,
    action_absolute,
    action_relative,
};

bool starts_with_separator(std::string_view path)
{
    return path.starts_with('\\') || path.starts_with('/');
}

PathType get_path_type(std::string_view path)
{
    if (path.empty()) {
        return PathType::Empty;
    }
    // drive letter
    if (path.substr(1).starts_with(':')) {
        if (starts_with_separator(path.substr(2))) {
            return PathType::FullyQualified;
        }
        return PathType::RelativeDriveCWD;
    }
    // match without normalization
    if (path.starts_with('\\')) {
        if (path.substr(1).starts_with('\\')) {
            if (path.substr(2).starts_with("?\\"sv)) {
                if (path.substr(4).starts_with("UNC\\"sv)) {
                    return PathType::DeviceNormalizedUNC;
                }
                return PathType::DeviceNormalized;
            }
            return PathType::UNC;
        }
        return PathType::RelativeCurrentDriveRoot;
    }
    // match with normalization
    if (starts_with_separator(path)) {
        if (starts_with_separator(path.substr(1))) {
            if (path.substr(2).starts_with('.')) {
                if (starts_with_separator(path.substr(3))) {
                    if (path.substr(4).starts_with("UNC"sv)) {
                        if (starts_with_separator(path.substr(7))) {
                            return PathType::DeviceUNC;
                        }
                    }
                    return PathType::Device;
                }
            }
            return PathType::UNC;
        }
        return PathType::RelativeCurrentDriveRoot;
    }
    // legacy device
    if (path.size() == 4) {
        if (path.starts_with("COM"sv) || path.starts_with("LPT"sv)) {
            if (std::isdigit(path.back()) > 0) {
                return PathType::LegacyDevice;
            }
        }
    }
    if (path.size() == 3) {
        if (path == "CON"sv || path == "AUX"sv || path == "PRN"sv || path == "NUL"sv) {
            return PathType::LegacyDevice;
        }
    }
    return PathType::RelativeCWD;
}

size_t get_path_type_offset(PathType type)
{
    switch (type) {
        case PathType::FullyQualified:
            return 3;
        case PathType::RelativeCWD:
        case PathType::LegacyDevice:
        case PathType::Empty:
            return 0;
        case PathType::RelativeCurrentDriveRoot:
            return 1;
        case PathType::UNC:
        case PathType::RelativeDriveCWD:
            return 2;
        case PathType::Device:
        case PathType::DeviceNormalized:
            return 4;
        case PathType::DeviceUNC:
        case PathType::DeviceNormalizedUNC:
            return 8;
        default:
            return 0;
    }
}

bool PathParts::is_relative() const
{
    switch (type) {
        case PathType::RelativeCWD:
        case PathType::RelativeCurrentDriveRoot:
        case PathType::RelativeDriveCWD:
            return true;
        default:
            return false;
    }
}

/**
 * Return the prefix that is implicitely stored in the path type
 */
std::string_view PathParts::prefix() const
{
    switch (type) {
        case PathType::FullyQualified:
        case PathType::RelativeDriveCWD:
        case PathType::LegacyDevice:
            return data[0];
        case PathType::UNC:
            return R"(\)";
        case PathType::Device:
            return R"(\\.)";
        case PathType::DeviceNormalized:
            return R"(\\?)";
        case PathType::DeviceUNC:
            return R"(\\.\UNC)";
        case PathType::DeviceNormalizedUNC:
            return R"(\\?\UNC)";
        case PathType::RelativeCWD:
            return ".";
        default:
            return std::string_view{};
    }
}

size_t PathParts::get_join_size() const
{
    auto size = data.size();
    switch (type) {
        case PathType::FullyQualified:
            size -= 1;
            break;
        case PathType::RelativeDriveCWD:
            size -= 2;
            break;
        case PathType::Device:
        case PathType::DeviceNormalized:
            size += 3;
            break;
        case PathType::DeviceUNC:
        case PathType::DeviceNormalizedUNC:
            size += 7;
            break;
        case PathType::UNC:
        case PathType::RelativeCWD:
            size += 1;
            break;
        case PathType::LegacyDevice:
            size = 0;
            break;
        default:;
    }
    for (auto const sv : data) {
        size += sv.size();
    }
    return size;
}

std::string PathParts::join() const
{
    if (data.empty()) {
        switch (type) {
            case PathType::Empty:
                return "";
            case PathType::RelativeCWD:
                return ".";
            case PathType::RelativeCurrentDriveRoot:
                return R"(\)";
            case PathType::UNC:
                return R"(\\)";
            case PathType::Device:
                return R"(\\.\)";
            case PathType::DeviceUNC:
                return R"(\\.\UNC\)";
            case PathType::DeviceNormalized:
                return R"(\\?\)";
            case PathType::DeviceNormalizedUNC:
                return R"(\\?\UNC\)";
        }
    }
    std::string result;
    result.reserve(get_join_size());
    result.append(prefix());
    std::span<std::string_view const> items;
    switch (type) {
        case PathType::FullyQualified:
            items = std::span(data).subspan(1);
            break;
        case PathType::RelativeDriveCWD:
            if (data.size() >= 2) {
                result.append(data[1]);
            }
            items = std::span(data).subspan(2);
            break;
        case PathType::LegacyDevice:
            return result;
        default:
            items = std::span(data);
    }
    for (auto const sv : items) {
        result.push_back('\\');
        result.append(sv);
    }
    return result;
}

void split_path(std::string_view path, std::vector<std::string_view> &parts)
{
    auto it = path | std::views::split("\\"sv);
    for (auto r : it) {
        parts.emplace_back(r.begin(), std::ranges::distance(r));
    }
}

void split_path(std::string_view path, std::vector<std::string_view> &parts, PathType type)
{
    auto tran = [](auto r) { return std::views::split(r, "/"sv); };
    auto pred = [](auto r) { return !(r.empty() || std::ranges::equal(r, "."sv)); };
    auto it =
        path | std::views::split("\\"sv) | std::views::transform(tran) | std::views::join | std::views::filter(pred);
    auto parent_action = SPLIT_PATH_PARENT_ACTIONS[static_cast<int>(type)];
    for (auto r : it) {
        if (std::ranges::equal(r, ".."sv)) {
            parent_action(r, parts);
        } else {
            parts.emplace_back(r.begin(), std::ranges::distance(r));
        }
    }
}

PathParts split_path(std::string_view path)
{
    auto type = get_path_type(path);
    if (type == PathType::Empty) {
        return {};
    } else if (type == PathType::LegacyDevice) {
        return {.data = {path}, .type = type};
    }
    std::vector<std::string_view> parts;
    // store drive letter if present
    if (type == PathType::FullyQualified || type == PathType::RelativeDriveCWD) {
        parts.emplace_back(path.substr(0, 2));
    }
    path = path.substr(get_path_type_offset(type));
    if (type == PathType::DeviceNormalized || type == PathType::DeviceNormalizedUNC) {
        split_path(path, parts);
    } else {
        split_path(path, parts, type);
    }
    return {.data = parts, .type = type};
}

#else // no _WIN32

void (*SPLIT_PATH_PARENT_ACTIONS[3])(Range, std::vector<std::string_view> &) = {
    action_noop,
    action_absolute,
    action_relative,
};

PathType get_path_type(std::string_view path)
{
    if (path.empty()) {
        return PathType::Empty;
    }
    if (path.starts_with('/')) {
        return PathType::Absolute;
    }
    return PathType::RelativeCWD;
}

size_t get_path_type_offset(PathType type)
{
    if (type == PathType::Absolute) {
        return 1;
    }
    return 0;
}

bool PathParts::is_relative() const
{
    return type == PathType::RelativeCWD;
}

std::string_view PathParts::prefix() const
{
    if (type == PathType::RelativeCWD) {
        return "."sv;
    }
    return std::string_view{};
}

size_t PathParts::get_join_size() const
{
    auto size = data.size();
    switch (type) {
        case PathType::RelativeCWD:
            size += 1;
            break;
        default:;
    }
    for (auto const sv : data) {
        size += sv.size();
    }
    return size;
}

std::string PathParts::join() const
{
    if (data.empty()) {
        switch (type) {
            case PathType::Absolute:
                return "/";
            case PathType::RelativeCWD:
                return ".";
            case PathType::Empty:
                return "";
        }
    }
    std::string result;
    result.reserve(get_join_size());
    result.append(prefix());
    for (auto const sv : data) {
        result.push_back('/');
        result.append(sv);
    }
    return result;
}

void split_path(std::string_view path, std::vector<std::string_view> &parts, PathType type)
{
    auto pred = [](auto r) { return !(r.empty() || std::ranges::equal(r, "."sv)); };
    auto it = path | std::views::split("/"sv) | std::views::filter(pred);
    auto parent_action = SPLIT_PATH_PARENT_ACTIONS[static_cast<int>(type)];
    for (auto r : it) {
        if (std::ranges::equal(r, ".."sv)) {
            parent_action(r, parts);
        } else {
            parts.emplace_back(r.begin(), std::ranges::distance(r));
        }
    }
}

PathParts split_path(std::string_view path)
{
    auto type = get_path_type(path);
    if (type == PathType::Empty) {
        return {};
    }
    std::vector<std::string_view> parts;
    path = path.substr(get_path_type_offset(type));
    split_path(path, parts, type);
    return {.data = parts, .type = type};
}

#endif

} // namespace Inkscape::IO
