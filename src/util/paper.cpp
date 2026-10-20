// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape PaperSize
 *
 * Authors:
 *      Bob Jamison (2006)
 *      Martin Owens (2021)
 *
 * Copyright (C) 2021 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glibmm/i18n.h>

#include "paper.h"
#include "pages-skeleton.h"

#include "io/resource.h"

namespace Inkscape {

/**
 * Returns a list of page sizes.
 */ 
const std::vector<PaperSize>& PaperSize::getPageSizes()
{   
    // Static makes us only load pages once.
    static std::vector<PaperSize> ret;
    if (!ret.empty())
        return ret;
    auto path = Inkscape::IO::Resource::profile_path("pages.csv");
    if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) {
        if (!g_file_set_contents(path.c_str(), pages_skeleton, -1, nullptr)) {
            g_warning("%s", _("Failed to create the page file."));
        }
    }   
    gchar *content = nullptr;
    if (g_file_get_contents(path.c_str(), &content, nullptr, nullptr)) {
        gchar **lines = g_strsplit_set(content, "\n", 0); 
    
        for (int i = 0; lines && lines[i]; ++i) {
            gchar **line = g_strsplit_set(lines[i], ",", 5);
    
            if (line[0] && line[1] && line[2] && line[3] && line[0][0] != '#') {
                //name, width, height, unit
                double width = g_ascii_strtod(line[1], nullptr);
                double height = g_ascii_strtod(line[2], nullptr);
                g_strstrip(line[0]);
                g_strstrip(line[3]);
                Glib::ustring name = line[0];
                ret.emplace_back(name, width, height, Inkscape::Util::UnitTable::get().getUnit(line[3]));
            }

            g_strfreev(line);
        }
        g_strfreev(lines); 
        g_free(content);
    }   
    return ret;
}   


PaperSize::PaperSize()
    : name("")
    , width(0.0)
    , height(0.0)
{
    unit = Inkscape::Util::UnitTable::get().getUnit("px");
}


PaperSize::PaperSize(std::string name, double width, double height, Inkscape::Util::Unit const *unit)
    : name(std::move(name))
    , width(width)
    , height(height)
    , unit(unit) 
{}

std::string PaperSize::getDescription(bool landscape) const {
    return toDescription(name, size[landscape], size[!landscape], unit);
}

std::string PaperSize::toDimsString(double x, double y, Util::Unit const *unit)
{
    return formatNumber(x) + " × " + formatNumber(y) + " " + unit->abbr;
}

std::string PaperSize::toDescription(std::string name, double x, double y, Inkscape::Util::Unit const *unit)
{
    if (!name.empty()) {
        name = _(name.c_str());
    }
    return name + " (" + toDimsString(x, y, unit) + ")";
}

std::string PaperSize::formatNumber(double val)
{
    char buf[20];
    snprintf(buf, 19, "%0.1f", val);
    auto ret = std::string(buf);
    // C++ doesn't provide a good number formatting control, so hack off trailing zeros.
    if ((ret.length() > 2) && (ret.back() == '0')) {
        ret = ret.substr(0, ret.length() - 2);
    }
    return ret;
}

void PaperSize::assign(const PaperSize &other)
{
    name = other.name;
    width = other.width;
    height = other.height;
    auto [smaller, larger] = std::minmax(width, height);
    size = Geom::Point(smaller, larger);
    unit = other.unit;
}

/**
 * Returns a matching paper size, if possible.
 */
const PaperSize *PaperSize::findPaperSize(double width, double height, Inkscape::Util::Unit const *unit)
{
    auto [smaller, larger] = std::minmax(width, height);
    auto size = Geom::Point(smaller, larger);
    auto px = Inkscape::Util::UnitTable::get().getUnit("px");

    for (auto&& page_size : Inkscape::PaperSize::getPageSizes()) {
        auto cmp = Geom::Point(
            unit->convert(size[0], page_size.unit),
            unit->convert(size[1], page_size.unit)
        );
        // We want a half a pixel tollerance to catch floating point errors
        auto tollerance = px->convert(0.5, page_size.unit);
        if (Geom::are_near(page_size.size, cmp, tollerance)) {
            return &page_size;
        }
    }
    return nullptr;
}

} // namespace Inkscape

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
