// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Factory for SPObject tree
 *
 * Authors:
 *   Markus Engel
 *   PBS <pbs3141@gmail.com>
 *
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-factory.h"

// primary
#include "box3d.h"
#include "box3d-side.h"
#include "color-profile.h"
#include "persp3d.h"
#include "sp-anchor.h"
#include "sp-clippath.h"
#include "sp-defs.h"
#include "sp-desc.h"
#include "sp-ellipse.h"
#include "sp-filter.h"
#include "sp-flowdiv.h"
#include "sp-flowregion.h"
#include "sp-flowtext.h"
#include "sp-font.h"
#include "sp-font-face.h"
#include "sp-glyph.h"
#include "sp-glyph-kerning.h"
#include "sp-grid.h"
#include "sp-guide.h"
#include "sp-hatch.h"
#include "sp-hatch-path.h"
#include "sp-image.h"
#include "sp-line.h"
#include "sp-linear-gradient.h"
#include "sp-marker.h"
#include "sp-mask.h"
#include "sp-mesh-gradient.h"
#include "sp-mesh-patch.h"
#include "sp-mesh-row.h"
#include "sp-metadata.h"
#include "sp-missing-glyph.h"
#include "sp-namedview.h"
#include "sp-offset.h"
#include "sp-page.h"
#include "sp-path.h"
#include "sp-pattern.h"
#include "sp-polyline.h"
#include "sp-radial-gradient.h"
#include "sp-rect.h"
#include "sp-root.h"
#include "sp-script.h"
#include "sp-solid-color.h"
#include "sp-spiral.h"
#include "sp-star.h"
#include "sp-stop.h"
#include "sp-string.h"
#include "sp-style-elem.h"
#include "sp-switch.h"
#include "sp-symbol.h"
#include "sp-tag.h"
#include "sp-tag-use.h"
#include "sp-text.h"
#include "sp-textpath.h"
#include "sp-title.h"
#include "sp-tref.h"
#include "sp-tspan.h"
#include "sp-use.h"
#include "live_effects/lpeobject.h"

// filters
#include "filters/blend.h"
#include "filters/colormatrix.h"
#include "filters/componenttransfer.h"
#include "filters/componenttransfer-funcnode.h"
#include "filters/composite.h"
#include "filters/convolvematrix.h"
#include "filters/diffuselighting.h"
#include "filters/displacementmap.h"
#include "filters/distantlight.h"
#include "filters/dropshadow.h"
#include "filters/flood.h"
#include "filters/gaussian-blur.h"
#include "filters/image.h"
#include "filters/merge.h"
#include "filters/mergenode.h"
#include "filters/morphology.h"
#include "filters/offset.h"
#include "filters/pointlight.h"
#include "filters/specularlighting.h"
#include "filters/spotlight.h"
#include "filters/tile.h"
#include "filters/turbulence.h"

#include <unordered_map>

namespace {

class Factory
{
public:
    SPObject *create(std::string const &id) const
    {
        auto it = map.find(id);

        if (it == map.end()) {
            std::cerr << "WARNING: unknown type: " << id << std::endl;
            return nullptr;
        }

        return it->second();
    }

    bool supportsId(std::string const &id) const
    {
        return map.find(id) != map.end();
    }

    static Factory const &get()
    {
        static Factory const singleton;
        return singleton;
    }

private:
    using Func = SPObject*(*)();

    template <typename T>
    static Func constexpr make = [] () -> SPObject* { return new T; };
    static Func constexpr null = [] () -> SPObject* { return nullptr; };

