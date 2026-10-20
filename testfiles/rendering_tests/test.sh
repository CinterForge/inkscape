#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

MY_LOCATION=$(dirname "$0")
source "${MY_LOCATION}/../utils/functions.sh"

ensure_command "compare"
ensure_command "bc"

if [ "$#" -lt 2 ]; then
    echo "Pass the path of the inkscape executable as parameter then the name of the test" $#
    exit 1
fi

INKSCAPE_EXE="$1"
TEST="$2"
FUZZ="$3"
EXIT_STATUS=0
EXPECTED="$(dirname "$TEST")/expected_rendering/$(basename "$TEST")"
TESTNAME="$(basename "$TEST")"
export LC_NUMERIC=C
export INKSCAPE_FONTCONFIG="${MY_LOCATION}/fonts/isolated.conf"

if [ "$FUZZ" = "" ]; then
    METRIC="AE"
else
    METRIC="RMSE"
fi

perform_test()
{
    local SUFFIX="$1"
    local DPI="$2"
    ${INKSCAPE_EXE} --export-png-use-dithering false --export-filename="${TESTNAME}${SUFFIX}.png" -d "$DPI" "${TEST}.svg"

    COMPARE_OUTPUT="$(compare -metric "$METRIC" "${TESTNAME}${SUFFIX}.png" "${EXPECTED}${SUFFIX}.png" "${TESTNAME}-compare${SUFFIX}.png" 2>&1)"

    if [ "$FUZZ" = "" ]; then
        if [ "$COMPARE_OUTPUT" = 0 ] || [ "$COMPARE_OUTPUT" = "0 (0)" ]; then
            echo "${TESTNAME}${SUFFIX}" "PASSED; absolute difference is exactly zero."
            rm "${TESTNAME}${SUFFIX}.png" "${TESTNAME}-compare${SUFFIX}.png"
        else
            echo "${TESTNAME} FAILED; absolute difference ${COMPARE_OUTPUT} is greater than zero."
            EXIT_STATUS=1
        fi
    else
        RELATIVE_ERROR=$(get_compare_result "$COMPARE_OUTPUT")
        PERCENTAGE_ERROR=$(fraction_to_percentage "$RELATIVE_ERROR")
        if (( $(is_relative_error_within_tolerance "$RELATIVE_ERROR" "$FUZZ") ))
        then
            echo "${TESTNAME}${SUFFIX}" "PASSED; error of ${PERCENTAGE_ERROR}% is within ${FUZZ}% tolerance."
            rm "${TESTNAME}${SUFFIX}.png" "${TESTNAME}-compare${SUFFIX}.png"
        else
            echo "${TESTNAME} FAILED; error of ${PERCENTAGE_ERROR}% exceeds ${FUZZ}% tolerance."
            EXIT_STATUS=1
        fi
    fi
}

perform_test "" 96

if [ -f "${EXPECTED}-large.png" ]; then
    perform_test "-large" 384
else
    echo "${TESTNAME}-large" "SKIPPED"
fi

exit $EXIT_STATUS
