#!/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Author: Martin Owens
# Copyright: 2025
#
"""
Parse an SVG file for it's inkscape paages and output page numbers in stderr 
and the page labels in stdout. This is used by CMake to contruct tests.
"""

import os
import sys

def main(filename):
    if not os.path.isfile(filename):
        sys.stderr.write(f"Can't find file: '{filename}'")
        sys.exit(1)
    parse = None
    with open(filename, 'r') as fhl:
        for line in fhl:
            if "<defs" in line:
                parse = ""
            if parse is not None:
                parse += line
            if "</defs>" in line:
                break
    if not parse:
        return

    parse = parse.replace("\n", "").replace(">", "\n").split("<view")

    labels = []
    for i, page in enumerate(parse[1:]):
        attrs = dict([tuple(a.replace("\"", "").strip().split("=", 1)) for a in page.split("\" ") if "=" in a])
        label = attrs.get("inkscape:label", attrs.get("label", f"badpage{i+1}"))
        label = label.encode('ascii', 'replace').decode("ascii").lower()
        label = label.strip().replace(" ", "-").replace("_", "-").replace(";", "-")
        if label in labels or not label:
            label = f"badpage{i+1}"
        labels.append(label)

    if not labels:
        labels = ["page0"]

    sys.stderr.write(";".join([str(i) for i in range(len(labels))]))
    sys.stdout.write(";".join(labels))

if __name__ == "__main__":
    for f in sys.argv[1:]:
        main(f)
