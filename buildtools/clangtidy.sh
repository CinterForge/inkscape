#
# clang-tidy is an utility to check that some c++ code good practices and patterns are respected. We use it at 1% of its possibilities (only when it suggests fixes).
#

CMP_BRANCH="origin/master"
GIT_DIR=`git rev-parse --show-toplevel`

if [ ! "$GIT_DIR" ]; then
    echo "This must be run from a git repository."
    exit 2
fi

if [ "$1" != "" ]; then
    CMP_BRANCH="$1"
fi

# "borrow" a 3rdparty file to dump header includes
BORROW="src/3rdparty/adaptagrams/libcola/box.cpp"

git diff $CMP_BRANCH --name-only -- '**.h' | sed 's!.*!#include"../../../../&"!' | tee $BORROW
git diff $CMP_BRANCH --name-only -- '**.cpp' | tee -a clang_tidy_files
bash buildtools/clangtidy-helper.sh $(cat clang_tidy_files)
rm clang_tidy_files

