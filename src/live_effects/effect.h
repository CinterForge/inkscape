// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) Johan Engelen 2007-2012 <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LIVEPATHEFFECT_H
#define INKSCAPE_LIVEPATHEFFECT_H

#include "effect-enum.h"
#include "parameter/bool.h"
#include "parameter/hidden.h"
#include "ui/widget/registry.h"
#include <2geom/forward.h>
#include <glibmm/ustring.h>
#include <iostream>

#define  LPE_CONVERSION_TOLERANCE 0.01    // FIXME: find good solution for this.

class  SPDocument;
class  SPDesktop;
class  SPItem;
class  LivePathEffectObject;
class  SPLPEItem;
class  KnotHolder;
class  KnotHolderEntity;
class  SPPath;

namespace Gtk {
    class Widget;
}

namespace Inkscape {

namespace XML {
    class Node;
}

namespace LivePathEffect {

enum LPEPathFlashType {
    SUPPRESS_FLASH,
//    PERMANENT_FLASH,
    DEFAULT
};

enum LPEAction
{
    LPE_NONE = 0,
    LPE_ERASE,
    LPE_TO_OBJECTS,
    LPE_VISIBILITY,
    LPE_UPDATE
};

// maybe can grow in the future with more SPItemData
struct LPEItemShapesNumbers {
    size_t nchildshapes = 0;
    size_t nsubpaths = 0;
    size_t ncurves = 0;
    inline bool operator==(LPEItemShapesNumbers const &other) const = default;
    inline bool operator!=(LPEItemShapesNumbers other) const
    {
        return !(*this == other);
    }
};
inline std::ostream &operator<<(std::ostream &os, LPEItemShapesNumbers const &lpeitemnumbers) {
    os << "Number of child shapes:" << lpeitemnumbers.nchildshapes << std::endl;
    os << "Number of subpaths:" << lpeitemnumbers.nsubpaths << std::endl;
    os << "Number of curves:" << lpeitemnumbers.ncurves; 
    return os;
}
class Effect {
public:
    static Effect* New(EffectType lpenr, LivePathEffectObject *lpeobj);
    static void createAndApply(const char* name, SPDocument *doc, SPItem *item);
    static void createAndApply(EffectType type, SPDocument *doc, SPItem *item);

    virtual ~Effect();
    Effect(const Effect&) = delete;
    Effect& operator=(const Effect&) = delete;

    EffectType effectType() const;

    //basically, to get this method called before the derived classes, a bit
    //of indirection is needed. We first call these methods, then the below.
    void doAfterEffect_impl(SPLPEItem const *lpeitem, Geom::PathVector *curve);
    void doOnApply_impl(SPLPEItem const* lpeitem);
    void doBeforeEffect_impl(SPLPEItem const* lpeitem);
    void doOnOpen_impl();
    void doOnRemove_impl(SPLPEItem const* lpeitem);
    void transform_multiply_impl(Geom::Affine const &postmul, SPLPEItem *);
    void doOnBeforeCommit();
    void read_from_SVG();
    void setCurrentZoom(double cZ);
    void setSelectedNodePoints(std::vector<Geom::Point> sNP);
    bool isNodePointSelected(Geom::Point const &nodePoint) const;
    bool isOnClipboard();
    std::vector<SPLPEItem *> getCurrrentLPEItems() const;
    void update_satellites();
    virtual void doOnException(SPLPEItem const *lpeitem);
    virtual void doOnVisibilityToggled(SPLPEItem const* lpeitem);
    virtual void adjustForNewPath();
    void writeParamsToSVG();
    std::vector<SPObject *> effect_get_satellites(bool force = true);
    virtual void acceptParamPath (SPPath const* param_path);
    static int acceptsNumClicks(EffectType type);
    int acceptsNumClicks() const { return acceptsNumClicks(effectType()); }
    SPShape * getCurrentShape() const { return current_shape; };
    void setCurrentShape(SPShape * shape) { current_shape = shape; }
    virtual void processObjects(LPEAction lpe_action);
    void makeUndoDone(Inkscape::Util::Internal::ContextString message);
    /*
     * isReady() indicates whether all preparations which are necessary to apply the LPE are done,
     * e.g., waiting for a parameter path either before the effect is created or when it needs a
     * path as argument. This is set in SPLPEItem::addPathEffect().
     */
    inline bool isReady() const { return is_ready; }
    inline void setReady(bool ready = true) { is_ready = ready; }

    virtual void doEffect(Geom::PathVector &curve);

    virtual Gtk::Widget * newWidget();
    /**
     * Sets all parameters to their default values and writes them to SVG.
     */
    virtual void resetDefaults(SPItem const* item);

