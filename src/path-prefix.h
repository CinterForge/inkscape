// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_PATH_PREFIX_H
#define SEEN_PATH_PREFIX_H

#include <string>

char const *get_inkscape_datadir();
char const *get_program_name();
char const *get_program_dir();
char const *get_user_config_dir();

void set_xdg_env();

#endif /* _PATH_PREFIX_H_ */
