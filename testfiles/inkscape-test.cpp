// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape integration test main.
 *
 * Author:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2015 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtest/gtest.h>

#include <glib.h>
#include <giomm/init.h>
#include <gsl/gsl_errno.h>

#include "inkgc/gc-core.h"
#include "util/statics.h"

int main(int argc, char **argv)
{
    Inkscape::Util::Statics statics;

    // Opt into handling GSL errors locally, rather than crashing.
    gsl_set_error_handler_off();

    // If possible, unit tests shouldn't require a GUI session
    // since this won't generally be available in auto-builders

    Gio::init();

    Inkscape::GC::init();

    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();

    return ret;
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
