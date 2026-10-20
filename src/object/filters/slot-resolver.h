// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SLOTRESOLVER_H
#define SLOTRESOLVER_H
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <string>
#include <unordered_map>
#include <optional>

class SlotResolver final
{
public:
    int read(std::optional<std::string> const &name) const;
    int read(std::string const &name) const;
    int write(std::optional<std::string> const &name);
    int write(std::string const &name);

private:
    std::unordered_map<std::string, int> map;
    int next = 1;
};

#endif // SLOTRESOLVER_H
