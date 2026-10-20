// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LivePathEffectParameters
 *
 * Copyright (C) Theodore Janeczko 2012 <flutterguy317@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_PARAMETER_SATELLITEARRAY_H
#define INKSCAPE_LIVEPATHEFFECT_PARAMETER_SATELLITEARRAY_H

#include <memory>
#include <vector>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/treemodel.h>

#include <sigc++/scoped_connection.h>
#include "live_effects/lpeobject.h"
#include "live_effects/parameter/array.h"
#include "live_effects/parameter/parameter.h"
#include "live_effects/parameter/satellite-reference.h"

namespace Gtk {
class TreeStore;
class TreeView;
class ScrolledWindow;
} // namespace Gtk

class SPObject;

namespace Inkscape::LivePathEffect {

class SatelliteReference;

class SatelliteArrayParam : public ArrayParam<std::shared_ptr<SatelliteReference>>
{
public:
    class ModelColumns;

    SatelliteArrayParam(const Glib::ustring &label, const Glib::ustring &tip, const Glib::ustring &key,
                        Inkscape::UI::Widget::Registry *wr, Effect *effect, bool visible);
    ~SatelliteArrayParam() override;

    SatelliteArrayParam(const SatelliteArrayParam &) = delete;
    SatelliteArrayParam &operator=(const SatelliteArrayParam &) = delete;

    Gtk::Widget *param_newWidget() override;
    bool param_readSVGValue(char const * strvalue) override;
    void link(SPObject *to, size_t pos = Glib::ustring::npos);
    void unlink(std::shared_ptr<SatelliteReference> const &to);
    void unlink(SPObject *to);
    bool is_connected(){ return linked_connections.size() != 0; };
    void clear();
    void start_listening();
    void quit_listening();
    ParamType paramType() const override { return ParamType::SATELLITE_ARRAY; };

private:
    void linked_modified(SPObject *linked_obj, guint flags);
    bool _selectIndex(const Gtk::TreeModel::iterator &iter, int *i);
    void updatesignal();

    std::unique_ptr<ModelColumns> _model;
    Glib::RefPtr<Gtk::TreeStore> _store;
    std::unique_ptr<Gtk::TreeView> _tree;
    std::unique_ptr<Gtk::ScrolledWindow> _scroller;

    void on_link_button_click();
    void on_remove_button_click();
    void move_up_down(int delta, bool is_up);
    void on_up_button_click();
    void on_down_button_click();
    void on_active_toggled(const Glib::ustring &item);

    void update();
    void initui();

    bool _visible{};
    std::vector<sigc::scoped_connection> linked_connections;
    std::vector<SPObject *> param_get_satellites() override;
};

} // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_LIVEPATHEFFECT_PARAMETER_SATELLITEARRAY_H

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
