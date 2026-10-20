#!/bin/bash
set -euo pipefail

if [ -z ${CI_MERGE_REQUEST_ID-} ]; then
	echo "Not from a MR"
        exit 0
fi

if [ -z ${API_TOKEN-} ]; then
	echo "No token"
        exit 0
fi

CHANGES=$(git diff ${CI_MERGE_REQUEST_DIFF_BASE_SHA} --name-only ../po/*.po | cut -d '/' -f 2 | cut -d '.' -f1)
NCHANGES=$(git diff ${CI_MERGE_REQUEST_DIFF_BASE_SHA} --name-only ../po/*.po | wc -l)
if [ $NCHANGES -eq 0 ]; then
	echo "No translation changes"
	exit 0
fi

./update_po_files.sh

tmpfile=$(mktemp)

echo '```' > ${tmpfile}
./language_statistics.sh $CHANGES | tee -a ${tmpfile}
echo '```' >> ${tmpfile}

curl --request POST --header "PRIVATE-TOKEN: $API_TOKEN" -G https://gitlab.com/api/v4/projects/${CI_MERGE_REQUEST_PROJECT_ID}/merge_requests/${CI_MERGE_REQUEST_IID}/notes --data-urlencode body@${tmpfile}
