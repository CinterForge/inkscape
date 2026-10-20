// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gio::Actions for output tied to the application and without GUI.
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#include "actions-output.h"

#include <giomm.h>  // Not <gtkmm.h>! To eventually allow a headless version!
#include <glibmm/i18n.h>
#include <glibmm/variant.h>

#include "actions-helper.h"
#include "document.h"
#include "inkscape-application.h"

// Actions for command line output (should be integrated with file dialog).

// These actions are currently stateless and result in changes to an instance of the
// InkFileExportCmd class owned by the application.

void
export_type(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    app->file_export()->export_type = s.get();
    // std::cout << "export-type: " << s.get() << std::endl;
}

void
export_filename(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_filename = s.get();
    // std::cout << "export-filename: " << s.get() << std::endl;
}

void
export_page(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(value);
    app->file_export()->export_page = s.get();
}

void
export_overwrite(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_overwrite = b.get();
    // std::cout << "export-overwrite: " << std::boolalpha << b.get() << std::endl;
}

void
export_area(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->set_export_area(s.get());
}

void
export_area_drawing(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    if (b.get()) {
        app->file_export()->set_export_area_type(ExportAreaType::Drawing);
    }
}

void
export_area_page(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    if (b.get()) {
        app->file_export()->set_export_area_type(ExportAreaType::Page);
    }
}

void
export_margin(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_margin = i.get();
    // std::cout << "export-margin: " << i.get() << std::endl;
}

void
export_area_snap(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_area_snap = b.get();
    // std::cout << "export-area-snap: " << std::boolalpha << b.get() << std::endl;
}

void
export_width(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_width = i.get();
    // std::cout << "export-width: " << i.get() << std::endl;
}

void
export_height(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_height = i.get();
    // std::cout << "export-height: " << i.get() << std::endl;
}

void
export_id(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_id = s.get();
    // std::cout << "export-id: " << s.get() << std::endl;
}

void
export_id_only(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_id_only = b.get();
    // std::cout << "export-id-only: " << std::boolalpha << b.get() << std::endl;
}

void
export_plain_svg(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_plain_svg = b.get();
    // std::cout << "export-plain-svg: " << std::boolalpha << b.get() << std::endl;
}

void
export_dpi(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    app->file_export()->export_dpi = d.get();
    // std::cout << "export-dpi: " << d.get() << std::endl;
}

void
export_ignore_filters(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_ignore_filters = b.get();
    // std::cout << "export-ignore-filters: " << std::boolalpha << b.get() << std::endl;
}

void
export_text_to_path(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_text_to_path = b.get();
    // std::cout << "export-text-to-path: " << std::boolalpha << b.get() << std::endl;
}

void
export_ps_level(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_ps_level = i.get();
    // std::cout << "export-ps-level: " << i.get() << std::endl;
}

void
export_pdf_level(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<Glib::ustring> s = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(value);
    app->file_export()->export_pdf_level = s.get();
    // std::cout << "export-pdf-level" << s.get() << std::endl;
}

void
export_latex(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_latex = b.get();
    // std::cout << "export-latex: " << std::boolalpha << b.get() << std::endl;
}

void
export_use_hints(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_use_hints = b.get();
    // std::cout << "export-use-hints: " << std::boolalpha << b.get() << std::endl;
}

void
export_background(const Glib::VariantBase& value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_background = s.get();
    // std::cout << "export-background: " << s.get() << std::endl;
}

void
export_background_opacity(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<double> d = Glib::VariantBase::cast_dynamic<Glib::Variant<double> >(value);
    app->file_export()->export_background_opacity = d.get();
    // std::cout << d.get() << std::endl;
}

void
export_png_color_mode(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<std::string> s = Glib::VariantBase::cast_dynamic<Glib::Variant<std::string> >(value);
    app->file_export()->export_png_color_mode = s.get();
    // std::cout << s.get() << std::endl;
}

void
export_png_use_dithering(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<bool> b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(value);
    app->file_export()->export_png_use_dithering = b.get();
    // std::cout << s.get() << std::endl;
}

