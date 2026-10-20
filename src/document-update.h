// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_DOCUMENT_UPDATE_H
#define SEEN_DOCUMENT_UPDATE_H

#include <memory>

class SPDocument;
class SPObject;
class SPRoot;

void sp_file_convert_text_baseline_spacing(SPDocument *doc);
void sp_file_convert_font_name(SPDocument *doc);
void sp_file_convert_dpi(SPDocument *doc);
void sp_file_fix_empty_lines(SPDocument *doc);
void sp_file_fix_osb(SPObject *doc);
void sp_file_fix_feComposite(SPObject *doc);
void sp_file_fix_hotspot(SPRoot *o);
void sp_file_fix_lpe(SPDocument *doc);
void sp_file_fix_page_elements(SPDocument *doc);
enum File_DPI_Fix { FILE_DPI_UNCHANGED = 0, FILE_DPI_VIEWBOX_SCALED, FILE_DPI_DOCUMENT_SCALED };
extern int sp_file_convert_dpi_method_commandline;

#endif // SEEN_DOCUMENT_UPDATE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vi: set autoindent shiftwidth=4 tabstop=8 filetype=cpp expandtab softtabstop=4 fileencoding=utf-8 textwidth=99 :
