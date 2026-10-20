// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This code abstracts the libwpg interfaces into the Inkscape
 * input extension interface.
 *
 * This file came from libwpg as a source, their utility wpg2svg
 * specifically.  It has been modified to work as an Inkscape extension.
 * The Inkscape extension code is covered by this copyright, but the
 * rest is covered by the one below.
 */
/* Authors:
 *   Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * Copyright (C) 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "cdr-input.h"

#include <libcdr/libcdr.h>

#include "extension/input.h"
#include "extension/internal/rvng-import-dialog.h"
#include "extension/system.h"

namespace Inkscape::Extension::Internal {

std::unique_ptr<SPDocument> CdrInput::open(Inkscape::Extension::Input *, char const *uri, bool)
{
    return rvng_open(uri, libcdr::CDRDocument::isSupported, libcdr::CDRDocument::parse);
}

#include "clear-n_.h"

void CdrInput::init()
{
    // clang-format off
    /* CDR */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Corel DRAW Input") "</name>\n"
            "<id>org.inkscape.input.cdr</id>\n"
            "<input>\n"
                "<extension>.cdr</extension>\n"
                "<mimetype>image/x-xcdr</mimetype>\n"
                "<filetypename>" N_("Corel DRAW 7-X4 files (*.cdr)") "</filetypename>\n"
                "<filetypetooltip>" N_("Open files saved in Corel DRAW 7-X4") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<CdrInput>());
    /* CDT */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Corel DRAW templates input") "</name>\n"
            "<id>org.inkscape.input.cdt</id>\n"
            "<input>\n"
                "<extension>.cdt</extension>\n"
                "<mimetype>application/x-xcdt</mimetype>\n"
                "<filetypename>" N_("Corel DRAW 7-13 template files (*.cdt)") "</filetypename>\n"
                "<filetypetooltip>" N_("Open files saved in Corel DRAW 7-13") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<CdrInput>());
    /* CCX */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Corel DRAW Compressed Exchange files input") "</name>\n"
            "<id>org.inkscape.input.ccx</id>\n"
            "<input>\n"
                "<extension>.ccx</extension>\n"
                "<mimetype>application/x-xccx</mimetype>\n"
                "<filetypename>" N_("Corel DRAW Compressed Exchange files (*.ccx)") "</filetypename>\n"
                "<filetypetooltip>" N_("Open compressed exchange files saved in Corel DRAW") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<CdrInput>());
    /* CMX */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("Corel DRAW Presentation Exchange files input") "</name>\n"
            "<id>org.inkscape.input.cmx</id>\n"
            "<input>\n"
                "<extension>.cmx</extension>\n"
                "<mimetype>application/x-xcmx</mimetype>\n"
                "<filetypename>" N_("Corel DRAW Presentation Exchange files (*.cmx)") "</filetypename>\n"
                "<filetypetooltip>" N_("Open presentation exchange files saved in Corel DRAW") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<CdrInput>());
    // clang-format on
}

} // namespace Inkscape::Extension::Internal

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
