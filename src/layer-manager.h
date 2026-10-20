// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::LayerManager - a view of a document's layers, relative
 *                          to a particular desktop
 *
 * Copyright 2006  MenTaLguY  <mental@rydia.net>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_LAYER_MANAGER_H
#define SEEN_INKSCAPE_LAYER_MANAGER_H

#include <memory>
#include <glibmm/ustring.h>

#include "document-subset.h"
#include "object/sp-item-group.h"

class SPDesktop;
class SPDocument;

namespace Inkscape {

class ObjectHierarchy;

class LayerManager : public DocumentSubset
{
public:
    LayerManager(SPDesktop *desktop);
    ~LayerManager();

    void renameLayer( SPObject* obj, char const *label, bool uniquify );
    Glib::ustring getNextLayerName( SPObject* obj, char const *label);

    sigc::connection connectCurrentLayerChanged(const sigc::slot<void (SPGroup *)> & slot) {
        return _layer_changed_signal.connect(slot);
    }

    SPGroup *currentRoot() const;
    SPGroup *currentLayer() const;

    unsigned getLayerCount() const { return childCount(currentRoot()); }

    void reset();
    void setCurrentLayer(SPObject *object, bool clear=false);
    void toggleLayerSolo(SPObject *object, bool force_hide = false);
    void toggleHideAllLayers(bool hide);
    void toggleLockAllLayers(bool lock);
    void toggleLockOtherLayers(SPObject *object, bool force_lock = false);
    SPObject *layerForObject(SPObject *object) const;
    std::list<SPItem *> getAllLayers();
    bool isLayer(SPObject *object) const;
    static SPGroup *asLayer(SPObject *object);

    bool isRoot() const { return currentLayer() == currentRoot(); }

private:
    void _objectModified( SPObject* obj, unsigned int flags );
    void _setDocument(SPDesktop *, SPDocument *document);
    void _rebuild();

    void _selectedLayerChanged(SPObject *top, SPObject *bottom);
    void _layer_activated(SPObject *layer);
    void _layer_deactivated(SPObject *layer);

    sigc::connection _layer_connection;
    sigc::connection _activate_connection;
    sigc::connection _deactivate_connection;
    sigc::connection _document_connection;
    sigc::connection _resource_connection;

    SPDesktop *_desktop;
    SPDocument *_document;

    std::unique_ptr<Inkscape::ObjectHierarchy> _layer_hierarchy;
    sigc::signal<void (SPGroup *)> _layer_changed_signal;
};

enum LayerRelativePosition {
    LPOS_ABOVE,
    LPOS_CHILD,
    LPOS_BELOW,
};

SPObject *create_layer(SPObject *root, SPObject *layer, LayerRelativePosition position);
SPObject *next_layer(SPObject *root, SPObject *layer);
SPObject *previous_layer(SPObject *root, SPObject *layer);

} // namespace Inkscape

#endif // SEEN_INKSCAPE_LAYER_MANAGER_H

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
