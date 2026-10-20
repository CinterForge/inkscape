#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

MY_LOCATION=$(dirname "$0")
source "${MY_LOCATION}/../utils/functions.sh"

OUTPUT_FILENAME=$1
REFERENCE_FILENAME=$2

get_outputs

test -f "${OUTPUT_FILENAME}" || { echo "compare.sh: First file '${OUTPUT_FILENAME}' not found."; exit 1; }
test -f "${REFERENCE_FILENAME}" || { echo "compare.sh: Second file '${REFERENCE_FILENAME}' not found."; exit 1; }

sed -i "s/LMSans..-......./'Latin Modern Sans'/" ${OUTPUT_FILENAME}

FILTER='sed -e /inkscape:version/d -e /sodipodi:namedview/,/>/d'

if ! cmp <($FILTER "${OUTPUT_FILENAME}") <($FILTER "${REFERENCE_FILENAME}"); then
    echo "compare.sh: Files '${OUTPUT_FILENAME}' and '${REFERENCE_FILENAME}' are not identical'."
    keep_outputs
    exit 1
fi
