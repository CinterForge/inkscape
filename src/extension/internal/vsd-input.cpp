// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * This code abstracts the libwpg interfaces into the Inkscape
 * input extension interface.
 *
 *  This file came from libwpg as a source, their utility wpg2svg
 *  specifically.  It has been modified to work as an Inkscape extension.
 *  The Inkscape extension code is covered by this copyright, but the
 *  rest is covered by the one below.
 */
/* Authors:
 *   Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * Copyright (C) 2012 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "vsd-input.h"

#include <libvisio/libvisio.h>

#include "extension/input.h"
#include "extension/internal/rvng-import-dialog.h"
#include "extension/system.h"

namespace Inkscape::Extension::Internal {

std::unique_ptr<SPDocument> VsdInput::open(Inkscape::Extension::Input *, char const *uri, bool)
{
    return rvng_open(uri, libvisio::VisioDocument::isSupported, libvisio::VisioDocument::parse);
}

#include "clear-n_.h"

void VsdInput::init()
{
    // clang-format off
    /* VSD */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("VSD Input") "</name>\n"
            "<id>org.inkscape.input.vsd</id>\n"
            "<input>\n"
                "<extension>.vsd</extension>\n"
                "<mimetype>application/vnd.visio</mimetype>\n"
                "<filetypename>" N_("Microsoft Visio Diagram (*.vsd)") "</filetypename>\n"
                "<filetypetooltip>" N_("File format used by Microsoft Visio 6 and later") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<VsdInput>());
    /* VDX */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("VDX Input") "</name>\n"
            "<id>org.inkscape.input.vdx</id>\n"
            "<input>\n"
                "<extension>.vdx</extension>\n"
                "<mimetype>application/vnd.visio</mimetype>\n"
                "<filetypename>" N_("Microsoft Visio XML Diagram (*.vdx)") "</filetypename>\n"
                "<filetypetooltip>" N_("File format used by Microsoft Visio 2010 and later") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<VsdInput>());
    /* VSDM */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("VSDM Input") "</name>\n"
            "<id>org.inkscape.input.vsdm</id>\n"
            "<input>\n"
                "<extension>.vsdm</extension>\n"
                "<mimetype>application/vnd.visio</mimetype>\n"
                "<filetypename>" N_("Microsoft Visio 2013 drawing (*.vsdm)") "</filetypename>\n"
                "<filetypetooltip>" N_("File format used by Microsoft Visio 2013 and later") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<VsdInput>());
    /* VSDX */
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension xmlns=\"" INKSCAPE_EXTENSION_URI "\">\n"
            "<name>" N_("VSDX Input") "</name>\n"
            "<id>org.inkscape.input.vsdx</id>\n"
            "<input>\n"
                "<extension>.vsdx</extension>\n"
                "<mimetype>application/vnd.visio</mimetype>\n"
                "<filetypename>" N_("Microsoft Visio 2013 drawing (*.vsdx)") "</filetypename>\n"
                "<filetypetooltip>" N_("File format used by Microsoft Visio 2013 and later") "</filetypetooltip>\n"
            "</input>\n"
        "</inkscape-extension>", std::make_unique<VsdInput>());
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
