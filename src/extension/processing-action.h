// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2023 Edgewood
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_EXTENSION_PROCESSING_ACTION_H__
#define INKSCAPE_EXTENSION_PROCESSING_ACTION_H__

#include <string>  // for string

class SPDocument;

namespace Inkscape::XML {
class Node;
}

namespace Inkscape::Extension {

class ProcessingAction {
public:
    ProcessingAction (Inkscape::XML::Node *in_repr);
    ~ProcessingAction () = default;
    bool is_enabled();
    void run(SPDocument *doc);

    std::string const &get_action_name() const { return _action_name; }
private:
    Inkscape::XML::Node * _repr;
    std::string _action_name;
    std::string _pref;
    bool _pref_default = true;
};

}  // namespace Inkscape::Extension

#endif /* INKSCAPE_EXTENSION_PROCESSING_ACTION_H__ */

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
