// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Helpers for using Gtk::Boxes, encapsulating large changes between GTK3 & GTK4
 */
/*
 * Authors:
 *   Daniel Boles <dboles.src+inkscape@gmail.com>
 *
 * Copyright (C) 2023 Daniel Boles
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_UI_BOX_PACK_H
#define SEEN_UI_BOX_PACK_H

namespace Gtk {
class Box;
class Widget;
} // namespace Gtk

namespace Inkscape::UI {

/// Equivalent to GTK3ʼs Gtk::PackOptions
enum class PackOptions {shrink, expand_padding, expand_widget};

/// Adds child to box, packed with reference to the start of box.
/// The child is packed after any other child packed with reference to the start of box.
///
/// Our pack_*() functions aim to replace GTKʼs Box.pack_start() in a GTK4-ready
/// way, so code built against GTK3 can swap to our functions and afterward will
/// not have need rewritten in migrating to GTK4. If writing new code you should
/// probably avoid using these if you can, as they cannot be converted from C++
/// to .ui for instance. Instead, set properties on child widgets (not using the
/// GTK3-only child properties!) and use Box.add() in GTK3 or .append() in GTK4.
///
/// Internally, the list of children in the Box is maintained in the order of:
/// * widgets added by our pack_start(), in the same order of the calls thereto;
/// * …widgets added by our pack_end() in the *opposite* order of calls thereto.
///
/// The expand, fill and padding are implemented by setting relevant [hv]expand,
/// [hv]align and margin-* properties on the child, instead of GTK3 child props.
///
/// @param child The Gtk::Widget to be added to the box.
///
/// @param expand TRUE if the new child is to be given extra space allocated to
///     box. The extra space will be divided evenly between all children that
///     use this option.
///     This is implemented by setting the childʼs relevant [hv]expand property.
///     Note that there’s a subtle but important difference between GtkBox‘s
///     expand and fill child properties and the ones in GtkWidget: setting
///     GtkWidget:hexpand or GtkWidget:vexpand to TRUE will propagate up the
///     widget hierarchy, so a pixel-perfect port might require you to reset the
///     expansion flags to FALSE in a parent widget higher up the hierarchy, or
///     to set the child to not expand (shrink). Our pack_*() functions do not
///     attempt to do workaround this for you, as that might cause NEW problems.
///
/// @param fill TRUE if space given to child by the expand option is actually
///     allocated to child, rather than just padding it. This parameter has no
///     effect if expand is set to FALSE. A child is always allocated the full
///     height of a horizontal GtkBox and the full width of a vertical GtkBox.
///     This option affects the other dimension.
///     This is implemented by setting the childʼs relevant [hv]align prop to
///     ALIGN_FILL if fill is TRUE, else to START or END to match the pack type.
///
/// @param padding Extra space in pixels to put between this child and its
///     neighbors, over and above the global amount specified by GtkBox:spacing
///     property. If child is a widget at one of the reference ends of box, then
///     padding pixels are also put between child and the reference edge of box.
///     This is implemented by adding to the childʼs relevant margin-* props.
void pack_start(Gtk::Box &box, Gtk::Widget &child, bool expand, bool fill,
                unsigned padding = 0);

/// Adds child to box, packed with reference to the start of box.
/// @param options The PackOptions to use, which are decomposed into booleans of
///     whether to expand or fill and passed to the other overload; see its doc.
void pack_start(Gtk::Box &box, Gtk::Widget &child,
                PackOptions options = PackOptions::expand_widget,
                unsigned padding = 0);

/// Adds child to box, packed with reference to the end of box.
/// The child is packed after (away from end of) any other child packed with reference to the end of box.
/// See the documentation of pack_start() for details of the parameters.
void pack_end(Gtk::Box &box, Gtk::Widget &child, bool expand, bool fill,
              unsigned padding = 0);

/// Adds child to box, packed with reference to the end of box.
/// The child is packed after (away from end of) any other child packed with reference to the end of box.
/// @param options The PackOptions to use, which are decomposed into booleans of
///     whether to expand or fill and passed to the other overload; see its doc.
void pack_end(Gtk::Box &box, Gtk::Widget &child,
              PackOptions options = PackOptions::expand_widget,
              unsigned padding = 0);

} // namespace Inkscape::UI

#endif // SEEN_UI_BOX_PACK_H

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