    std::unordered_map<std::string, Func> const map =
    {
        // primary
        { "inkscape:box3d", make<SPBox3D> },
        { "inkscape:box3dside", make<Box3DSide> },
        { "svg:color-profile", make<Inkscape::ColorProfile> },
        { "inkscape:persp3d", make<Persp3D> },
        { "svg:a", make<SPAnchor> },
        { "svg:clipPath", make<SPClipPath> },
        { "svg:defs", make<SPDefs> },
        { "svg:desc", make<SPDesc> },
        { "svg:ellipse", [] () -> SPObject* {
            auto e = new SPGenericEllipse;
            e->type = SP_GENERIC_ELLIPSE_ELLIPSE;
            return e;
        }},
        { "svg:circle", [] () -> SPObject* {
            auto c = new SPGenericEllipse;
            c->type = SP_GENERIC_ELLIPSE_CIRCLE;
            return c;
        }},
        { "arc", [] () -> SPObject* {
            auto a = new SPGenericEllipse;
            a->type = SP_GENERIC_ELLIPSE_ARC;
            return a;
        }},
        { "svg:filter", make<SPFilter> },
        { "svg:flowDiv", make<SPFlowdiv> },
        { "svg:flowSpan", make<SPFlowtspan> },
        { "svg:flowPara", make<SPFlowpara> },
        { "svg:flowLine", make<SPFlowline> },
        { "svg:flowRegionBreak", make<SPFlowregionbreak> },
        { "svg:flowRegion", make<SPFlowregion> },
        { "svg:flowRegionExclude", make<SPFlowregionExclude> },
        { "svg:flowRoot", make<SPFlowtext> },
        { "svg:font", make<SPFont> },
        { "svg:font-face", make<SPFontFace> },
        { "svg:glyph", make<SPGlyph> },
        { "svg:hkern", make<SPHkern> },
        { "svg:vkern", make<SPVkern> },
        { "sodipodi:guide", make<SPGuide> },
        { "svg:view", make<SPPage> },
        { "svg:hatch", make<SPHatch> },
        { "svg:hatchpath", make<SPHatchPath> },
        { "svg:hatchPath", [] () -> SPObject* {
            std::cerr << "Warning: <hatchPath> has been renamed <hatchpath>" << std::endl;
            return new SPHatchPath;
        }},
        { "svg:image", make<SPImage> },
        { "svg:g", make<SPGroup> },
        { "svg:line", make<SPLine> },
        { "svg:linearGradient", make<SPLinearGradient> },
        { "svg:marker", make<SPMarker> },
        { "svg:mask", make<SPMask> },
        { "svg:mesh", [] () -> SPObject* { // SVG 2 old
             std::cerr << "Warning: <mesh> has been renamed <meshgradient>." << std::endl;
             std::cerr << "Warning: <mesh> has been repurposed as a shape that tightly wraps a <meshgradient>." << std::endl;
             return new SPMeshGradient;
        }},
        { "svg:meshGradient", [] () -> SPObject* { // SVG 2 old
             std::cerr << "Warning: <meshGradient> has been renamed <meshgradient>" << std::endl;
             return new SPMeshGradient;
        }},
        { "svg:meshgradient", [] () -> SPObject* { // SVG 2
             return new SPMeshGradient;
        }},
        { "svg:meshPatch", [] () -> SPObject* {
             std::cerr << "Warning: <meshPatch> and <meshRow> have been renamed <meshpatch> and <meshrow>" << std::endl;
             return new SPMeshpatch;
        }},
        { "svg:meshpatch", make<SPMeshpatch> },
        { "svg:meshRow", make<SPMeshrow> },
        { "svg:meshrow", make<SPMeshrow> },
        { "svg:metadata", make<SPMetadata> },
        { "svg:missing-glyph", make<SPMissingGlyph> },
        { "sodipodi:namedview", make<SPNamedView> },
        { "inkscape:offset", make<SPOffset> },
        { "svg:path", make<SPPath> },
        { "svg:pattern", make<SPPattern> },
        { "svg:polygon", make<SPPolygon> },
        { "svg:polyline", make<SPPolyLine> },
        { "svg:radialGradient", make<SPRadialGradient> },
        { "svg:rect", make<SPRect> },
        { "rect", make<SPRect> }, // LPE rect;
        { "svg:svg", make<SPRoot> },
        { "svg:script", make<SPScript> },
        { "svg:solidColor", [] () -> SPObject* {
            std::cerr << "Warning: <solidColor> has been renamed <solidcolor>" << std::endl;
            return new SPSolidColor;
        }},
        { "svg:solidColor", [] () -> SPObject* {
            std::cerr << "Warning: <solidColor> has been renamed <solidcolor>" << std::endl;
            return new SPSolidColor;
        }},
        { "svg:solidcolor", make<SPSolidColor> },
        { "spiral", make<SPSpiral> },
        { "star", make<SPStar> },
        { "svg:stop", make<SPStop> },
        { "string", make<SPString> },
        { "svg:style", make<SPStyleElem> },
        { "svg:switch", make<SPSwitch> },
        { "svg:symbol", make<SPSymbol> },
        { "inkscape:tag", make<SPTag> },
        { "inkscape:tagref", make<SPTagUse> },
        { "svg:text", make<SPText> },
        { "svg:title", make<SPTitle> },
        { "svg:tref", make<SPTRef> },
        { "svg:tspan", make<SPTSpan> },
        { "svg:textPath", make<SPTextPath> },
        { "svg:use", make<SPUse> },
        { "inkscape:path-effect", make<LivePathEffectObject> },

        // filters
        { "svg:feBlend", make<SPFeBlend> },
        { "svg:feColorMatrix", make<SPFeColorMatrix> },
        { "svg:feComponentTransfer", make<SPFeComponentTransfer> },
        { "svg:feFuncR", [] () -> SPObject* { return new SPFeFuncNode(SPFeFuncNode::R); }},
        { "svg:feFuncG", [] () -> SPObject* { return new SPFeFuncNode(SPFeFuncNode::G); }},
        { "svg:feFuncB", [] () -> SPObject* { return new SPFeFuncNode(SPFeFuncNode::B); }},
        { "svg:feFuncA", [] () -> SPObject* { return new SPFeFuncNode(SPFeFuncNode::A); }},
        { "svg:feComposite", make<SPFeComposite> },
        { "svg:feConvolveMatrix", make<SPFeConvolveMatrix> },
        { "svg:feDiffuseLighting", make<SPFeDiffuseLighting> },
        { "svg:feDisplacementMap", make<SPFeDisplacementMap> },
        { "svg:feDistantLight", make<SPFeDistantLight> },
        { "svg:feDropShadow", make<SPFeDropShadow> },
        { "svg:feFlood", make<SPFeFlood> },
        { "svg:feGaussianBlur", make<SPGaussianBlur> },
        { "svg:feImage", make<SPFeImage> },
        { "svg:feMerge", make<SPFeMerge> },
        { "svg:feMergeNode", make<SPFeMergeNode> },
        { "svg:feMorphology", make<SPFeMorphology> },
        { "svg:feOffset", make<SPFeOffset> },
        { "svg:fePointLight", make<SPFePointLight> },
        { "svg:feSpecularLighting", make<SPFeSpecularLighting> },
        { "svg:feSpotLight", make<SPFeSpotLight> },
        { "svg:feTile", make<SPFeTile> },
        { "svg:feTurbulence", make<SPFeTurbulence> },
        { "inkscape:grid", make<SPGrid> },

        // ignore
        { "rdf:RDF", null }, // no SP node yet
        { "inkscape:clipboard", null }, // SP node not necessary
        { "inkscape:templateinfo", null }, // metadata for templates
        { "inkscape:_templateinfo", null }, // metadata for templates
        { "", null } // comments
    };
};

} // namespace

SPObject *SPFactory::createObject(std::string const &id)
{
    return Factory::get().create(id);
}

bool SPFactory::supportsType(std::string const &id)
{
    return Factory::get().supportsId(id);
}

std::string NodeTraits::get_type_string(Inkscape::XML::Node const &node)
{
    switch (node.type()) {
        case Inkscape::XML::NodeType::TEXT_NODE:
            return "string";
        case Inkscape::XML::NodeType::ELEMENT_NODE:
            if (auto sptype = node.attribute("sodipodi:type")) {
                return sptype;
            }
            return node.name();
        default:
            return "";
    }
}

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
