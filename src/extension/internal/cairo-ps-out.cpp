// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A quick hack to use the Cairo renderer to write out a file.  This
 * then makes 'save as...' PS.
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *   Ulf Erikson <ulferikson@users.sf.net>
 *   Adib Taraben <theAdib@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004-2006 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cairo.h>
#ifdef CAIRO_HAS_PS_SURFACE

#include "cairo-ps.h"
#include "cairo-ps-out.h"
#include "cairo-render-context.h"
#include "cairo-renderer.h"
#include "latex-text-renderer.h"
#include "path-chemistry.h"
#include <print.h>
#include "extension/system.h"
#include "extension/print.h"
#include "extension/db.h"
#include "extension/output.h"
#include "display/drawing.h"

#include "path/path-curve.h"

#include "object/sp-item.h"
#include "object/sp-root.h"

#include "io/sys.h"
#include "document.h"

namespace Inkscape {
namespace Extension {
namespace Internal {

bool CairoPsOutput::check (Inkscape::Extension::Extension * /*module*/)
{
    if (nullptr == Inkscape::Extension::db.get(SP_MODULE_KEY_PRINT_CAIRO_PS)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

bool CairoEpsOutput::check (Inkscape::Extension::Extension * /*module*/)
{
    if (nullptr == Inkscape::Extension::db.get(SP_MODULE_KEY_PRINT_CAIRO_EPS)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static bool
ps_print_document_to_file(SPDocument *doc, gchar const *filename, unsigned int level, bool texttopath, bool omittext,
                          bool filtertobitmap, int resolution, bool eps = false)
{
    if (texttopath) {
        assert(!omittext);
        // Cairo's text-to-path method has numerical precision and font matching
        // issues (https://gitlab.com/inkscape/inkscape/-/issues/1979).
        // We get better results by using Inkscape's Object-to-Path method.
        Inkscape::convert_text_to_curves(doc);
    }

    doc->ensureUpToDate();

    SPRoot *root = doc->getRoot();
    if (!root) {
        return false;
    }

    Inkscape::Drawing drawing;
    unsigned dkey = SPItem::display_key_new(1);
    root->invoke_show(drawing, dkey, SP_ITEM_SHOW_DISPLAY);

    /* Create renderer and context */
    CairoRenderer renderer;
    CairoRenderContext ctx = renderer.createContext();
    ctx.setPSLevel(level);
    ctx.setEPS(eps);
    ctx.setTextToPath(texttopath);
    ctx.setOmitText(omittext);
    ctx.setFilterToBitmap(filtertobitmap);
    ctx.setBitmapResolution(resolution);

    bool ret = ctx.setPsTarget(filename);
    if (ret) {
        /* Render document */
        ret = renderer.setupDocument(&ctx, doc, root);
        if (ret) {
            /* Render multiple pages */
            ret = renderer.renderPages(&ctx, doc, false);
            ctx.finish();
        }
    }

    root->invoke_hide(dkey);
    return ret;
}


/**
    \brief  This function calls the output module with the filename
	\param  mod   unused
	\param  doc   Document to be saved
    \param  filename   Filename to save to (probably will end in .ps)
*/
void
CairoPsOutput::save(Inkscape::Extension::Output *mod, SPDocument *doc, gchar const *filename)
{
    Inkscape::Extension::Extension * ext;
    unsigned int ret;

    ext = Inkscape::Extension::db.get(SP_MODULE_KEY_PRINT_CAIRO_PS);
    if (ext == nullptr)
        return;

    int level = CAIRO_PS_LEVEL_2;
    try {
        const gchar *new_level = mod->get_param_optiongroup("PSlevel");
        if((new_level != nullptr) && (g_ascii_strcasecmp("PS3", new_level) == 0)) {
            level = CAIRO_PS_LEVEL_3;
        }
    } catch(...) {}

    bool new_textToPath  = FALSE;
    try {
        new_textToPath = (strcmp(mod->get_param_optiongroup("textToPath"), "paths") == 0);
    } catch(...) {}

    bool new_textToLaTeX  = FALSE;
    try {
        new_textToLaTeX = (strcmp(mod->get_param_optiongroup("textToPath"), "LaTeX") == 0);
    }
    catch(...) {
        g_warning("Parameter <textToLaTeX> might not exist");
    }

    bool new_blurToBitmap  = FALSE;
    try {
        new_blurToBitmap  = mod->get_param_bool("blurToBitmap");
    } catch(...) {}

    int new_bitmapResolution  = 72;
    try {
        new_bitmapResolution = mod->get_param_int("resolution");
    } catch(...) {}

    // Create PS
    {
        gchar * final_name;
        final_name = g_strdup_printf("> %s", filename);
        ret = ps_print_document_to_file(doc, final_name, level, new_textToPath,
                                        new_textToLaTeX, new_blurToBitmap,
                                        new_bitmapResolution);
        g_free(final_name);

        if (!ret)
            throw Inkscape::Extension::Output::save_failed();
    }

    // Create LaTeX file (if requested)
    if (new_textToLaTeX) {
        ret = latex_render_document_text_to_file(doc, filename, false);

        if (!ret)
            throw Inkscape::Extension::Output::save_failed();
    }
}


/**
    \brief  This function calls the output module with the filename
	\param  mod   unused
	\param  doc   Document to be saved
    \param  filename   Filename to save to (probably will end in .ps)
*/
void
CairoEpsOutput::save(Inkscape::Extension::Output *mod, SPDocument *doc, gchar const *filename)
{
    Inkscape::Extension::Extension * ext;
    unsigned int ret;

    ext = Inkscape::Extension::db.get(SP_MODULE_KEY_PRINT_CAIRO_EPS);
    if (ext == nullptr)
        return;

    int level = CAIRO_PS_LEVEL_2;
    try {
        const gchar *new_level = mod->get_param_optiongroup("PSlevel");
        if((new_level != nullptr) && (g_ascii_strcasecmp("PS3", new_level) == 0)) {
            level = CAIRO_PS_LEVEL_3;
        }
    } catch(...) {}

    bool new_textToPath  = FALSE;
    try {
        new_textToPath = (strcmp(mod->get_param_optiongroup("textToPath"), "paths") == 0);
    } catch(...) {}

    bool new_textToLaTeX  = FALSE;
    try {
        new_textToLaTeX = (strcmp(mod->get_param_optiongroup("textToPath"), "LaTeX") == 0);
    }
    catch(...) {
        g_warning("Parameter <textToLaTeX> might not exist");
    }

    bool new_blurToBitmap  = FALSE;
    try {
        new_blurToBitmap  = mod->get_param_bool("blurToBitmap");
    } catch(...) {}

    int new_bitmapResolution  = 72;
    try {
        new_bitmapResolution = mod->get_param_int("resolution");
    } catch(...) {}

    // Create EPS
    {
        gchar * final_name;
        final_name = g_strdup_printf("> %s", filename);
        ret = ps_print_document_to_file(doc, final_name, level, new_textToPath,
                                        new_textToLaTeX, new_blurToBitmap,
                                        new_bitmapResolution, true);
        g_free(final_name);

        if (!ret)
            throw Inkscape::Extension::Output::save_failed();
    }

    // Create LaTeX file (if requested)
    if (new_textToLaTeX) {
        ret = latex_render_document_text_to_file(doc, filename, false);

        if (!ret)
            throw Inkscape::Extension::Output::save_failed();
    }
}


bool
CairoPsOutput::textToPath(Inkscape::Extension::Print * ext)
{
    return ext->get_param_bool("textToPath");
}

bool
CairoEpsOutput::textToPath(Inkscape::Extension::Print * ext)
{
    return ext->get_param_bool("textToPath");
}

#include "clear-n_.h"

/**
	\brief   A function allocate a copy of this function.

	This is the definition of Cairo PS out.  This function just
	calls the extension system with the memory allocated XML that
	describes the data.
*/
void
CairoPsOutput::init ()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("PostScript") "</name>\n"
            "<id>" SP_MODULE_KEY_PRINT_CAIRO_PS "</id>\n"
            "<param name=\"PSlevel\" gui-text=\"" N_("Restrict to PS level:") "\" type=\"optiongroup\" appearance=\"combo\" >\n"
                "<option value='PS3'>" N_("PostScript level 3") "</option>\n"
                "<option value='PS2'>" N_("PostScript level 2") "</option>\n"
            "</param>\n"
            "<param name=\"textToPath\" gui-text=\"" N_("Text output options:") "\" type=\"optiongroup\" appearance=\"radio\">\n"
                "<option value=\"embed\">" N_("Embed fonts") "</option>\n"
                "<option value=\"paths\">" N_("Convert text to paths") "</option>\n"
                "<option value=\"LaTeX\">" N_("Omit text in PDF and create LaTeX file") "</option>\n"
            "</param>\n"
            "<param name=\"blurToBitmap\" gui-text=\"" N_("Rasterize filter effects") "\" type=\"bool\">true</param>\n"
            "<param name=\"resolution\" gui-text=\"" N_("Resolution for rasterization (dpi):") "\" type=\"int\" min=\"1\" max=\"10000\">96</param>\n"
            "<spacer/>"
            "<hbox indent=\"1\"><image>info-outline</image><spacer/><vbox><spacer/>"
                "<label>" N_("When exporting from the Export dialog, you can choose objects to export. 'Save a copy' / 'Save as' will export all pages.") "</label>"
                "<spacer size=\"5\" />"
                "<label>" N_("The page bleed can be set with the Page tool.") "</label>"
            "</vbox></hbox>"
            "<output>\n"
                "<extension>.ps</extension>\n"
                "<mimetype>image/x-postscript</mimetype>\n"
                "<filetypename>" N_("PostScript (*.ps)") "</filetypename>\n"
                "<filetypetooltip>" N_("PostScript File") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>", std::make_unique<CairoPsOutput>());
    // clang-format on

    return;
}

/**
	\brief   A function allocate a copy of this function.

	This is the definition of Cairo EPS out.  This function just
	calls the extension system with the memory allocated XML that
	describes the data.
*/
void
CairoEpsOutput::init ()
{
    // clang-format off
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Encapsulated PostScript") "</name>\n"
            "<id>" SP_MODULE_KEY_PRINT_CAIRO_EPS "</id>\n"
            "<param name=\"PSlevel\" gui-text=\"" N_("Restrict to PS level:") "\" type=\"optiongroup\" appearance=\"combo\" >\n"
                "<option value='PS3'>" N_("PostScript level 3") "</option>\n"
                "<option value='PS2'>" N_("PostScript level 2") "</option>\n"
            "</param>\n"
            "<param name=\"textToPath\" gui-text=\"" N_("Text output options:") "\" type=\"optiongroup\" appearance=\"radio\">\n"
                "<option value=\"embed\">" N_("Embed fonts") "</option>\n"
                "<option value=\"paths\">" N_("Convert text to paths") "</option>\n"
                "<option value=\"LaTeX\">" N_("Omit text in PDF and create LaTeX file") "</option>\n"
            "</param>\n"
            "<param name=\"blurToBitmap\" gui-text=\"" N_("Rasterize filter effects") "\" type=\"bool\">true</param>\n"
            "<param name=\"resolution\" gui-text=\"" N_("Resolution for rasterization (dpi):") "\" type=\"int\" min=\"1\" max=\"10000\">96</param>\n"
            "<spacer/>"
            "<hbox indent=\"1\"><image>info-outline</image><spacer/><vbox><spacer/>"
                "<label>" N_("When exporting from the Export dialog, you can choose objects to export. 'Save a copy' / 'Save as' will export all pages.") "</label>"
                "<spacer size=\"5\" />"
                "<label>" N_("The page bleed can be set with the Page tool.") "</label>"
            "</vbox></hbox>"
            "<output>\n"
                "<extension>.eps</extension>\n"
                "<mimetype>image/x-e-postscript</mimetype>\n"
                "<filetypename>" N_("Encapsulated PostScript (*.eps)") "</filetypename>\n"
                "<filetypetooltip>" N_("Encapsulated PostScript File") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>", std::make_unique<CairoEpsOutput>());
    // clang-format on

    return;
}

} } }  /* namespace Inkscape, Extension, Implementation */

#endif /* HAVE_CAIRO_PDF */
