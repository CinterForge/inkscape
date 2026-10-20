// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *  Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *
 * Copyright (C) 2007 Author
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_WIDGET_RANDOM_H
#define INKSCAPE_UI_WIDGET_RANDOM_H

#include "scalar.h"

namespace Inkscape::UI::Widget {

/**
 * A labelled text box, with spin buttons and optional
 * icon, for entering arbitrary number values. It adds an extra
 * number called "startseed", that is not UI edittable, but should be put in SVG.
 * This does NOT generate a random number, but provides merely the saving of 
 * the startseed value.
 */
class Random : public Scalar
{
public:

    /**
     * Construct a Random scalar Widget.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to false).
     */
    Random(Glib::ustring const &label,
           Glib::ustring const &tooltip,
           Glib::ustring const &icon = {},
           bool mnemonic = true);

    /**
     * Construct a  Random Scalar Widget.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param digits    Number of decimal digits to display.
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to false).
     */
    Random(Glib::ustring const &label,
           Glib::ustring const &tooltip,
           unsigned digits,
           Glib::ustring const &icon = {},
           bool mnemonic = true);

    /**
     * Construct a Random Scalar Widget.
     *
     * @param label     Label, as per the Labelled base class.
     * @param tooltip   Tooltip, as per the Labelled base class.
     * @param adjust    Adjustment to use for the SpinButton.
     * @param digits    Number of decimal digits to display (defaults to 0).
     * @param icon      Icon name, placed before the label (defaults to empty).
     * @param mnemonic  Mnemonic toggle; if true, an underscore (_) in the label
     *                  indicates the next character should be used for the
     *                  mnemonic accelerator key (defaults to true).
     */
    Random(Glib::ustring const &label,
           Glib::ustring const &tooltip,
	   Glib::RefPtr<Gtk::Adjustment> adjust,
           unsigned digits = 0,
           Glib::ustring const &icon = {},
           bool mnemonic = true);

    /**
     * Gets the startseed.
     */
    long getStartSeed() const;

    /**
     * Sets the startseed number.
     */
    void setStartSeed(long newseed);

    sigc::signal<void ()> signal_reseeded;

protected:
    long startseed;

private:

    /**
     * Add reseed button to the widget.
     */
    void addReseedButton();

    void onReseedButtonClick();
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_RANDOM_H

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
