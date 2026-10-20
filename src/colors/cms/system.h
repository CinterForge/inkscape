// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Access operating system wide information about color management.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_CMS_SYSTEM_H
#define SEEN_COLORS_CMS_SYSTEM_H

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "preferences.h"

using DirPaths = std::vector<std::pair<std::string, bool>>;

namespace Inkscape::Colors::CMS {

class Profile;
class TransformSurface;
class System
{
private:
    System(System const &) = delete;
    void operator=(System const &) = delete;

public:
    // Access the singleton CMS::System object.
    static System &get();

    DirPaths const &getDirectoryPaths();
    void addDirectoryPath(std::string path, bool is_user);
    void clearDirectoryPaths();

    std::vector<std::shared_ptr<Profile>> getProfiles() const;
    const std::shared_ptr<Profile> &getProfile(std::string const &name) const;

    std::vector<std::shared_ptr<Profile>> getDisplayProfiles() const;
    const std::shared_ptr<Profile> &getDisplayProfile(bool &updated);
    const std::shared_ptr<TransformSurface> &getDisplayTransform();

    std::vector<std::shared_ptr<Profile>> getOutputProfiles() const;

    void refreshProfiles();

    System();
    ~System() = default;

    // Used by testing to add profiles when needed
    void addProfile(std::shared_ptr<Profile> profile) { _profiles.emplace_back(profile); }

private:
    // List of ICC profiles found on system
    std::vector<std::shared_ptr<Profile>> _profiles;

    // Paths to search for icc profiles
    DirPaths _paths;

    // We track last display transform settings. If there is a change, we delete create new transform.
    std::shared_ptr<Profile> _display_profile;
    std::shared_ptr<TransformSurface> _display_transform;
    bool _display = false;
    int _display_intent = -1;

    Inkscape::PrefObserver _prefs_observer;
};

} // namespace Inkscape::Colors::CMS

#endif // SEEN_COLORS_CMS_SYSTEM_H

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
