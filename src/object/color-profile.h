// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SPObject of the color-profile object found a direct child of defs.
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLOR_PROFILE_H
#define SEEN_COLOR_PROFILE_H

#include "colors/cms/system.h"
#include "colors/spaces/enum.h" // RenderingIntent
#include "object/uri.h"
#include "sp-object.h"

namespace Inkscape {

class URI;

enum class ColorProfileStorage
{
    HREF_DATA,
    HREF_FILE,
    LOCAL_ID,
};

class ColorProfile final : public SPObject
{
public:
    ColorProfile() = default;
    ~ColorProfile() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    static ColorProfile *createFromProfile(SPDocument *doc, Colors::CMS::Profile const &profile,
                                           std::string const &name, ColorProfileStorage storage,
                                           std::optional<Colors::RenderingIntent> intent);

    std::string getName() const { return _name; }
    std::string getLocalProfileId() const { return _local; }
    std::string getProfileData() const;
    Colors::RenderingIntent getRenderingIntent() const { return _intent; }

    // This is the only variable we expect inkscape to modify. Changing the icc
    // profile data or ID should instead involve creating a new ColorProfile element.
    void setRenderingIntent(Colors::RenderingIntent intent);

    // For unit test usage
    Inkscape::URI const *getUri() const { return _uri.get(); }

protected:
    void build(SPDocument *doc, Inkscape::XML::Node *repr) override;
    void release() override;

    void set(SPAttr key, char const *value) override;

    Inkscape::XML::Node *write(Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, unsigned int flags) override;

private:
    std::string _name;
    std::string _local;
    Colors::RenderingIntent _intent;

    std::unique_ptr<Inkscape::URI> _uri;
};

} // namespace Inkscape

#endif // !SEEN_COLOR_PROFILE_H

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
