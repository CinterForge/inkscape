#
# This script checks that **changes** to the code conform to the code style.
#
# See clang-format ./_clang-format and https://inkscape.org/develop/coding-style/
#

CMP_BRANCH="origin/master"
GIT_DIR=`git rev-parse --show-toplevel`

if [ ! "$GIT_DIR" ]; then
    echo "This must be run from a git repository."
    exit 2
fi

CLANG_FILE="$GIT_DIR/_clang-format"

if [ ! -f "$CLANG_FILE" ]; then
    echo "Can't find clang format file in '$CLANG_FILE'."
    exit 2
fi

if [ "$1" != "" ]; then
    CMP_BRANCH="$1"
fi

cd $GIT_DIR
git diff $CMP_BRANCH -U0 --no-color $2 | clang-format-diff -p1 -style file
if [ $? -eq 1 ]; then
    >&2 echo ""
    >&2 echo "  !! WARNING !!"
    >&2 echo "  Your clang-format-diff returned exit code 1, this indicates your clang"
    >&2 echo "  version is buggy and is only outputting one file at a time."
    >&2 echo ""
    >&2 echo "  You must run this multiple times until the diff is empty."
    >&2 echo ""
fi

