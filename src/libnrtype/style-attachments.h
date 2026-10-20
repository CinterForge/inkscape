// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Creates and maintains display tree needed for text styling.
 */
/*
 * This class is used by sp-text and sp-flowtext to maintain the display tree required by the
 * patterns and filters of tspans.
 *
 * The basic lifecycle of the DrawingText objects associated to tspans is that they are created
 * in Layout::set_visible(true), and destroyed in SP(Flow)Text::_clearFlow(), or at the end of the SP(Flow)Text's
 * lifetime by SPItem::release(). They don't need to be maintained during their lifetime; when they
 * need to be modified they are simply torn down and recreated.
 *
 * In order for patterns and filters to be correctly applied to tspans, certain extra pieces of display
 * tree must be attached to their DrawingText objects, and unattached at the right time to avoid crashes.
 * Normally this is handled by SPItem, however tspans are not SPItems and require their own code for this,
 * hence this class.
 *
 * A StyleAttachments allows creating display tree from a supplied SPFilter or SPPaintServer and
 * attaching it to a DrawingText. Upon deletion of the SPFilter/SPPaintServer, a call to unattachAll(),
 * or destruction, the display tree is then removed from the DrawingText.
 *
 * It is used as follows. When a tspan creates a DrawingText, each of the attach*() methods is called at
 * most once on it. Then just before the DrawingText is destroyed, unattachAll() or the destructor is called.
 */

#ifndef INKSCAPE_TEXT_STYLEATTACHMENTS_H
#define INKSCAPE_TEXT_STYLEATTACHMENTS_H

#include <vector>
#include <unordered_map>
#include <2geom/rect.h>
#include <sigc++/connection.h>

class SPFilter;
class SPPaintServer;

namespace Inkscape {
class DrawingText;
class DrawingPattern;

namespace Text {

class StyleAttachments
{
public:
    void attachFilter(DrawingText *item, SPFilter *filter);
    void attachFill(DrawingText *item, SPPaintServer *paintserver, Geom::OptRect const &bbox);
    void attachStroke(DrawingText *item, SPPaintServer *paintserver, Geom::OptRect const &bbox);
    void unattachAll();

private:
    class FilterEntry
    {
    public:
        FilterEntry(SPFilter *filter);
        FilterEntry(FilterEntry const &) = delete;
        FilterEntry const &operator=(FilterEntry const &) = delete;
        ~FilterEntry();

        void addItem(Inkscape::DrawingText *item);

    private:
        SPFilter *_filter;
        sigc::connection _conn;
        std::vector<Inkscape::DrawingText*> _items;

        void _removeAllItems();
    };

    class PatternEntry
    {
    public:
        PatternEntry(SPPaintServer *paintserver);
        PatternEntry(PatternEntry const &) = delete;
        PatternEntry const &operator=(PatternEntry const &) = delete;
        ~PatternEntry();

        void addFill(Inkscape::DrawingText *item, Geom::OptRect const &bbox);
        void addStroke(Inkscape::DrawingText *item, Geom::OptRect const &bbox);

    private:
        SPPaintServer *_paintserver;
        sigc::connection _conn;
        std::vector<unsigned> _keys;

        void _removeAllItems();
    };

    std::unordered_map<SPFilter*, FilterEntry> _filters;
    std::unordered_map<SPPaintServer*, PatternEntry> _patterns;
};

} // namespace Text
} // namespace Inkscape

#endif // INKSCAPE_TEXT_STYLEATTACHMENTS_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
