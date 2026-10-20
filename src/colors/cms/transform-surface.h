// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors: see git history
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_COLORS_CMS_TRANSFORM_SURFACE_H
#define SEEN_COLORS_CMS_TRANSFORM_SURFACE_H

#include <array>
#include <limits>
#include <memory>
#include <vector>

#include "profile.h"
#include "transform.h"

namespace Inkscape::Colors::CMS {

class Profile;

class TransformContext
{
    static void cms_error_handler(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text)
    {
         fprintf(stderr, "  ---- LCMS error: %s (ErrorCode: %d)\n", Text, ErrorCode);
    }  
public:
    TransformContext(TransformContext const &) = delete;
    TransformContext &operator=(TransformContext const &) = delete;

    TransformContext()
        : _context{cmsCreateContext(nullptr, nullptr)}
    {
        cmsSetLogErrorHandlerTHR(_context, cms_error_handler);
    }

    ~TransformContext()
    {
        cmsDeleteContext(_context);
    }
    cmsContext _context;
};

class TransformSurface : public Transform
{
public:
    static auto get_context()
    {
        static TransformContext c;
        return c._context;
    }
    /**
     * The format of surface data being transformed by CMS
     *
     * @arg profile       - The CMS Profile the surface data will start in.
     * @arg byte_count    - Number of bytes used per pixel in the input.
     * @arg integral      - True if the input is integer, false if a decimal.
     * @arg premultiplied - Is the format pre-multiplied or not, ONLY works for input data!
     * @arg has_alpha     - True if the format contains an alpha channel, must be true for
     *                      premultiplied alpha.
     */
    struct Format
    {
        std::shared_ptr<Profile> profile;

        int  byte_count;
        bool integral;
        bool premultiplied = true;
        bool has_alpha = true;
    };

    /**
     * Construct a transformation suitable for display conversion in a surface buffer
     *
     * @arg input        - The input data format (see above)
     * @arg output       - The output data format
     * @arg intent       - The rendering intent for the conversion between input and output.
     * @arg proof        - A profile to apply a proofing step to, this can be CMYK for example.
     * @arg proof_intent - An optional intent for the proofing conversion
     * @arg gamut_warn   - Optional flag for rendering out of gamut colors with a warning color.
     */
    TransformSurface(Format const &input,
                     Format const &output,
                     RenderingIntent intent = RenderingIntent::PERCEPTUAL,
                     std::shared_ptr<Profile> const &proof = nullptr,
                     RenderingIntent proof_intent = RenderingIntent::AUTO,
                     bool gamut_warn = false)
        : Transform(proof
                        ? cmsCreateProofingTransformTHR(
                              get_context(), input.profile->getHandle(),
                              lcms_color_format(input.profile, input.byte_count, !input.integral, alpha_mode(input.premultiplied, input.has_alpha)),
                              output.profile->getHandle(),
                              lcms_color_format(output.profile, output.byte_count, !output.integral, alpha_mode(false, output.has_alpha)),
                              proof->getHandle(), lcms_intent(intent), lcms_intent(proof_intent),
                              (input.has_alpha && output.has_alpha ? cmsFLAGS_COPY_ALPHA : 0) | cmsFLAGS_SOFTPROOFING | (gamut_warn ? cmsFLAGS_GAMUTCHECK : 0) |
                                  lcms_bpc(proof_intent))
                        : cmsCreateTransformTHR(
                              get_context(), input.profile->getHandle(),
                              lcms_color_format(input.profile, input.byte_count, !input.integral, alpha_mode(input.premultiplied, input.has_alpha)),
                              output.profile->getHandle(),
                              lcms_color_format(output.profile, output.byte_count, !output.integral, alpha_mode(false, output.has_alpha)),
                              lcms_intent(intent), (input.has_alpha && output.has_alpha ? cmsFLAGS_COPY_ALPHA : 0)),
                    false)
        , _pixel_size_in((_channels_in + input.has_alpha) * input.byte_count)
        , _pixel_size_out((_channels_out + output.has_alpha) * output.byte_count)
    {
        assert(!gamut_warn || (input.integral && input.byte_count == 2 && output.integral && output.byte_count == 2));
    }

    template <typename TypeIn, typename TypeOut = TypeIn>
    static TransformSurface create(std::shared_ptr<Profile> const &in_profile, 
                            std::shared_ptr<Profile> const &out_profile,
                            RenderingIntent intent = RenderingIntent::PERCEPTUAL,
                            std::shared_ptr<Profile> const &proof = nullptr,
                            RenderingIntent proof_intent = RenderingIntent::AUTO,
                            bool premultiplied = true, bool gamut_warn = false,
                            bool in_alpha = true, bool out_alpha = true)
    {
        auto input  = Format(in_profile, sizeof(TypeIn), std::is_integral_v<TypeIn>, premultiplied, in_alpha);
        auto output = Format(out_profile, sizeof(TypeOut), std::is_integral_v<TypeOut>, false, out_alpha);
        return TransformSurface(input, output, intent, proof, proof_intent, gamut_warn);
    }

    /**
     * Apply the CMS transform to the surface and paint it into the output surface.
     *
     * @arg width      - The width of the image to transform
     * @arg height     - The height of the image to transform
     * @arg px_in      - The source surface with the pixels to transform.
     * @arg px_out     - The destination surface which may be the same as in.
     * @arg stride_in  - The optional stride for the input image, if known to be uncontiguous.
     * @arg stride_out - The optional stride for the output image, if known to be uncontiguous.
     */
    template <typename TypeIn, typename TypeOut>
    void do_transform(int width, int height, TypeIn const *px_in, TypeOut *px_out, int stride_in = 0,
                      int stride_out = 0) const
    {
        cmsDoTransformLineStride(_handle, px_in, px_out, width, height, stride_in ? stride_in : width * _pixel_size_in,
                                 stride_out ? stride_out : width * _pixel_size_out, 0, 0);
    }

    /**
     * Set the alarm code / gamut warn color for this transformation.
     */
    void set_gamut_warn_color(std::vector<double> const &input)
    {
        std::array<cmsUInt16Number, cmsMAXCHANNELS> color;
        color.fill(0);

        for (auto i = 0; i < input.size(); i++) {
            color[i] = input[i] * std::numeric_limits<uint16_t>::max();
        }

        if (_context) {
            cmsSetAlarmCodesTHR(_context, color.data());
        } else {
            cmsSetAlarmCodes(color.data());
        }
    }

private:
    int const _pixel_size_in;
    int const _pixel_size_out;
};

} // namespace Inkscape::Colors::CMS

#endif // SEEN_COLORS_CMS_TRANSFORM_SURFACE_H

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
