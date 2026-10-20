// SPDX-License-Identifier: GPL-2.0-or-later
#include <gtest/gtest.h>
#include <functional>
#include <typeinfo>

#include "object/box3d.h"
#include "object/box3d-side.h"
#include "object/color-profile.h"
#include "object/persp3d.h"
#include "object/sp-anchor.h"
#include "object/sp-clippath.h"
#include "object/sp-defs.h"
#include "object/sp-desc.h"
#include "object/sp-ellipse.h"
#include "object/sp-filter.h"
#include "object/sp-flowdiv.h"
#include "object/sp-flowregion.h"
#include "object/sp-flowtext.h"
#include "object/sp-font.h"
#include "object/sp-font-face.h"
#include "object/sp-glyph.h"
#include "object/sp-glyph-kerning.h"
#include "object/sp-grid.h"
#include "object/sp-guide.h"
#include "object/sp-hatch.h"
#include "object/sp-hatch-path.h"
#include "object/sp-image.h"
#include "object/sp-line.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-marker.h"
#include "object/sp-mask.h"
#include "object/sp-mesh-gradient.h"
#include "object/sp-mesh-patch.h"
#include "object/sp-mesh-row.h"
#include "object/sp-metadata.h"
#include "object/sp-missing-glyph.h"
#include "object/sp-namedview.h"
#include "object/sp-offset.h"
#include "object/sp-page.h"
#include "object/sp-path.h"
#include "object/sp-pattern.h"
#include "object/sp-polyline.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-rect.h"
#include "object/sp-root.h"
#include "object/sp-script.h"
#include "object/sp-solid-color.h"
#include "object/sp-spiral.h"
#include "object/sp-star.h"
#include "object/sp-stop.h"
#include "object/sp-string.h"
#include "object/sp-style-elem.h"
#include "object/sp-switch.h"
#include "object/sp-symbol.h"
#include "object/sp-tag.h"
#include "object/sp-tag-use.h"
#include "object/sp-text.h"
#include "object/sp-textpath.h"
#include "object/sp-title.h"
#include "object/sp-tref.h"
#include "object/sp-tspan.h"
#include "object/sp-use.h"
#include "live_effects/lpeobject.h"
#include "object/filters/blend.h"
#include "object/filters/colormatrix.h"
#include "object/filters/componenttransfer.h"
#include "object/filters/componenttransfer-funcnode.h"
#include "object/filters/composite.h"
#include "object/filters/convolvematrix.h"
#include "object/filters/diffuselighting.h"
#include "object/filters/displacementmap.h"
#include "object/filters/distantlight.h"
#include "object/filters/dropshadow.h"
#include "object/filters/flood.h"
#include "object/filters/gaussian-blur.h"
#include "object/filters/image.h"
#include "object/filters/merge.h"
#include "object/filters/mergenode.h"
#include "object/filters/morphology.h"
#include "object/filters/offset.h"
#include "object/filters/pointlight.h"
#include "object/filters/specularlighting.h"
#include "object/filters/spotlight.h"
#include "object/filters/tile.h"
#include "object/filters/turbulence.h"

namespace {

// Error reporting function, because asserts can only be used inside test body.
using F = std::function<void(char const*, char const*, bool, bool)>;

// Ensure tree structure is consistent with actual class hierarchy.
template <typename A, typename B>
void test_real(F const &f)
{
    constexpr bool b1 = std::is_base_of_v<A, B>;
    constexpr bool b2 = first_tag<A> <= tag_of<B> && tag_of<B> <= last_tag<A>;

    if constexpr (b1 != b2) {
        f(typeid(A).name(), typeid(B).name(), b1, b2);
    }
}

template <typename A, typename B>
void test_dispatcher2(F const &f)
{
    test_real<A, B>(f);
    test_real<B, A>(f);
}

template <typename A, typename B, typename... T>
void test_dispatcher(F const &f)
{
    test_dispatcher2<A, B>(f);
    if constexpr (sizeof...(T) >= 1) {
        test_dispatcher<A, T...>(f);
    }
}

// Calls test_real<A, B> for all distinct pairs of types A, B in T.
template <typename A, typename... T>
void test(F const &f)
{
    if constexpr (sizeof...(T) >= 1) {
        test_dispatcher<A, T...>(f);
    }
    if constexpr (sizeof...(T) >= 2) {
        test<T...>(f);
    }
}

} // namespace

TEST(SPObjectTagsTest, compare_dynamic_cast)
{
    test<
        SPObject,
        Inkscape::ColorProfile,
        LivePathEffectObject,
        Persp3D,
        SPDefs,
        SPDesc,
        SPFeDistantLight,
        SPFeFuncNode,
        SPFeMergeNode,
        SPFePointLight,
        SPFeSpotLight,
        SPFilter,
        SPFilterPrimitive,
        SPFeBlend,
        SPFeColorMatrix,
        SPFeComponentTransfer,
        SPFeComposite,
        SPFeConvolveMatrix,
        SPFeDiffuseLighting,
        SPFeDisplacementMap,
        SPFeDropShadow,
        SPFeFlood,
        SPFeImage,
        SPFeMerge,
        SPFeMorphology,
        SPFeOffset,
        SPFeSpecularLighting,
        SPFeTile,
        SPFeTurbulence,
        SPGaussianBlur,
        SPFlowline,
        SPFlowregionbreak,
        SPFont,
        SPFontFace,
        SPGlyph,
        SPGlyphKerning,
        SPHkern,
        SPVkern,
        SPGrid,
        SPGuide,
        SPHatchPath,
        SPItem,
        SPFlowdiv,
        SPFlowpara,
        SPFlowregion,
        SPFlowregionExclude,
        SPFlowtext,
        SPFlowtspan,
        SPImage,
        SPLPEItem,
        SPGroup,
        SPAnchor,
        SPBox3D,
        SPMarker,
        SPRoot,
        SPSwitch,
        SPSymbol,
        SPShape,
        SPGenericEllipse,
        SPLine,
        SPOffset,
        SPPath,
        SPPolyLine,
        SPPolygon,
        Box3DSide,
        SPRect,
        SPSpiral,
        SPStar,
        SPTRef,
        SPTSpan,
        SPText,
        SPTextPath,
        SPUse,
        SPMeshpatch,
        SPMeshrow,
        SPMetadata,
        SPMissingGlyph,
        SPObjectGroup,
        SPClipPath,
        SPMask,
        SPNamedView,
        SPPage,
        SPPaintServer,
        SPGradient,
        SPLinearGradient,
        SPMeshGradient,
        SPRadialGradient,
        SPHatch,
        SPPattern,
        SPSolidColor,
        SPScript,
        SPStop,
        SPString,
        SPStyleElem,
        SPTag,
        SPTagUse,
        SPTitle
    >([&] (char const *a, char const *b, bool b1, bool b2) {
        ADD_FAILURE() << "For downcasting " << a << " -> " << b << ", got " << b2 << ", expected " << b1;
    });
}
