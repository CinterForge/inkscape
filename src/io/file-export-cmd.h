// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File export from the command line. This code, greatly modified, use to be in main.cpp. It should
 * be replaced by code shared with the file dialog (Gio::Actions?).
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * Git blame shows that bulia byak is the main author of the original export code from
 * main.cpp. Other authors of note include Nicolas Dufour, Vinicius dos Santos Oliveira, and Bob
 * Jamison; none of whom bothered to add their names to the copyright of main.cc.
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 */

#ifndef INK_FILE_EXPORT_CMD_H
#define INK_FILE_EXPORT_CMD_H

#include <string>
#include <2geom/rect.h>
#include <glibmm/ustring.h>

class SPDocument;
class SPItem;
class SPObject;
namespace Inkscape {
namespace Extension {
class Output;
} // namespace Extension
namespace Colors {
class Color;
}
} // namespace Inkscape

enum class ExportAreaType
{
    Unset,
    Drawing,
    Page,
    Area,
};

class InkFileExportCmd {
public:
    InkFileExportCmd();

    void do_export(SPDocument* doc, std::string filename_in="");

private:
    ExportAreaType export_area_type{ExportAreaType::Unset};
    Glib::ustring export_area{};
    void fit_to_export_area(SPDocument *doc, SPObject *object);
    Inkscape::Colors::Color get_bgcolor(SPDocument *doc);
    std::string get_filename_out(std::string filename_in = "", std::string object_id = "");
    int do_export_svg(SPDocument *doc, std::string const &filename_in);
    int do_export_vector(SPDocument *doc, std::string const &filename_in, Inkscape::Extension::Output &extension);
    int do_export_png(SPDocument *doc, std::string const &filename_in);
    int do_export_ps_pdf(SPDocument *doc, std::string const &filename_in, std::string const &mime_type);
    int do_export_ps_pdf(SPDocument *doc, std::string const &filename_in, std::string const &mime_type,
                         Inkscape::Extension::Output &extension);
    int do_export_extension(SPDocument *doc, std::string const &filename_in, Inkscape::Extension::Output *extension);
    Glib::ustring export_type_current;
    void do_export_png_now(SPDocument *doc, std::string const &filename_out, Geom::Rect area, double dpi_in, const std::vector<SPItem const *> &items);

public:
    // Should be private, but this is just temporary code (I hope!).

    // One-to-one correspondence with command line options
    std::string   export_filename; // Only if one file is processed!

    Glib::ustring export_type;
    Glib::ustring export_extension;
    bool          export_overwrite;

    int           export_margin;
    bool          export_area_snap;
    int           export_width;
    int           export_height;

    Glib::ustring export_page;

    double        export_dpi;
    bool          export_ignore_filters;
    bool          export_text_to_path;
    int           export_ps_level;
    Glib::ustring export_pdf_level;
    bool          export_latex;
    Glib::ustring export_id;
    bool          export_id_only;
    bool          export_use_hints;
    Glib::ustring export_background;
    double        export_background_opacity;
    Glib::ustring export_png_color_mode;
    bool          export_plain_svg;
    bool          export_png_use_dithering;
    int           export_png_compression;
    int           export_png_antialias;
    bool          make_paths = false;

    void set_export_area(const Glib::ustring &area);
    void set_export_area_type(ExportAreaType type);
};

#endif // INK_FILE_EXPORT_CMD_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
