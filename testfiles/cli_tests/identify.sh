#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

file1=$1
PATTERN=$2

test -f "${file1}" || { echo "identify.sh: File '${file1}' not found."; exit 1; }
if ! $(identify "${file1}" | grep -q -e "${PATTERN}" - ); then
    echo "expected $2 but got" `identify "${file1}"`
    exit 1
fi
