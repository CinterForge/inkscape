#!/usr/bin/env bash
set -e
# -------------------------------------------------------------------------------
# This script installs all dependencies required for building Inkscape with MSYS2.
#
# See https://wiki.inkscape.org/wiki/Compiling_Inkscape_on_Windows_with_MSYS2 for
# detailed instructions.
#
# The following instructions assume you are building for the standard x86_64 processor architecture,
# which means that you use the UCRT64 variant of msys2.
# Else, replace UCRT64 with the appropriate variant for your architecture.
#
# To run this script, execute it once on an UCRT64 shell, i.e.
#    - use the "MSYS2 UCRT64" shortcut in the start menu or
#    - run "ucrt64.exe" in MSYS2's installation folder
#
# MSYS2 and installed libraries can be updated later by executing
#   pacman -Syu
# -------------------------------------------------------------------------------

# select if you want to build 32-bit (i686), 64-bit (x86_64), or both
case "$MSYSTEM" in
  MINGW32)
    ARCH=mingw-w64-i686
    ;;
  MINGW64)
    ARCH=mingw-w64-x86_64
    ;;
  UCRT64)
    ARCH=mingw-w64-ucrt-x86_64
    ;;
  CLANG64)
    ARCH=mingw-w64-clang-x86_64
    ;;
  CLANGARM64)
    ARCH=mingw-w64-clang-aarch64
    ;;
  *)
    ARCH={mingw-w64-i686,mingw-w64-x86_64}
    ;;
esac

PACMAN_OPTIONS="--needed --noconfirm"

# sync package databases
pacman -Sy

# install basic development system, compiler toolchain and build tools
eval pacman -S $PACMAN_OPTIONS \
git \
base-devel \
$ARCH-autotools \
$ARCH-ccache \
$ARCH-cmake \
$ARCH-meson \
$ARCH-ninja \
$ARCH-toolchain

# install Inkscape dependencies (required)
eval pacman -S $PACMAN_OPTIONS \
$ARCH-boost \
$ARCH-double-conversion \
$ARCH-gc \
$ARCH-gsl \
$ARCH-gtk4 \
$ARCH-gtk-doc \
$ARCH-gtkmm-4.0 \
$ARCH-libxslt

# install packaging tools (required for dist-win-* targets)
eval pacman -S $PACMAN_OPTIONS \
$ARCH-7zip \
$ARCH-nsis \
$ARCH-ntldd

# install Inkscape dependencies (optional)
eval pacman -S $PACMAN_OPTIONS \
$ARCH-enchant \
$ARCH-graphicsmagick \
$ARCH-gtksourceview5 \
$ARCH-harfbuzz-cairo \
$ARCH-libheif \
$ARCH-libcdr \
$ARCH-libjxl \
$ARCH-libspelling \
$ARCH-libvisio \
$ARCH-libwpg \
$ARCH-poppler \
$ARCH-potrace \
$ARCH-webp-pixbuf-loader

# install Python and modules used by Inkscape
eval pacman -S $PACMAN_OPTIONS \
$ARCH-python \
$ARCH-python-coverage \
$ARCH-python-cssselect \
$ARCH-python-gobject \
$ARCH-python-lxml \
$ARCH-python-numpy \
$ARCH-python-packaging \
$ARCH-python-pillow \
$ARCH-python-pip \
$ARCH-python-pyparsing \
$ARCH-python-pyserial \
$ARCH-python-six \
$ARCH-python-tinycss2 \
$ARCH-python-webencodings \
$ARCH-python-zstandard \
$ARCH-scour

# install modules needed by extensions manager and clipart importer
eval pacman -S $PACMAN_OPTIONS \
$ARCH-python-platformdirs \
$ARCH-python-beautifulsoup4 \
$ARCH-python-cachecontrol \
$ARCH-python-certifi \
$ARCH-python-chardet \
$ARCH-python-filelock \
$ARCH-python-idna \
$ARCH-python-msgpack \
$ARCH-python-requests \
$ARCH-python-urllib3

# install packages for testing Inkscape
eval pacman -S $PACMAN_OPTIONS \
$ARCH-ghostscript \
$ARCH-gtest

# install Python modules not provided as MSYS2/MinGW packages
PACKAGES=""
for arch in $(eval echo $ARCH); do
  case ${arch} in
    mingw-w64-i686)
      #/mingw32/bin/pip3 install --upgrade ${PACKAGES}
      ;;
    mingw-w64-x86_64)
      #/mingw64/bin/pip3 install --upgrade ${PACKAGES}
      ;;
    mingw-w64-ucrt-x86_64)
      #/ucrt64/bin/pip3 install --upgrade ${PACKAGES}
      ;;
    mingw-w64-clang-x86_64)
      #/clang64/bin/pip3 install --upgrade ${PACKAGES}
      ;;
    mingw-w64-clang-aarch64)
      #/clangarm64/bin/pip3 install --upgrade ${PACKAGES}
      ;;
  esac
done

echo "Done :-)"