    // /TODO: providesKnotholder() is currently used as an indicator of whether a nodepath is
    // created for an item or not. When we allow both at the same time, this needs rethinking!
    bool providesKnotholder() const;
    // /TODO: in view of providesOwnFlashPaths() below, this is somewhat redundant
    //       (but spiro lpe still needs it!)
    virtual LPEPathFlashType pathFlashType() const { return DEFAULT; }
    void addHandles(KnotHolder *knotholder, SPItem *item);
    std::vector<Geom::PathVector> getCanvasIndicators(SPLPEItem const* lpeitem);
    void update_helperpath();
    bool has_exception;

    inline bool providesOwnFlashPaths() const {
        return provides_own_flash_paths || show_orig_path;
    }
    inline bool showOrigPath() const { return show_orig_path; }

    Glib::ustring          getName() const;
    Inkscape::XML::Node *  getRepr();
    SPDocument *           getSPDoc();
    LivePathEffectObject * getLPEObj() {return lpeobj;};
    LivePathEffectObject const * getLPEObj() const {return lpeobj;};
    Parameter *            getParameter(const char * key);

    void readallParameters(Inkscape::XML::Node const* repr);
    void setParameter(const gchar * key, const gchar * new_value);

    inline bool isVisible() const { return is_visible; }

    void editNextParamOncanvas(SPItem * item, SPDesktop * desktop);
    bool apply_to_clippath_and_mask;
    bool keep_paths; // set this to false allow retain extra generated objects, see measure line LPE
    bool is_load;
    bool is_applied;
    bool on_remove_all;
    bool refresh_widgets;
    bool finishiddle = false;
    bool satellitestoclipboard = false;
    bool helperLineSatellites = false;
    gint spinbutton_width_chars = 7;
    void setLPEAction(LPEAction lpe_action) { _lpe_action = lpe_action; }
    BoolParam is_visible;
    HiddenParam lpeversion;
    Geom::PathVector pathvector_before_effect;
    Geom::PathVector pathvector_after_effect;
    SPLPEItem *sp_lpe_item = nullptr; // these get stored in doBeforeEffect_impl, and derived classes may do as they please with
                            // them.
    SPShape *current_shape; // these get stored in performPathEffects.
    std::vector<Parameter *> param_vector;
    void setDefaultParameters();
    void resetDefaultParameters();
    bool hasDefaultParameters();
    virtual bool getHolderRemove() { return false; }
protected:
    Effect(LivePathEffectObject *lpeobject);
    friend class SatelliteArrayParam;
    friend class LPEMeasureSegments;
    // adjust path study make public if grow
    bool _adjust_path = false;
    // provide a set of doEffect functions so the developer has a choice
    // of what kind of input/output parameters he desires.
    // the order in which they appear is the order in which they are
    // called by this base class. (i.e. doEffect(Geom::PathVector *curve) defaults to calling
    // doEffect(Geom::PathVector )
    virtual Geom::PathVector
            doEffect_path (Geom::PathVector const & path_in);
    virtual Geom::Piecewise<Geom::D2<Geom::SBasis> >
            doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in);

    void registerParameter(Parameter * param);
    Parameter * getNextOncanvasEditableParam();

    virtual void addKnotHolderEntities(KnotHolder * /*knotholder*/, SPItem * /*item*/) {};

    virtual void addCanvasIndicators(SPLPEItem const* lpeitem, std::vector<Geom::PathVector> &hp_vec);

    bool _provides_knotholder_entities;
    bool _provides_path_adjustment = false;
    LPEAction _lpe_action = LPE_NONE;
    int oncanvasedit_it;
    bool show_orig_path; // set this to true in derived effects to automatically have the original
                         // path displayed as helperpath
    // this boolean defaults to false, it concatenates the input path to one pwd2,
    // instead of normally 'splitting' the path into continuous pwd2 paths and calling doEffect_pwd2 for each.
    bool concatenate_before_pwd2;
    double current_zoom;
    std::vector<Geom::Point> selectedNodesPoints;
    Inkscape::UI::Widget::Registry wr;
private:
    LivePathEffectObject *lpeobj;
    virtual void transform_multiply(Geom::Affine const &postmul, bool set);
    virtual bool doOnOpen(SPLPEItem const *lpeitem);
    virtual void doAfterEffect(SPLPEItem const *lpeitem, Geom::PathVector *curve);
    // we want to call always to overrided methods not effect ones
    virtual void doOnRemove(SPLPEItem const* /*lpeitem*/);
    virtual void doOnApply (SPLPEItem const* lpeitem);
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    
    void setDefaultParam(Glib::ustring pref_path, Parameter *param);
    void unsetDefaultParam(Glib::ustring pref_path, Parameter *param);
    bool provides_own_flash_paths; // if true, the standard flash path is suppressed
    sigc::connection _before_commit_connection;
    LPEItemShapesNumbers _lpenumbers;
    bool is_ready;
    bool defaultsopen;
};

bool can_have_lpe(SPObject const* object);

} //namespace LivePathEffect
} //namespace Inkscape


#endif // INKSCAPE_LIVEPATHEFFECT_H

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
