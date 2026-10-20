// SPDX-License-Identifier: GPL-2.0-or-later
#include <cstring>
#include <optional>
#include "slot-resolver.h"
#include "display/nr-filter-types.h"

static auto read_special_name(std::string const &name) -> std::optional<int>
{
    static auto const dict = std::unordered_map<std::string, int>{
        { "SourceGraphic",   Inkscape::Filters::NR_FILTER_SOURCEGRAPHIC },
        { "SourceAlpha",     Inkscape::Filters::NR_FILTER_SOURCEALPHA },
        { "StrokePaint",     Inkscape::Filters::NR_FILTER_STROKEPAINT },
        { "FillPaint",       Inkscape::Filters::NR_FILTER_FILLPAINT },
        { "BackgroundImage", Inkscape::Filters::NR_FILTER_BACKGROUNDIMAGE },
        { "BackgroundAlpha", Inkscape::Filters::NR_FILTER_BACKGROUNDALPHA }
    };

    if (auto it = dict.find(name); it != dict.end()) {
        return it->second;
    }

    return {};
}

int SlotResolver::read(std::optional<std::string> const &name) const
{
    return name ? read(*name) : Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
}

int SlotResolver::read(std::string const &name) const
{
    if (auto ret = read_special_name(name)) {
        return *ret;
    }

    if (auto it = map.find(name); it != map.end()) {
        return it->second;
    }

    return Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
}

int SlotResolver::write(std::optional<std::string> const &name)
{
    return name ? write(*name) : Inkscape::Filters::NR_FILTER_SLOT_NOT_SET;
}

int SlotResolver::write(std::string const &name)
{
    auto [it, ret] = map.try_emplace(name);

    if (ret) {
        it->second = next;
        next++;
    }

    return it->second;
}
