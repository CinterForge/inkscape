// SPDX-License-Identifier: GPL-2.0-or-later

#include <functional>
#include <glibmm/ustring.h>
#include <memory>
#include <sigc++/signal.h>
#include <vector>
#include <pangomm.h>
#include "async/operation-stream.h"
#include <sigc++/scoped_connection.h>
#include "libnrtype/font-factory.h"
#include "statics.h"

namespace Inkscape {

struct FontInfo {
    Glib::RefPtr<Pango::FontFamily> ff;
    Glib::RefPtr<Pango::FontFace> face;
    Glib::ustring variations;   // pango-style font variations (if any)
    double weight = 0;           // proxy for font weight - how black it is
    double width = 0;            // proxy for font width - how compressed/extended it is
    unsigned short family_kind = 0; // OS/2 family class
    bool monospaced = false;    // fixed-width font
    bool oblique = false;       // italic or oblique font
    bool variable_font = false; // this is variable font
    bool synthetic = false;     // this is an alias, like "Sans" or "Monospace"

    bool operator == (const FontInfo&) const = default;
};

enum class FontOrder {
    _First = 0,
    ByName = 0,
    ByWeight,
    ByWidth,
    ByFamily,
    _Last = ByFamily
};

class FontDiscovery : public Util::EnableSingleton<FontDiscovery, Util::Depends<FontFactory>>
{
public:
    using FontsPayload = std::shared_ptr<const std::vector<std::vector<FontInfo>>>;
    using MessageType = Async::Msg::Message<FontsPayload, double, Glib::ustring, std::vector<FontInfo>>;

    sigc::scoped_connection connect_to_fonts(std::function<void (const MessageType&)> fn);

private:
    FontDiscovery();
    friend class EnableSingleton;

    FontsPayload _fonts;
    sigc::scoped_connection _connection;
    Inkscape::Async::OperationStream<FontsPayload, double, Glib::ustring, std::vector<FontInfo>> _loading;
    sigc::signal<void (const MessageType&)>_events;
};

// Use font factory and cached font details to return a list of all fonts available to Inkscape
std::vector<FontInfo> get_all_fonts();

// change order
void sort_fonts(std::vector<FontInfo>& fonts, FontOrder order, bool sans_first);

// sort font families
void sort_font_families(std::vector<std::vector<FontInfo>>& fonts, bool sans_first);

// get regular font from the family
const FontInfo& get_family_font(const std::vector<FontInfo>& family);
FontInfo& get_family_font(std::vector<FontInfo>& family);

Pango::FontDescription get_font_description(const Glib::RefPtr<Pango::FontFamily>& ff, const Glib::RefPtr<Pango::FontFace>& face);

Glib::ustring get_fontspec(const Glib::ustring& family, const Glib::ustring& face);
Glib::ustring get_fontspec(const Glib::ustring& family, const Glib::ustring& face, const Glib::ustring& variations);

Glib::ustring get_face_style(const Pango::FontDescription& desc);

Glib::ustring get_fontspec_without_variants(const Glib::ustring& fontspec);

Glib::ustring get_inkscape_fontspec(
    const Glib::RefPtr<Pango::FontFamily>& ff,
    const Glib::RefPtr<Pango::FontFace>& face,
    const Glib::ustring& variations);

// combine font style, weight, stretch and other traits to come up with a value
// that can be used to order font faces within the same family
int get_font_style_order(const Pango::FontDescription& desc);

Glib::ustring get_full_font_name(Glib::RefPtr<Pango::FontFamily> ff, Glib::RefPtr<Pango::FontFace> face);

} // namespace Inkscape
