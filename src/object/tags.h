// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SP_OBJECT_TAGS_H
#define SP_OBJECT_TAGS_H

#include "util/cast.h"

// Class hierarchy structure

#define SPOBJECT_HIERARCHY_DATA(X)\
X(SPObject,\
    X(ColorProfile_PLACEHOLDER)\
    X(LivePathEffectObject)\
    X(Persp3D)\
    X(SPDefs)\
    X(SPDesc)\
    X(SPFeDistantLight)\
    X(SPFeFuncNode)\
    X(SPFeMergeNode)\
    X(SPFePointLight)\
    X(SPFeSpotLight)\
    X(SPFilter)\
    X(SPFilterPrimitive,\
        X(SPFeBlend)\
        X(SPFeColorMatrix)\
        X(SPFeComponentTransfer)\
        X(SPFeComposite)\
        X(SPFeConvolveMatrix)\
        X(SPFeDiffuseLighting)\
        X(SPFeDisplacementMap)\
        X(SPFeDropShadow)\
        X(SPFeFlood)\
        X(SPFeImage)\
        X(SPFeMerge)\
        X(SPFeMorphology)\
        X(SPFeOffset)\
        X(SPFeSpecularLighting)\
        X(SPFeTile)\
        X(SPFeTurbulence)\
        X(SPGaussianBlur)\
    )\
    X(SPFlowline)\
    X(SPFlowregionbreak)\
    X(SPFont)\
    X(SPFontFace)\
    X(SPGlyph)\
    X(SPGlyphKerning,\
        X(SPHkern)\
        X(SPVkern)\
    )\
    X(SPGrid)\
    X(SPGuide)\
    X(SPHatchPath)\
    X(SPItem,\
        X(SPFlowdiv)\
        X(SPFlowpara)\
        X(SPFlowregion)\
        X(SPFlowregionExclude)\
        X(SPFlowtext)\
        X(SPFlowtspan)\
        X(SPImage)\
        X(SPLPEItem,\
            X(SPGroup,\
                X(SPAnchor)\
                X(SPBox3D)\
                X(SPMarker)\
                X(SPRoot)\
                X(SPSwitch)\
                X(SPSymbol)\
            )\
            X(SPShape,\
                X(SPGenericEllipse)\
                X(SPLine)\
                X(SPOffset)\
                X(SPPath)\
                X(SPPolyLine)\
                X(SPPolygon,\
                    X(Box3DSide)\
                )\
                X(SPRect)\
                X(SPSpiral)\
                X(SPStar)\
            )\
        )\
        X(SPTRef)\
        X(SPTSpan)\
        X(SPText)\
        X(SPTextPath)\
        X(SPUse)\
    )\
    X(SPMeshpatch)\
    X(SPMeshrow)\
    X(SPMetadata)\
    X(SPMissingGlyph)\
    X(SPObjectGroup,\
        X(SPClipPath)\
        X(SPMask)\
        X(SPNamedView)\
    )\
    X(SPPage)\
    X(SPPaintServer,\
        X(SPGradient,\
            X(SPLinearGradient)\
            X(SPMeshGradient)\
            X(SPRadialGradient)\
        )\
        X(SPHatch)\
        X(SPPattern)\
        X(SPSolidColor)\
    )\
    X(SPScript)\
    X(SPStop)\
    X(SPString)\
    X(SPStyleElem)\
    X(SPTag)\
    X(SPTagUse)\
    X(SPTitle)\
)

// Forward declarations

#define X(n, ...) class n; __VA_ARGS__
SPOBJECT_HIERARCHY_DATA(X)
#undef X

// Tag generation

enum class SPObjectTag : int
{
    #define X(n, ...) n##_first, __VA_ARGS__ n##_tmp, n##_last = n##_tmp - 1,
    SPOBJECT_HIERARCHY_DATA(X)
    #undef X
};

// Tag specialization

#define X(n, ...) template <> inline constexpr int first_tag<n> = static_cast<int>(SPObjectTag::n##_first); __VA_ARGS__
SPOBJECT_HIERARCHY_DATA(X)
#undef X

#define X(n, ...) template <> inline constexpr int last_tag<n> = static_cast<int>(SPObjectTag::n##_last); __VA_ARGS__
SPOBJECT_HIERARCHY_DATA(X)
#undef X

// Special case for Inkscape::ColorProfile which lives in its own namespace.

namespace Inkscape { class ColorProfile; }

template <> inline constexpr int first_tag<Inkscape::ColorProfile> = first_tag<ColorProfile_PLACEHOLDER>;
template <> inline constexpr int last_tag <Inkscape::ColorProfile> = last_tag <ColorProfile_PLACEHOLDER>;

#undef SPOBJECT_HIERARCHY_DATA

#endif // SP_OBJECT_TAGS_H

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