void
export_png_compression(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_png_compression = i.get();
}

void
export_png_antialias(const Glib::VariantBase&  value, InkscapeApplication *app)
{
    Glib::Variant<int> i = Glib::VariantBase::cast_dynamic<Glib::Variant<int> >(value);
    app->file_export()->export_png_antialias = i.get();
}

void
export_do(InkscapeApplication *app)
{
    SPDocument* document = app->get_active_document();
    if (!document) {
        show_output("export_do: no documents open!");
        return;
    }
    std::string filename;
    if (document->getDocumentFilename()) {
        filename = document->getDocumentFilename();
    }
    app->file_export()->do_export(document, filename);
}

const Glib::ustring SECTION = NC_("Action Section", "Export");

std::vector<std::vector<Glib::ustring>> raw_data_output =
{
    // clang-format off
    {"app.export-type",               N_("Export Type"),               SECTION, N_("Set export file type")                               },
    {"app.export-filename",           N_("Export File Name"),          SECTION, N_("Set export file name")                               },
    {"app.export-overwrite",          N_("Export Overwrite"),          SECTION, N_("Allow to overwrite existing files during export")    },

    {"app.export-area",               N_("Export Area"),               SECTION, N_("Set export area")                                    },
    {"app.export-area-drawing",       N_("Export Area Drawing"),       SECTION, N_("Export drawing area")                                },
    {"app.export-area-page",          N_("Export Area Page"),          SECTION, N_("Export page area")                                   },
    {"app.export-margin",             N_("Export Margin"),             SECTION, N_("Set additional export margin")                       },
    {"app.export-area-snap",          N_("Export Area Snap"),          SECTION, N_("Snap export area to integer values")                 },
    {"app.export-width",              N_("Export Width"),              SECTION, N_("Set export width")                                   },
    {"app.export-height",             N_("Export Height"),             SECTION, N_("Set export height")                                  },

    {"app.export-id",                 N_("Export ID"),                 SECTION, N_("Export selected ID(s)")                              },
    {"app.export-id-only",            N_("Export ID Only"),            SECTION, N_("Hide any objects not given in export-id option")     },

    {"app.export-plain-svg",          N_("Export Plain SVG"),          SECTION, N_("Export as plain SVG")                                },
    {"app.export-dpi",                N_("Export DPI"),                SECTION, N_("Set export DPI")                                     },
    {"app.export-ignore-filters",     N_("Export Ignore Filters"),     SECTION, N_("Export without filters to avoid rasterization for PDF, PS, EPS")},
    {"app.export-text-to-path",       N_("Export Text to Path"),       SECTION, N_("Convert texts to paths in the exported file")        },
    {"app.export-ps-level",           N_("Export PS Level"),           SECTION, N_("Set PostScript level")                               },
    {"app.export-pdf-version",        N_("Export PDF Version"),        SECTION, N_("Set PDF version")                                    },
    {"app.export-page",               N_("Export Page"),               SECTION, N_("Select page(s) to export")                           },
    {"app.export-latex",              N_("Export LaTeX"),              SECTION, N_("Export LaTeX")                                       },
    {"app.export-use-hints",          N_("Export Use Hints"),          SECTION, N_("Export using saved hints")                           },
    {"app.export-background",         N_("Export Background"),         SECTION, N_("Include background color in exported file")          },
    {"app.export-background-opacity", N_("Export Background Opacity"), SECTION, N_("Include background opacity in exported file")        },
    {"app.export-png-color-mode",     N_("Export PNG Color Mode"),     SECTION, N_("Set color mode for PNG export")                      },
    {"app.export-png-use-dithering",  N_("Export PNG Dithering"),      SECTION, N_("Set dithering for PNG export")                       },
    {"app.export-png-compression",    N_("Export PNG Compression"),    SECTION, N_("Set compression level for PNG export")               },
    {"app.export-png-antialias",      N_("Export PNG Antialiasing"),   SECTION, N_("Set antialiasing level for PNG export")              },

    {"app.export-do",                 N_("Do Export"),                 SECTION, N_("Do export")                                          }
    // clang-format on
};

