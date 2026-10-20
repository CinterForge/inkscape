// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_SP_NAMEDVIEW_H
#define INKSCAPE_SP_NAMEDVIEW_H

/*
 * <sodipodi:namedview> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2006 Johan Engelen <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000-2002
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include <sigc++/scoped_connection.h>

#include "attributes.h"
#include "snap.h"
#include "sp-object-group.h"

#include "colors/color.h"
#include "svg/svg-bool.h"

namespace Inkscape {
    class CanvasPage;
    namespace Util {
        class Unit;
    }
    namespace Colors {
        class Color;
    }
}

class SPGrid;

enum {
    SP_BORDER_LAYER_BOTTOM,
    SP_BORDER_LAYER_TOP
};

class SPNamedView final : public SPObjectGroup
{
public:
    SPNamedView();
    ~SPNamedView() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    bool editable = true;
    SVGBool showguides{true};
    SVGBool lockguides{false};
    SVGBool clip_to_page{false}; // if true, clip rendered content to pages' boundaries
    SVGBool antialias_rendering{true};
    SVGBool desk_checkerboard{false};
    double zoom = 0;
    double rotation = 0; // Document rotation in degrees (positive is clockwise)
    double cx = 0;
    double cy = 0;
    int window_width = 0;
    int window_height = 0;
    int window_x = 0;
    int window_y = 0;
    int window_maximized = 0;
    SnapManager snap_manager;
    Inkscape::Util::Unit const *display_units = nullptr;   // Units used for the UI (*not* the same as units of SVG coordinates)
    // Inkscape::Util::Unit const *page_size_units; // Only used in "Custom size" part of Document Properties dialog
    GQuark default_layer_id = 0;
    double connector_spacing;
    std::vector<SPGuide *> guides;
    std::vector<SPGrid *> grids;
    std::vector<SPDesktop *> views;
    int viewcount = 0;

    void show(SPDesktop *desktop);
    void hide(SPDesktop const *desktop);
    void setDefaultAttribute(std::string attribute, std::string preference, std::string fallback);
    void activateGuides(void* desktop, bool active);
    char const *getName() const;
    std::vector<SPDesktop *> const getViewList() const;
    Inkscape::Util::Unit const * getDisplayUnit() const;
    void setDisplayUnit(std::string unit);
    void setDisplayUnit(Inkscape::Util::Unit const *unit);

    void translateGuides(Geom::Translate const &translation);
    void translateGrids(Geom::Translate const &translation);
    void scrollAllDesktops(double dx, double dy);

    void toggleShowGuides();
    void toggleLockGuides();
    void toggleShowGrids();

    bool getLockGuides();
    void setLockGuides(bool v);

    void setShowGuides(bool v);
    bool getShowGuides();

    void updateViewPort();

    // page background, border, desk colors
    void change_color(SPAttr color_key, SPAttr opacity_key, Inkscape::Colors::Color const &color);
    // show border, border on top, anti-aliasing, ...
    void change_bool_setting(SPAttr key, bool value);
    // sync desk colors
    void set_desk_color(SPDesktop* desktop);
    // turn clip to page mode on/off
    void set_clip_to_page(SPDesktop* desktop, bool enable);
    // immediate show/hide guides request, not recorded in a named view
    void temporarily_show_guides(bool show);
    // coordinate system origin correction
    bool get_origin_follows_page() const { return _origin_correction; }
    void set_origin_follows_page(bool on);
    // Y axis orientation
    bool is_y_axis_down() const { return _y_axis_down; }
    void set_y_axis_down(bool down);
    // fix guidelines positions after Y axis flip
    void fix_guidelines();

    SPGrid *getFirstEnabledGrid();

    Inkscape::Colors::Color getDeskColor() const;
    Inkscape::Colors::Color getGuideColor() const;
    Inkscape::Colors::Color getGuideHiColor() const;

private:
    void updateGuides();
    void updateGrids();

    bool getShowGrids();
    void setShowGrids(bool v);

    void setShowGuideSingle(SPGuide *guide);

    friend class SPDocument;

    std::unique_ptr<Inkscape::CanvasPage> _viewport;
    std::optional<Inkscape::Colors::Color> _desk_color;
    std::optional<Inkscape::Colors::Color> _guide_color;
    std::optional<Inkscape::Colors::Color> _guide_hi_color;
    double _guide_opacity = 0.6;
    double _guide_hi_opacity = 0.5;
    // if true, move coordinate system origin to the current page; if false - keep origin on front page
    SVGBool _origin_correction{true};
    SVGBool _y_axis_down{true};

    sigc::scoped_connection _page_added;

protected:
    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void release() override;
    void modified(unsigned int flags) override;
    void update(SPCtx *ctx, unsigned int flags) override;
    void set(SPAttr key, char const* value) override;

    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void remove_child(Inkscape::XML::Node* child) override;
    void order_changed(Inkscape::XML::Node *child, Inkscape::XML::Node *old_repr,
                       Inkscape::XML::Node *new_repr) override;

    Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
};


void sp_namedview_window_from_document(SPDesktop *desktop);
void sp_namedview_zoom_and_view_from_document(SPDesktop *desktop);
void sp_namedview_document_from_window(SPDesktop *desktop);
void sp_namedview_update_layers_from_document (SPDesktop *desktop);

const Inkscape::Util::Unit* sp_parse_document_units(const char* unit);


#endif /* !INKSCAPE_SP_NAMEDVIEW_H */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
