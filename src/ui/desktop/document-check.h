// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Check for data loss when closing a document window.
 *
 * Copyright (C) 2021 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef DOCUMENT_CHECK_H

class SPDesktop;

bool document_check_for_data_loss(SPDesktop *desktop);

#endif // DOCUMENT_CHECK_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
