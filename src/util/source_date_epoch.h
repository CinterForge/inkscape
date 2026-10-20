// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Functions to parse the "SOURCE_DATE_EPOCH" environment variable for reproducible build hacks, see
 *     https://reproducible-builds.org/docs/source-date-epoch/
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SOURCE_DATE_EPOCH
#define SEEN_SOURCE_DATE_EPOCH

#include <ctime>
#include <glibmm/ustring.h>

namespace ReproducibleBuilds {

/** parse current time from SOURCE_DATE_EPOCH environment variable
 *
 *  \return current time (or zero if SOURCE_DATE_EPOCH unset)
 */
time_t now();

/** like ReproducibleBuilds::now() but returns a ISO 8601 formatted string
 *
 *  \return current time as ISO 8601 formatted string (or empty string if SOURCE_DATE_EPOCH unset)
 */
Glib::ustring now_iso_8601();

} // namespace ReproducibleBuilds

#endif // SEEN_SOURCE_DATE_EPOCH
