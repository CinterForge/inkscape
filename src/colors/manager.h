// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Colors::Manager - Look after all a document's icc profiles.
 *
 * Copyright 2023 Martin Owens <doctormo@geek-2.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_MANAGER_H
#define SEEN_COLORS_MANAGER_H

#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "spaces/components.h"

namespace Inkscape::Colors {
namespace Space {
enum class Type;
class AnySpace;
} // namespace Space
class Color;
class Parser;

class Manager
{
private:
    Manager(Manager const &) = delete;
    void operator=(Manager const &) = delete;

public:

    static Manager &get()
    {
        static Manager instance;
        return instance;
    }

    // Request all color spaces with given trait(s); Ex: spaces(Traits::Picker) to list of types suitable for GUI
    std::vector<std::shared_ptr<Space::AnySpace>> spaces(Space::Traits traits);

    std::shared_ptr<Space::AnySpace> find(Space::Type type) const;
    std::shared_ptr<Space::AnySpace> find(std::string const &name) const;
    std::shared_ptr<Space::AnySpace> findSvgColorSpace(std::string const &input) const;

protected:
    Manager();
    ~Manager() = default;

    std::shared_ptr<Space::AnySpace> addSpace(Space::AnySpace *space);
    bool removeSpace(std::shared_ptr<Space::AnySpace> space);

private:
    std::vector<std::shared_ptr<Space::AnySpace>> _spaces;

    struct cmpCaseInsensitive {
        bool operator()(const std::string& a, const std::string& b) const {
            return strcasecmp(a.c_str(), b.c_str()) < 0;
        }
    };
    std::map<std::string, std::shared_ptr<Space::AnySpace>, cmpCaseInsensitive> _svg_names_lookup;
};

} // namespace Inkscape::Colors

#endif // SEEN_COLORS_MANAGER_H

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
