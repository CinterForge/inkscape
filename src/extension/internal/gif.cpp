// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 4/15/25.
//

#include "gif.h"

#include <cairomm/context.h>
#include <cairomm/surface.h>

#include "cairo-render-context.h"
#include "cairo-renderer.h"
#include "document.h"
#include "extension/system.h"
#include "lib-gif.h"
#include "page-manager.h"
#include "extension/extension.h"
#include "util/units.h"
#include <glibmm/i18n.h>

#include "extension/output.h"

namespace Inkscape::Extension::Internal {

#undef N_
#define N_(str) str

void Gif::init() {
    build_from_mem(
R"(<?xml version="1.0" encoding="UTF-8" ?>
<inkscape-extension xmlns="http://www.inkscape.org/namespace/inkscape/extension">
    <_name>Animated GIF</_name>
    <id>org.inkscape.output.gif.animated</id>
    <param name='delay' gui-text=')" N_("Default frame duration (ms)") R"(' type='float' min='0.0' max='100000000.0'>100.0</param>
    <label>)" N_("Note: frame duration accuracy is 10 ms.") R"(</label>
    <param name='bit-depth' gui-text=')" N_("Bit depth (1-8)") R"(' type='int' min='1' max='8'>8</param>
    <param name='dither' type='bool' gui-text=')" N_("Dithering") R"('>false</param>
    <separator/>
    <label>)" N_("To save animated GIF go to:") R"(</label>
    <label>)" N_("'Export - Single File - Page'") R"(</label>
    <output is_exported='true' priority='2'>
        <extension>.gif</extension>
        <mimetype>image/gif</mimetype>
        <filetypename>)" N_("GIF (*.gif)") R"(</filetypename>
        <filetypetooltip>)" N_("Graphics Interchange Format") R"(</filetypetooltip>
    </output>
</inkscape-extension>
)", std::make_unique<Gif>());
}

#undef N_

void Gif::save(Output* extension, SPDocument* doc, char const* filename) {
    if (!doc || !filename || !extension) return;

    auto width = static_cast<uint32_t>(doc->getWidth().value("px"));
    auto height = static_cast<uint32_t>(doc->getHeight().value("px"));

    GifWriter writer;
    auto dither = extension->get_param_bool("dither");
    auto bit_depth = extension->get_param_int("bit-depth");
    auto delay = static_cast<int>(extension->get_param_float("delay") / 10);
    if (!GifBegin(&writer, filename, width, height, delay, bit_depth, dither)) {
        g_warning(_("Failed to create file '%s'"), filename);
        return;
    }

    auto& pm = doc->getPageManager();
    auto color = pm.getBackgroundColor();
    color.convert(Colors::Space::Type::RGB);

    // add frames
    for (auto page : pm.getPages()) {
        CairoRenderer renderer;
        auto ctx = renderer.createContext();

        ctx.setTextToPath(false);
        ctx.setFilterToBitmap(true);
        ctx.setBitmapResolution(72);

        auto format = Cairo::Surface::Format::ARGB32;
        int stride = Cairo::ImageSurface::format_stride_for_width(format, width);
        std::vector<unsigned char> frame(stride * height);
        auto surface = Cairo::ImageSurface::create(frame.data(), format, width, height, stride);
        auto context = Cairo::Context::create(surface);
        context->set_source_rgba(color[0], color[1], color[2], 1.0);
        context->paint();
        auto ctm = context->get_matrix();

        bool ret = ctx.setSurfaceTarget(surface->cobj(), false, &ctm);
        if (!ret) {
            g_warning("%s", _("Failed to set CairoRenderContext"));
            break;
        }
        ret = renderer.setupDocument(&ctx, doc);
        if (!ret) {
            g_warning("%s", _("Could not set up Document"));
            break;
        }
        renderer.renderPage(&ctx, doc, page, false);
        ctx.finish(false);  // do not finish the cairo_surface_t - it's owned by our GtkPrintContext!

        // swap Red with Blue
        auto pix = frame.data();
        auto lim = pix + frame.size();
        while (pix < lim) {
            std::swap(pix[0], pix[2]);
            pix += 4;
        }
        GifWriteFrame(&writer, frame.data(), width, height, delay, bit_depth, dither);
    }

    GifEnd(&writer);
}

} // namespace
