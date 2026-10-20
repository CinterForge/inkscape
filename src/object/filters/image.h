// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief SVG image filter effect
 *//*
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Hugo Rodrigues <haa.rodrigues@gmail.com>
 *
 * Copyright (C) 2006 Hugo Rodrigues
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_FEIMAGE_H_SEEN
#define SP_FEIMAGE_H_SEEN

#include <memory>
#include "sp-filter-primitive.h"
#include "enums.h"

class SPItem;

namespace Inkscape {
class URIReference;
class DrawingItem;
class Drawing;
class Pixbuf;
} // namespace Inksacpe

class SPFeImage final
    : public SPFilterPrimitive
{
public:
    SPFeImage();
    ~SPFeImage() override;
    int tag() const override { return tag_of<decltype(*this)>; }

private:
    std::string href;

    // preserveAspectRatio
    unsigned char aspect_align = SP_ASPECT_XMID_YMID;
    unsigned char aspect_clip = SP_ASPECT_MEET;

    enum Type
    {
        ELEM,  // If href points to an element that is an SPItem.
        IMAGE, // If href points to non-element that is an image filename.
        NONE   // Neither of the above.
    };
    Type type = NONE;
    std::unique_ptr<Inkscape::URIReference> elemref; // Tracks href if it is a valid URI.
    SPItem *elem; // If type == ELEM, the referenced element.
    std::shared_ptr<Inkscape::Pixbuf const> pixbuf; // If type == IMAGE, the loaded image.

    sigc::connection _href_changed_connection; // Tracks the reference being reattached.
    sigc::connection _href_modified_connection; // If type == ELEM, tracks the referenced object being modified.

    void try_load_image();
    void reread_href();

    void on_href_changed(SPObject *new_elem);
    void on_href_modified();

    struct View
    {
        Inkscape::DrawingItem *parent; // The item to which the filter is applied.
        Inkscape::DrawingItem *child; // The element or image shown by the filter.
        unsigned inner_key; // The display key at which child is shown at.
    };
    std::vector<View> views;
    void create_view(View &v);
    void destroy_view(View &v);

    bool valid_for(SPObject const *obj) const override;

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
	void release() override;
    void set(SPAttr key, char const *value) override;

    void show(Inkscape::DrawingItem *item) override;
    void hide(Inkscape::DrawingItem *item) override;

    std::unique_ptr<Inkscape::Filters::FilterPrimitive> build_renderer(Inkscape::DrawingItem *item) const override;
};

#endif // SP_FEIMAGE_H_SEEN

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
