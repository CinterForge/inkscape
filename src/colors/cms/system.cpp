// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A class to provide access to system/user ICC color profiles.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "system.h"

#include <glibmm.h> // home-dir and filename building
#include <iomanip>

#include "io/resource.h"
#include "profile.h"
#include "transform-surface.h"

// clang-format off
#ifdef _WIN32
#undef NOGDI
#include <windows.h>
#include <icm.h>
#endif
// clang-format on

namespace Inkscape::Colors::CMS {

System &System::get()
{
    static System instance;
    return instance;
}

System::System()
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    _prefs_observer = prefs->createObserver("/options/displayprofile", [this]() {
        _display_profile.reset();
        _display_transform.reset();
    });
}

/**
 * Search for system ICC profile files and add them to list.
 */
void System::refreshProfiles()
{
    _profiles.clear(); // Allows us to refresh list if necessary.

    // Get list of all possible file directories, with flag if they are "home" directories or not.
    // Look for icc files in specified directories.
    for (auto const &directory_path : getDirectoryPaths()) {
        using Inkscape::IO::Resource::get_filenames;
        for (auto &&filename : get_filenames(directory_path.first, {".icc", ".icm"})) {
            // Check if files are ICC files and extract out basic information, add to list.
            if (!Profile::isIccFile(filename)) {
                g_warning("System::load_profiles: '%s' is not an ICC file!", filename.c_str());
                continue;
            }

            auto profile = Profile::create_from_uri(std::move(filename), directory_path.second);

            for (auto const &other : _profiles) {
                if (other->getName() == profile->getName() && other->getId() != profile->getId()) {
                    std::cerr << "System::load_profiles: Different ICC profile with duplicate name: "
                              << profile->getName() << ":" << std::endl;
                    std::cerr << "   " << profile->getPath() << " (" << profile->getId() << ")" << std::endl;
                    std::cerr << "   " << other->getPath() << " (" << other->getId() << ")" << std::endl;
                    continue;
                }
            }
            _profiles.emplace_back(std::move(profile));
        }
    }
}

static DirPaths get_directory_paths()
{
    DirPaths paths;

    // First try user's local directory.
    paths.emplace_back(Glib::build_filename(Glib::get_user_data_dir(), "color", "icc"), true);

    // See
    // https://github.com/hughsie/colord/blob/fe10f76536bb27614ced04e0ff944dc6fb4625c0/lib/colord/cd-icc-store.c#L590

    // User store
    paths.emplace_back(Glib::build_filename(Glib::get_user_data_dir(), "icc"), true);
    paths.emplace_back(Glib::build_filename(Glib::get_home_dir(), ".color", "icc"), true);

    // System store
    paths.emplace_back("/var/lib/color/icc", false);
    paths.emplace_back("/var/lib/colord/icc", false);

    auto data_directories = Glib::get_system_data_dirs();
    for (auto const &data_directory : data_directories) {
        paths.emplace_back(Glib::build_filename(data_directory, "color", "icc"), false);
    }

#ifdef __APPLE__
    paths.emplace_back("/System/Library/ColorSync/Profiles", false);
    paths.emplace_back("/Library/ColorSync/Profiles", false);

    paths.emplace_back(Glib::build_filename(Glib::get_home_dir(), "Library", "ColorSync", "Profiles"), true);
#endif // __APPLE__

#ifdef _WIN32
    wchar_t pathBuf[MAX_PATH + 1];
    pathBuf[0] = 0;
    DWORD pathSize = sizeof(pathBuf);
    g_assert(sizeof(wchar_t) == sizeof(gunichar2));
    if (GetColorDirectoryW(NULL, pathBuf, &pathSize)) {
        auto utf8Path = g_utf16_to_utf8((gunichar2 *)(&pathBuf[0]), -1, NULL, NULL, NULL);
        if (!g_utf8_validate(utf8Path, -1, NULL)) {
            g_warning("GetColorDirectoryW() resulted in invalid UTF-8");
        } else {
            paths.emplace_back(utf8Path, false);
        }
        g_free(utf8Path);
    }
#endif // _WIN32

    return paths;
}

/**
 * Create list of all directories where ICC profiles are expected to be found.
 */
DirPaths const &System::getDirectoryPaths()
{
    if (_paths.empty()) {
        _paths = get_directory_paths();
    }
    return _paths;
}

