// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Code for handling XSLT extensions
 *
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2006-2007 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_EXTENSION_IMPEMENTATION_XSLT_H
#define SEEN_INKSCAPE_EXTENSION_IMPEMENTATION_XSLT_H

#include <string>

#include "implementation.h"

#include "libxml/tree.h"
#include "libxslt/xslt.h"
#include "libxslt/xsltInternals.h"

namespace Inkscape::XML {
class Node;
} // namespace Inkscape::XML

namespace Inkscape::Extension::Implementation {

class XSLT : public Implementation {
private:
    std::string _filename;
    xmlDocPtr _parsedDoc = nullptr;
    xsltStylesheetPtr _stylesheet = nullptr;

public:
    XSLT() = default;

    bool load(Inkscape::Extension::Extension *module) override;
    void unload(Inkscape::Extension::Extension *module) override;
    void effect(Inkscape::Extension::Effect *module, ExecutionEnv *env, SPDocument *document) override;

    bool check(Inkscape::Extension::Extension *module) override;

    std::unique_ptr<SPDocument> open(Inkscape::Extension::Input *module, char const *filename, bool is_importing) override;
    void save(Inkscape::Extension::Output *module, SPDocument *doc, char const *filename) override;
};

} // namespace Inkscape::Extension::Implementation

#endif // SEEN_INKSCAPE_EXTENSION_IMPEMENTATION_XSLT_H

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
