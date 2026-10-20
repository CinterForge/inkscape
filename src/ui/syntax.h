// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Syntax coloring via Gtksourceview and Pango markup.
 */
/* Authors:
 *   Rafael Siejakowski <rs@rs-math.net>
 *
 * Copyright (C) 2022 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_UI_SYNTAX_H
#define SEEN_UI_UI_SYNTAX_H

#include <glibmm/markup.h>
#include <cstdint>
#include <optional>

namespace Gtk {
class TextView;
}

namespace Inkscape::UI::Syntax {

/** The style of a single element in a (Pango markup)-enabled widget. */
struct Style
{
    std::optional<Glib::ustring> color;
    std::optional<Glib::ustring> background;
    uint8_t bold      : 1;
    uint8_t italic    : 1;
    uint8_t underline : 1;

    Style()
        : bold{false}
        , italic{false}
        , underline{false}
    {}

    bool isDefault() const { return !color && !background && !bold && !italic && !underline; }
    Glib::ustring openingTag() const;
    Glib::ustring closingTag() const;
};

/** The styles used for simple XML syntax highlighting. */
struct XMLStyles
{
    Style prolog;
    Style comment;
    Style angular_brackets;
    Style tag_name;
    Style attribute_name;
    Style attribute_value;
    Style content;
    Style error;
};

/** @brief A formatter for XML syntax, based on Pango markup.
 *
 * This mechanism is used in the TreeView in the XML Dialog,
 * where the syntax highlighting of XML tags is accomplished
 * via Pango markup.
 */
class XMLFormatter
{
public:
    XMLFormatter() = default;
    XMLFormatter(XMLStyles &&styles)
        : _style{styles}
    {}

    void setStyle(XMLStyles const &new_style) { _style = new_style; }
    void setStyle(XMLStyles &&new_style) { _style = new_style; }

    void openTag(char const *tag_name);
    void addAttribute(char const *attribute_name, char const *attribute_value);
    Glib::ustring finishTag(bool self_close = false);

    Glib::ustring formatContent(char const* content, bool wrap_in_quotes = true) const;
    Glib::ustring formatComment(char const* comment, bool wrap_in_comment_marks = true) const;
    Glib::ustring formatProlog(char const* prolog) const { return _format(_style.prolog, prolog); }

private:
    Glib::ustring _format(Style const &style, Glib::ustring const &content) const;
    Glib::ustring _format(Style const &style, char const *content) const
    {
        return style.openingTag() + Glib::Markup::escape_text(content) + style.closingTag();
    }

    XMLStyles _style;
    Glib::ustring _wip;
};

/// Build XML styles from a GTKSourceView syntax color theme.
XMLStyles build_xml_styles(const Glib::ustring& syntax_theme);

/// Syntax highlighting mode (language).
enum class SyntaxMode
{
    PlainText,     ///< Plain text (no highlighting).
    InlineCss,     ///< Inline CSS (contents of a style="..." attribute).
    CssStyle,      ///< File-scope CSS (contents of a CSS file or a <style> tag).
    SvgPathData,   ///< Contents of the 'd' attribute of the SVG <path> element.
    SvgPolyPoints, ///< Contents of the 'points' attribute of <polyline> or <polygon>.
    JavaScript     // JavaScript code
};

/// Base class for styled text editing widget.
class TextEditView
{
public:
    virtual ~TextEditView() = default;
    virtual void setStyle(const Glib::ustring& theme) = 0;
    virtual void setText(const Glib::ustring& text) = 0;
    virtual Glib::ustring getText() const = 0;
    virtual Gtk::TextView& getTextView() const = 0;

    static std::unique_ptr<TextEditView> create(SyntaxMode mode);
};

} // namespace Inkscape::UI::Syntax

#endif // SEEN_UI_UI_SYNTAX_H

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
