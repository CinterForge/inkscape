#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

testfile=$1

if test -f "${testfile}"
then
    echo "test_no_output_file.sh: testfile '${testfile}' was found.";
    exit 1;
fi
