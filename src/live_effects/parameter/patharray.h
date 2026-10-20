// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINALPATHARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINALPATHARRAY_H

#include <memory>
#include <vector>

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/treemodel.h>

#include <2geom/pathvector.h>

#include "live_effects/effect-enum.h"          // for ParamType
#include "live_effects/parameter/parameter.h"  // for Parameter
#include "object/uri-references.h"             // for URIReference

namespace Gtk {
class TreeStore;
class TreeView;
class ScrolledWindow;
} // namespace Gtk

class SPObject;

namespace Inkscape::LivePathEffect {

class PathAndDirectionAndVisible {
public:
    PathAndDirectionAndVisible(SPObject *owner)
        : ref(owner)
    {
    }

    Glib::ustring href;
    URIReference ref;
    Geom::PathVector _pathvector = {};
    bool reversed = false;
    bool visibled = true;
    
    sigc::connection linked_changed_connection;
    sigc::connection linked_release_connection;
    sigc::connection linked_modified_connection;
};

class PathArrayParam : public Parameter
{
public:
    class ModelColumns;

    PathArrayParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                   Inkscape::UI::Widget::Registry *wr, Effect *effect);
    ~PathArrayParam() override;

    PathArrayParam(const PathArrayParam &) = delete;
    PathArrayParam &operator=(const PathArrayParam &) = delete;

    Gtk::Widget * param_newWidget() override;
    std::vector<SPObject *> param_get_satellites() override;
    bool param_readSVGValue(char const * strvalue) override;
    Glib::ustring param_getSVGValue() const override;
    Glib::ustring param_getDefaultSVGValue() const override;
    void param_set_default() override;
    void param_update_default(char const * default_value) override{};
    /** Disable the canvas indicators of parent class by overriding this method */
    void param_editOncanvas(SPItem * /*item*/, SPDesktop * /*dt*/) override {};
    /** Disable the canvas indicators of parent class by overriding this method */
    void addCanvasIndicators(SPLPEItem const* /*lpeitem*/, std::vector<Geom::PathVector> & /*hp_vec*/) override {};
    void setFromOriginalD(bool from_original_d){ _from_original_d = from_original_d; update();};
    void allowOnlyBsplineSpiro(bool allow_only_bspline_spiro){ _allow_only_bspline_spiro = allow_only_bspline_spiro; update();};
    std::vector<PathAndDirectionAndVisible*> _vector;
    ParamType paramType() const override { return ParamType::PATH_ARRAY; };

private:
    friend class LPEFillBetweenMany;

    bool _updateLink(const Gtk::TreeModel::iterator& iter, PathAndDirectionAndVisible* pd);
    bool _selectIndex(const Gtk::TreeModel::iterator& iter, int* i);
    void unlink(PathAndDirectionAndVisible* to);
    void start_listening();
    void setPathVector(SPObject *linked_obj, guint flags, PathAndDirectionAndVisible* to);
    
    void linked_changed(SPObject *old_obj, SPObject *new_obj, PathAndDirectionAndVisible* to);
    void linked_modified(SPObject *linked_obj, guint flags, PathAndDirectionAndVisible* to);
    void linked_release(SPObject *release, PathAndDirectionAndVisible* to);
    
    std::unique_ptr<ModelColumns> _model;
    Glib::RefPtr<Gtk::TreeStore> _store;
    std::unique_ptr<Gtk::TreeView> _tree;
    std::unique_ptr<Gtk::ScrolledWindow> _scroller;
    
    void on_link_button_click();
    void on_remove_button_click();
    void on_up_button_click();
    void on_down_button_click();
    void on_reverse_toggled(const Glib::ustring& path);
    void on_visible_toggled(const Glib::ustring& path);
    
    bool _from_original_d = false;
    bool _allow_only_bspline_spiro = false;

    void update();
    void initui();
};

} // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_ORIGINALPATHARRAY_H

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