std::vector<std::vector<Glib::ustring>> hint_data_output =
{
    // clang-format off
    {"app.export-type",               N_("Enter string for the file type")                               },
    {"app.export-filename",           N_("Enter string for the file name")                               },
    {"app.export-overwrite",          N_("Enter 1/0 for Yes/No to overwrite exported file")              },

    {"app.export-area",               N_("Enter string for export area, formatted like x0:y0:x1:y1")     },
    {"app.export-area-drawing",       N_("Enter 1/0 for Yes/No to export drawing area")                  },
    {"app.export-area-page",          N_("Enter 1/0 for Yes/No to export page area")                     },
    {"app.export-margin",             N_("Enter integer number for margin")                              },
    {"app.export-area-snap",          N_("Enter 1/0 for Yes/No to snap the export area")                 },
    {"app.export-width",              N_("Enter integer number for width")                               },
    {"app.export-height",             N_("Enter integer number for height")                              },

    {"app.export-id",                 N_("Enter string for export ID")                                   },
    {"app.export-id-only",            N_("Enter 1/0 for Yes/No to export only given ID")                 },

    {"app.export-plain-svg",          N_("Enter 1/0 for Yes/No to export plain SVG")                     },
    {"app.export-dpi",                N_("Enter integer number for export DPI")                          },
    {"app.export-ignore-filters",     N_("Enter 1/0 for Yes/No to export ignoring filters")              },
    {"app.export-text-to-path",       N_("Enter 1/0 for Yes/No to convert text to path on export")       },
    {"app.export-ps-level",           N_("Enter integer number 2 or 3 for PS Level")                     },
    {"app.export-pdf-version",        N_("Enter string for PDF Version, e.g. 2.0")                       },
    {"app.export-page",               N_("Enter string for page numbers e.g. '1', '1-3' or '2,4'")       },
    {"app.export-latex",              N_("Enter 1/0 for Yes/No to export to PDF and LaTeX")              },
    {"app.export-use-hints",          N_("Enter 1/0 for Yes/No to use export hints from document")       },
    {"app.export-background",         N_("Enter string for background color, e.g. #ff007f or rgb(255, 0, 128)")                 },
    {"app.export-background-opacity", N_("Enter number for background opacity, either between 0.0 and 1.0, or 1 up to 255")     },
    {"app.export-png-color-mode",     N_("Enter string for PNG Color Mode, one of Gray_1/Gray_2/Gray_4/Gray_8/Gray_16/RGB_8/RGB_16/GrayAlpha_8/GrayAlpha_16/RGBA_8/RGBA_16")},
    {"app.export-png-use-dithering",  N_("Enter 1/0 for Yes/No to use dithering")          },
    {"app.export-png-compression",    N_("Enter integer for PNG compression level (0 (none) to 9 (max))")},
    {"app.export-png-antialias",      N_("Enter integer for PNG antialiasing level (0 (none) to 3 (best))")}
    // clang-format on
};