/**
 * Remove all directory paths that might have been generated (useful for refreshing)
 */
void System::clearDirectoryPaths()
{
    _paths.clear();
}

/**
 * Replace all generated profile paths with this single path, useful for testing.
 */
void System::addDirectoryPath(std::string path, bool is_user)
{
    _paths.emplace_back(std::move(path), is_user);
}

/**
 * Returns a list of profiles sorted by their internal names.
 */
std::vector<std::shared_ptr<Profile>> System::getProfiles() const
{
    auto result = _profiles; // copy
    std::sort(result.begin(), result.end(), Profile::sortByName);
    return result;
}

/**
 * Get the user set display profile, if set.
 */
const std::shared_ptr<Profile> &System::getDisplayProfile(bool &updated)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    std::string uri = prefs->getString("/options/displayprofile/uri");

    if (!uri.empty() && (!_display_profile || _display_profile->getPath() != uri)) {
        auto profile = Profile::create_from_uri(uri, false);
        if (!profile->isForDisplay()) {
            g_warning("System::get_display_profile: Not a display (display) profile: %s", uri.c_str());
        } else {
            updated = true;
            _display_profile = profile;
        }
    }
    return _display_profile;
}

/**
 * Returns a list of profiles that can apply to the display (display), sorted by their internal names.
 */
std::vector<std::shared_ptr<Profile>> System::getDisplayProfiles() const
{
    std::vector<std::shared_ptr<Profile>> result;
    result.reserve(_profiles.size());

    for (auto const &profile : _profiles) {
        if (profile->isForDisplay()) {
            result.push_back(profile);
        }
    }
    std::sort(result.begin(), result.end(), Profile::sortByName);
    return result;
}

/**
 * Return vector of profiles which can be used for cms output
 */
std::vector<std::shared_ptr<Profile>> System::getOutputProfiles() const
{
    std::vector<std::shared_ptr<Profile>> result;
    result.reserve(_profiles.size());

    for (auto const &profile : _profiles) {
        if (profile->getProfileClass() == cmsSigOutputClass) {
            result.push_back(profile);
        }
    }
    std::sort(result.begin(), result.end(), Profile::sortByName);
    return result;
}

/**
 * Return the profile object which is matched by the given name, id or path
 *
 * @arg name - A string that can contain either the profile name as stored in the icc file
 *             the ID which is a hex value different for each version of the profile also
 *             stored in the icc file. Or the path where the profile was found.
 * @returns A pointer to the profile object, or nullptr
 */
const std::shared_ptr<Profile> &System::getProfile(std::string const &name) const
{
    for (auto const &profile : _profiles) {
        if (name == profile->getName() || name == profile->getId() || name == profile->getPath()) {
            return profile;
        }
    }
    static std::shared_ptr<Profile> not_found;
    return not_found;
}

/**
 * Get the color managed transform for the screen.
 *
 * There is one transform for all displays, anything more complex and the user should
 * use their operating system CMS configurations instead of the Inkscape display cms.
 *
 * Transform immutably shared between System and Canvas.
 */
const std::shared_ptr<TransformSurface> &System::getDisplayTransform()
{
    bool need_to_update = false;

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool display = prefs->getIntLimited("/options/displayprofile/enabled", false);
    int display_intent = prefs->getIntLimited("/options/displayprofile/intent", 0, 0, 3);

    if (_display != display || _display_intent != display_intent) {
        need_to_update = true;
        _display = display;
        _display_intent = display_intent;
    }

    auto display_profile = display ? getDisplayProfile(need_to_update) : nullptr;

    if (need_to_update) {
        if (display_profile) {
            // No High bit depths here! Just low resolution gamma curved sRGB.
            // TODO: Replace with something with a better gammut and depth.
            TransformSurface::Format in = {
                .profile = Profile::create_srgb(),
                .byte_count = sizeof(char), // Char 8 per channel
                .integral = true,
            };
            TransformSurface::Format out = {
                .profile = display_profile,
                .byte_count = sizeof(char),
                .integral = true,
            };
            _display_transform = std::make_shared<TransformSurface>(in, out);
        } else {
            _display_transform = nullptr;
        }
    }
    return _display_transform;
}

} // namespace Inkscape::Colors::CMS

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
