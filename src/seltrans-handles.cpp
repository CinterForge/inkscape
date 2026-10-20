// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "seltrans-handles.h"

#ifdef __cplusplus
#undef N_
#define N_(x) x
#endif

SPSelTransHandle const hands[] = {
    // clang-format off
    //center handle will be 0 so we can reference it quickly.
    { HANDLE_CENTER,        SP_ANCHOR_CENTER,  12,    0.5,  0.5 },
    //handle-type           anchor-nudge       image  x     y
    { HANDLE_SCALE,         SP_ANCHOR_SE,      0,     0,    1   },
    { HANDLE_STRETCH,       SP_ANCHOR_S,       3,     0.5,  1   },
    { HANDLE_SCALE,         SP_ANCHOR_SW,      1,     1,    1   },
    { HANDLE_STRETCH,       SP_ANCHOR_W,       2,     1,    0.5 },
    { HANDLE_SCALE,         SP_ANCHOR_NW,      0,     1,    0   },
    { HANDLE_STRETCH,       SP_ANCHOR_N,       3,     0.5,  0   },
    { HANDLE_SCALE,         SP_ANCHOR_NE,      1,     0,    0   },
    { HANDLE_STRETCH,       SP_ANCHOR_E,       2,     0,    0.5 },
    { HANDLE_ROTATE,        SP_ANCHOR_SE,      4,     0,    1   },
    { HANDLE_SKEW,          SP_ANCHOR_S,       8,     0.5,  1   },
    { HANDLE_ROTATE,        SP_ANCHOR_SW,      5,     1,    1   },
    { HANDLE_SKEW,          SP_ANCHOR_W,       9,     1,    0.5 },
    { HANDLE_ROTATE,        SP_ANCHOR_NW,      6,     1,    0   },
    { HANDLE_SKEW,          SP_ANCHOR_N,       10,    0.5,  0   },
    { HANDLE_ROTATE,        SP_ANCHOR_NE,      7,     0,    0   },
    { HANDLE_SKEW,          SP_ANCHOR_E,       11,    0,    0.5 },

    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_S,       13,    0.5,  1   },
    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_W,       14,    1,    0.5 },
    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_N,       15,    0.5,  0   },
    { HANDLE_SIDE_ALIGN,    SP_ANCHOR_E,       16,    0,    0.5 },
    { HANDLE_CENTER_ALIGN,  SP_ANCHOR_CENTER,  17,    0.5,  0.5 },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_SE,      18,    0,    1   },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_SW,      19,    1,    1   },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_NW,      20,    1,    0   },
    { HANDLE_CORNER_ALIGN,  SP_ANCHOR_NE,      21,    0,    0   },
    // clang-format on
};

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
