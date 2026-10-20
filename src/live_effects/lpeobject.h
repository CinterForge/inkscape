// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LIVEPATHEFFECT_OBJECT_H
#define INKSCAPE_LIVEPATHEFFECT_OBJECT_H

/*
 * Inkscape::LivePathEffect
 *
 * Copyright (C) Johan Engelen 2007-2008 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "effect-enum.h"
#include "object/sp-object.h"
#include "xml/node-observer.h"

namespace Inkscape {
namespace XML { class Node; struct Document; }
namespace LivePathEffect { class Effect; }
} // namespace Inkscape

class LPENodeObserver : public Inkscape::XML::NodeObserver
{
    friend class LivePathEffectObject;
    ~LPENodeObserver() override = default; // can only exist as a direct base of LivePathEffectObject

    void notifyAttributeChanged(Inkscape::XML::Node &node, GQuark key, Inkscape::Util::ptr_shared oldval, Inkscape::Util::ptr_shared newval) final;
};

class LivePathEffectObject final
    : public SPObject
    , private LPENodeObserver
{
public:
    LivePathEffectObject();
    ~LivePathEffectObject() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    Inkscape::LivePathEffect::EffectType effecttype{Inkscape::LivePathEffect::INVALID_LPE};

    bool effecttype_set{false};
    bool deleted{false};
    bool isOnClipboard() const { return _isOnClipboard; };
    // dont check values only structure and ID
    bool is_similar(LivePathEffectObject *that);

    LivePathEffectObject *fork_private_if_necessary(unsigned int nr_of_allowed_users = 1);

    /* Note that the returned pointer can be NULL in a valid LivePathEffectObject contained in a valid list of lpeobjects in an lpeitem!
     * So one should always check whether the returned value is NULL or not */
    Inkscape::LivePathEffect::Effect *get_lpe() { return lpe; }
    Inkscape::LivePathEffect::Effect const *get_lpe() const { return lpe; };

    Inkscape::LivePathEffect::Effect *lpe{nullptr}; // this can be NULL in a valid LivePathEffectObject

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void release() override;

    void set(SPAttr key, char const *value) override;

    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned int flags) override;

private:
    void setOnClipboard();
    bool _isOnClipboard = false;
    friend LPENodeObserver; // for static_cast
    LPENodeObserver &nodeObserver() { return *this; }
};

#endif // INKSCAPE_LIVEPATHEFFECT_OBJECT_H

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