void
add_actions_output(InkscapeApplication* app)
{
    Glib::VariantType Bool(  Glib::VARIANT_TYPE_BOOL);
    Glib::VariantType Int(   Glib::VARIANT_TYPE_INT32);
    Glib::VariantType Double(Glib::VARIANT_TYPE_DOUBLE);
    Glib::VariantType String(Glib::VARIANT_TYPE_STRING);
    Glib::VariantType BString(Glib::VARIANT_TYPE_BYTESTRING);

    // Debian 9 has 2.50.0
#if GLIB_CHECK_VERSION(2, 52, 0)
    auto *gapp = app->gio_app();

    // Matches command line options
    // clang-format off
    gapp->add_action_with_parameter( "export-type",              String, sigc::bind(sigc::ptr_fun(&export_type),         app));
    gapp->add_action_with_parameter( "export-filename",          String, sigc::bind(sigc::ptr_fun(&export_filename),     app)); // MAY NOT WORK DUE TO std::string
    gapp->add_action_with_parameter( "export-page",              String, sigc::bind(sigc::ptr_fun(&export_page), app));
    gapp->add_action_with_parameter( "export-overwrite",         Bool,   sigc::bind(sigc::ptr_fun(&export_overwrite),    app));

    gapp->add_action_with_parameter( "export-area",              String, sigc::bind(sigc::ptr_fun(&export_area),         app));
    gapp->add_action_with_parameter( "export-area-drawing",      Bool,   sigc::bind(sigc::ptr_fun(&export_area_drawing), app));
    gapp->add_action_with_parameter( "export-area-page",         Bool,   sigc::bind(sigc::ptr_fun(&export_area_page),    app));
    gapp->add_action_with_parameter( "export-margin",            Int,    sigc::bind(sigc::ptr_fun(&export_margin),       app));
    gapp->add_action_with_parameter( "export-area-snap",         Bool,   sigc::bind(sigc::ptr_fun(&export_area_snap),    app));
    gapp->add_action_with_parameter( "export-width",             Int,    sigc::bind(sigc::ptr_fun(&export_width),        app));
    gapp->add_action_with_parameter( "export-height",            Int,    sigc::bind(sigc::ptr_fun(&export_height),       app));

    gapp->add_action_with_parameter( "export-id",                String, sigc::bind(sigc::ptr_fun(&export_id),           app));
    gapp->add_action_with_parameter( "export-id-only",           Bool,   sigc::bind(sigc::ptr_fun(&export_id_only),      app));

    gapp->add_action_with_parameter( "export-plain-svg",         Bool,   sigc::bind(sigc::ptr_fun(&export_plain_svg),    app));
    gapp->add_action_with_parameter( "export-dpi",               Double, sigc::bind(sigc::ptr_fun(&export_dpi),          app));
    gapp->add_action_with_parameter( "export-ignore-filters",    Bool,   sigc::bind(sigc::ptr_fun(&export_plain_svg),    app));
    gapp->add_action_with_parameter( "export-text-to-path",      Bool,   sigc::bind(sigc::ptr_fun(&export_text_to_path), app));
    gapp->add_action_with_parameter( "export-ps-level",          Int,    sigc::bind(sigc::ptr_fun(&export_ps_level),     app));
    gapp->add_action_with_parameter( "export-pdf-version",       String, sigc::bind(sigc::ptr_fun(&export_pdf_level),    app));
    gapp->add_action_with_parameter( "export-latex",             Bool,   sigc::bind(sigc::ptr_fun(&export_latex),        app));
    gapp->add_action_with_parameter( "export-use-hints",         Bool,   sigc::bind(sigc::ptr_fun(&export_use_hints),    app));
    gapp->add_action_with_parameter( "export-background",        String, sigc::bind(sigc::ptr_fun(&export_background),   app));
    gapp->add_action_with_parameter( "export-background-opacity",Double, sigc::bind(sigc::ptr_fun(&export_background_opacity), app));
    gapp->add_action_with_parameter( "export-png-color-mode",    String, sigc::bind(sigc::ptr_fun(&export_png_color_mode), app));
    gapp->add_action_with_parameter( "export-png-use-dithering", Bool,   sigc::bind(sigc::ptr_fun(&export_png_use_dithering), app));
    gapp->add_action_with_parameter( "export-png-compression",   Int,    sigc::bind(sigc::ptr_fun(&export_png_compression),   app));
    gapp->add_action_with_parameter( "export-png-antialias",     Int,    sigc::bind(sigc::ptr_fun(&export_png_antialias),     app));

    // Extra
    gapp->add_action(                "export-do",                        sigc::bind(sigc::ptr_fun(&export_do),           app));
    // clang-format on
#else
    show_output("add_actions: Some actions require Glibmm 2.52, compiled with: " << glib_major_version << "." << glib_minor_version);
#endif

    app->get_action_extra_data().add_data(raw_data_output);
    app->get_action_hint_data().add_data(hint_data_output);
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
